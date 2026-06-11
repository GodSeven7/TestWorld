// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/GameHero.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "CineCameraComponent.h"
#include "ActorUnifiedDataComponent.h"
#include "Components/CombatWithUnifiedDataComponent.h"
#include "Components/BattleDamageWithUnifiedDataComponent.h"
#include "Components/SparseGridWithUnifiedDataComponent.h"
#include "CombatWeapon.h"
#include "BattleDamageTypes.h"
#include "BattleDamageManager.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "CharacterSkelMeshComponent.h"

AGameHero::AGameHero()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessPlayer = EAutoReceiveInput::Type::Player0;

    CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
    CameraSpringArm->SetupAttachment(RootComponent);
    CameraSpringArm->TargetArmLength = 10000.0f;
    CameraSpringArm->bUsePawnControlRotation = false;

    CameraComponent = CreateDefaultSubobject<UCineCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(CameraSpringArm);

    CameraSpringArm->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));

    if (SparseGridComponent)
    {
        SparseGridComponent->SetEnableSteering(false);
    }
}

void AGameHero::BeginPlay()
{
    Super::BeginPlay();

    if (!HeroConfig)
    {
        HeroConfig = GetClass()->GetDefaultObject<AGameHero>()->HeroConfig;
        if (!HeroConfig)
        {
            HeroConfig = LoadObject<UHeroConfig>(nullptr, TEXT("/Game/Config/DA_HeroConfig_Default"));
        }
    }

    if (CameraSpringArm)
    {
        CameraSpringArm->TargetArmLength = GetConfigCameraDistance();
        CameraSpringArm->SetRelativeRotation(FRotator(GetConfigCameraPitch(), GetConfigCameraYaw(), 0.0f));
    }

    SaveInitialSpawnLocation();

    if (IsConfigTiltShiftEnabled() && CameraComponent)
    {
        CameraComponent->FocusSettings.FocusMethod = ECameraFocusMethod::Manual;
        CameraComponent->FocusSettings.ManualFocusDistance = GetConfigTiltShiftFocusDistance();
        
        CameraComponent->LensSettings.MinFStop = 0.5f;
        CameraComponent->LensSettings.MaxFStop = 16.0f;        
    }

    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                if (InputMappingContext)
                {
                    Subsystem->AddMappingContext(InputMappingContext, 0);
                }
            }
        }
    }
}

void AGameHero::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (CameraSpringArm && MeshComponent)
    {
        CameraSpringArm->SetWorldLocation(MeshComponent->GetComponentLocation());
    }
}

void AGameHero::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (MoveAction)
        {
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGameHero::Move);
        }
        if (LookAction)
        {
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGameHero::Look);
        }
        if (PrimaryAction)
        {
            EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Started, this, &AGameHero::OnPrimaryAction);
        }
        if (SecondaryAction)
        {
            EnhancedInputComponent->BindAction(SecondaryAction, ETriggerEvent::Started, this, &AGameHero::OnSecondaryAction);
        }
    }
}

void AGameHero::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();
    
    if (MovementVector.IsNearlyZero())
        return;

    if (!Controller)
        return;

    const FRotator CameraRotation = CameraSpringArm->GetTargetRotation();
    FVector WorldDirection = GetCameraRelativeMovement(MovementVector, CameraRotation.Yaw);
    
    MoveInDirection(WorldDirection, GetWorld()->GetDeltaSeconds());
	
    if (UActorUnifiedDataComponent* UnifiedComp = GetUnifiedDataComponent())
    {
		UnifiedComp->SyncFromActor();
	}

    LastInputTime = GetWorld()->GetTimeSeconds();
}

void AGameHero::Look(const FInputActionValue& Value)
{
    //FVector2D LookVector = Value.Get<FVector2D>();
    
    //if (LookVector.IsNearlyZero())
    //    return;

    //FRotator NewRotation = GetActorRotation();
    //NewRotation.Yaw += LookVector.X;
    //SetActorRotation(NewRotation);
}

void AGameHero::OnPrimaryAction(const FInputActionValue& Value)
{
    LastInputTime = GetWorld()->GetTimeSeconds();
}

void AGameHero::OnSecondaryAction(const FInputActionValue& Value)
{
    LastInputTime = GetWorld()->GetTimeSeconds();
}

FVector AGameHero::GetCameraRelativeMovement(const FVector2D& InputVector, float InCameraYaw) const
{
	const FRotator YawRotation(0, InCameraYaw, 0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	FVector Movement = ForwardDirection * InputVector.X + RightDirection * InputVector.Y;
	Movement.Z = 0.0f;

	return Movement;
}

void AGameHero::SaveInitialSpawnLocation()
{
    if (SpawnLocation.IsZero())
    {
        SpawnLocation = GetActorLocation();
    }
}

void AGameHero::HandleDeath(const FDamageInfo& KillingBlow)
{

    UE_LOG(LogTemp, Log, TEXT("GameHero died"));

    if (ShouldConfigAutoRespawn())
    {
        FVector RespawnLocation = SpawnLocation;
        APlayerController* PC = Cast<APlayerController>(GetController());
        UWorld* World = GetWorld();
        
        FTimerHandle RespawnTimer;
        World->GetTimerManager().SetTimer(
            RespawnTimer,
            [World, PC, RespawnLocation]()
            {
				UClass* GameHeroClass = AGameHero::StaticClass();
                if (PC)
                {
					if (APawn* Pawn = PC->GetPawn())
					{
						GameHeroClass = Pawn->GetClass();
						PC->UnPossess();
						Pawn->Destroy();
					}
                }
                
                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                
                AGameHero* NewHero = World->SpawnActor<AGameHero>(GameHeroClass, RespawnLocation, FRotator::ZeroRotator, SpawnParams);
                
                if (NewHero && PC)
                {
                    PC->Possess(NewHero);
                    UE_LOG(LogTemp, Log, TEXT("GameHero respawned successfully"));
                }
            },
            GetConfigRespawnDelay(),
            false);
    }
    else
    {
		Super::HandleDeath(KillingBlow);
    }
}

static FAutoConsoleCommand CVarGameHeroKill(
    TEXT("GameHero.Kill"),
    TEXT("Kill the GameHero for testing respawn functionality"),
    FConsoleCommandDelegate::CreateStatic([]()
    {
        UWorld* World = GEngine->GameViewport->GetWorld();
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("GameHero.Kill: World is null"));
            return;
        }

        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (AGameHero* Hero = Cast<AGameHero>(PC->GetPawn()))
            {
                if (UBattleDamageManager* DamageManager = World->GetSubsystem<UBattleDamageManager>())
                {
                    DamageManager->QueueKill(Hero, nullptr, 999999.0f, 999999.0f, EDamageType::Real, false);
                    UE_LOG(LogTemp, Log, TEXT("GameHero.Kill: Queued kill for GameHero"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("GameHero.Kill: Player's pawn is not a GameHero"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("GameHero.Kill: No PlayerController found"));
        }
    })
);
