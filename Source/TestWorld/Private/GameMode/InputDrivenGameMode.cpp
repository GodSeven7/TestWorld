#include "GameMode/InputDrivenGameMode.h"
#include "Subsystems/WorldGridSubsystem.h"
#include "Actors/GameHero.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AInputDrivenGameMode::AInputDrivenGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AInputDrivenGameMode::BeginPlay()
{
    Super::BeginPlay();

    WorldGridSubsystem = GetWorld()->GetSubsystem<UWorldGridSubsystem>();
}

void AInputDrivenGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UpdateGameplaySuspension();
}

void AInputDrivenGameMode::UpdateGameplaySuspension()
{
    if (!WorldGridSubsystem)
    {
        return;
    }

	if (!bGameSuspendEnabled)
	{
		return;
	}

    AGameHero* Hero = Cast<AGameHero>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!Hero)
    {
		UE_LOG(LogTemp, Warning, TEXT("AInputDrivenGameMode can not cast AGameHero"));
        return;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();
    float IdleDuration = CurrentTime - Hero->GetLastInputTime();

    if (!bGameplaySuspended && IdleDuration > (InputIdleThreshold + 0.1f))
    {
        bGameplaySuspended = true;
        WorldGridSubsystem->SetGameplaySuspended(true);
    }
    else if (bGameplaySuspended && IdleDuration <= (InputIdleThreshold + 0.1f))
    {
        bGameplaySuspended = false;
        WorldGridSubsystem->SetGameplaySuspended(false);
    }
}
