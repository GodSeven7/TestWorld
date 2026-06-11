#include "Actors/GameCharacter.h"
#include "CharacterSkelMeshComponent.h"
#include "CharacterAnimData.h"
#include "CharacterAnimManager.h"
#include "Components/SparseGridWithUnifiedDataComponent.h"
#include "Components/CombatWithUnifiedDataComponent.h"
#include "ActorUnifiedDataComponent.h"
#include "CombatWeapon.h"
#include "Subsystems/CombatObjectFactory.h"

AGameCharacter::AGameCharacter()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	MeshRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MeshRootComponent"));
	MeshRootComponent->SetupAttachment(RootComponent);

	MeshRootSubComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MeshRootSubComponent"));
	MeshRootSubComponent->SetupAttachment(MeshRootComponent);
	MeshRootSubComponent->AddLocalRotation(FRotator(0.f, -90.f, 0.f));

    MeshComponent = CreateDefaultSubobject<UCharacterSkelMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(MeshRootSubComponent);

    MeshComponent->SetMobility(EComponentMobility::Movable);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeshComponent->SetGenerateOverlapEvents(false);
}

void AGameCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (USparseGridWithUnifiedDataComponent* GridComp = GetSparseGridComponent())
    {
        GridComp->SetAssociatedComponent(MeshRootComponent);
    }

    if (UActorUnifiedDataComponent* UnifiedComp = GetUnifiedDataComponent())
    {
        UnifiedComp->SetAssociatedComponent(MeshRootComponent);
    }

    if (UCombatWithUnifiedDataComponent* CombatComp = GetCombatComponent())
    {
        // SetWeaponClass 已移除，武器通过 WeaponID 从配置表加载
        // CombatComp->SetWeaponClass(DefaultWeaponClass);
    }

    MeshComponent->InitFromAnimData(AnimDataAsset);
    MeshComponent->SetMeshType(MeshType);
    MeshComponent->PlayAnim();

    if (UCharacterAnimManager* Mgr = GetWorld()->GetSubsystem<UCharacterAnimManager>())
    {
        ManagerID = Mgr->RegisterComponent(MeshComponent);
    }
}

void AGameCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ManagerID != INDEX_NONE)
    {
        if (UCharacterAnimManager* Mgr = GetWorld()->GetSubsystem<UCharacterAnimManager>())
        {
            Mgr->UnregisterComponent(ManagerID);
        }
        ManagerID = INDEX_NONE;
    }

    Super::EndPlay(EndPlayReason);
}

void AGameCharacter::HandleDeath(const FDamageInfo& KillingBlow)
{
    if (MeshComponent)
    {
        MeshComponent->SetDead(true);
        MeshComponent->SetAttacking(false);
    }

    Super::HandleDeath(KillingBlow);
}

void AGameCharacter::OnActorPoolSpawn()
{
    Super::OnActorPoolSpawn();

    if (MeshComponent)
    {
        MeshComponent->SetDead(false);
        MeshComponent->SetAttacking(false);
        MeshComponent->PlayAnim();
    }
}

void AGameCharacter::ResetActorPoolState()
{
    Super::ResetActorPoolState();

    if (MeshComponent)
    {
        MeshComponent->SetDead(false);
        MeshComponent->SetAttacking(false);
    }
}

float AGameCharacter::GetMoveSpeedFromConfig() const
{
	if (UCombatObjectFactory* Factory = GetWorld()->GetSubsystem<UCombatObjectFactory>())
	{
		FCombatObjectConfig OutConfig;
		if (Factory->GetConfig(CharacterTypeID, OutConfig))
		{
			return OutConfig.DefaultMoveSpeed;
		}
	}
	return MoveSpeed;
}

void AGameCharacter::MoveInDirection(const FVector& WorldDirection, float DeltaTime)
{
    FVector Movement = WorldDirection;
    Movement.Z = 0.0f;
	Movement.Normalize();

    if (Movement.IsNearlyZero())
        return;

    //AddActorWorldOffset(Movement * MoveSpeed * DeltaTime, true);

    FRotator NewRotation = Movement.GetSafeNormal().Rotation();

    if (MeshRootComponent)
    {
		MeshRootComponent->AddWorldOffset(Movement * GetMoveSpeedFromConfig() * DeltaTime);
		MeshRootComponent->SetWorldRotation(NewRotation);
    }
}
