#include "Glue/CameraAdvanceGlue.h"
#include "CameraShakeManager.h"
#include "Actors/GameHero.h"
#include "CineCameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

static bool bDisableCameraShake = true;
FAutoConsoleVariableRef CVarDisableCameraShake(
	TEXT("Disable.CameraShake"),
	bDisableCameraShake,
	TEXT(""));

static int32 CameraShakeTestTrigger = 0;
FAutoConsoleVariableRef CVarCameraShakeTestTrigger(
	TEXT("CameraShake.Test"),
	CameraShakeTestTrigger,
	TEXT("Set to 1 to trigger a test shake"));

static float CameraShakeTestDuration = 0.3f;
FAutoConsoleVariableRef CVarCameraShakeTestDuration(
	TEXT("CameraShake.TestDuration"),
	CameraShakeTestDuration,
	TEXT("Duration in seconds for CameraShake.Test (default 0.3)"));

static float CameraShakeTestScale = 1.0f;
FAutoConsoleVariableRef CVarCameraShakeTestScale(
	TEXT("CameraShake.TestScale"),
	CameraShakeTestScale,
	TEXT("Scale multiplier for CameraShake.Test (default 1.0)"));

void UCameraAdvanceGlue::Initialize(UWorld* World)
{
}

void UCameraAdvanceGlue::Shutdown()
{
    if (EventBus)
    {
        EventBus->OnDamageApplied.RemoveDynamic(this, &UCameraAdvanceGlue::OnDamageApplied);
        EventBus->OnCombatantDeath.RemoveDynamic(this, &UCameraAdvanceGlue::OnCombatantDeath);
    }
}

void UCameraAdvanceGlue::Bind(
    UCameraShakeManager* InShakeManager,
    UEventBus* InEventBus)
{
    ShakeManager = InShakeManager;
    EventBus = InEventBus;

    if (EventBus)
    {
        EventBus->OnDamageApplied.AddDynamic(this, &UCameraAdvanceGlue::OnDamageApplied);
        EventBus->OnCombatantDeath.AddDynamic(this, &UCameraAdvanceGlue::OnCombatantDeath);
    }
}

void UCameraAdvanceGlue::Tick(float DeltaTime)
{
    if (!ShakeManager || bDisableCameraShake)
        return;

    // Debug: 检测测试触发
    if (CameraShakeTestTrigger != 0)
    {
        ShakeManager->RequestShake(CameraShakeTestDuration, CameraShakeTestScale);
        CameraShakeTestTrigger = 0;
    }

    if (!bSuspended)
    {
        ShakeManager->Tick(DeltaTime);
    }

    ApplyShakeOffsetToCamera();
}

void UCameraAdvanceGlue::OnDamageApplied(const FDamageAppliedEvent& Event)
{
    if (bSuspended || !ShakeManager)
        return;

    AActor* Target = Event.Target.Get();
    if (!Target)
        return;

    UWorld* World = GetWorld();
    if (!World)
        return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
        return;

    APawn* PlayerPawn = PC->GetPawn();

    if (Target == PlayerPawn)
    {
        ShakeManager->RequestShake(0.2f, 0.8f);
    }
}

void UCameraAdvanceGlue::OnCombatantDeath(const FCombatantDeathEvent& Event)
{
    if (bSuspended || !ShakeManager)
        return;

    UWorld* World = GetWorld();
    if (!World)
        return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
        return;

    APawn* PlayerPawn = PC->GetPawn();
    AActor* Killer = Event.Killer.Get();

    if (Killer == PlayerPawn)
    {
        ShakeManager->RequestShake(0.3f, 1.5f);
    }
}

void UCameraAdvanceGlue::ApplyShakeOffsetToCamera()
{
    if (!ShakeManager)
        return;

    FGameCameraShakeOffset Offset = ShakeManager->GetCurrentOffset();
    if (Offset.LocationOffset.IsNearlyZero() && Offset.RotationOffset.IsNearlyZero())
        return;

    UWorld* World = GetWorld();
    if (!World)
        return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
        return;

    APawn* PlayerPawn = PC->GetPawn();
    if (!PlayerPawn)
        return;

    AGameHero* Hero = Cast<AGameHero>(PlayerPawn);
    if (!Hero)
        return;

	UCineCameraComponent* CameraComp = Hero->GetCameraComponent();
    if (!CameraComp)
        return;

	CameraComp->SetRelativeLocation(Offset.LocationOffset);
    //SpringArm->SetRelativeRotation(Offset.RotationOffset);
}
