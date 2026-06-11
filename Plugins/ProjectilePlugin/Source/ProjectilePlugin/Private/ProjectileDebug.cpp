#include "ProjectileDebug.h"
#include "ProjectileManager.h"
#include "ProjectilePlugin.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

TArray<IConsoleObject*> FProjectileDebug::RegisteredConsoleCommands;

void FProjectileDebug::Initialize()
{
    RegisterConsoleCommands();
}

void FProjectileDebug::Deinitialize()
{
    UnregisterConsoleCommands();
}

void FProjectileDebug::RegisterConsoleCommands()
{
    RegisteredConsoleCommands.Empty();

    auto FireCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("Projectile.Fire"),
        TEXT("Fire a single projectile from the player camera.\nUsage: Projectile.Fire"),
        FConsoleCommandDelegate::CreateLambda([]()
        {
            UWorld* World = GWorld;
            if (!World) return;

            UProjectileManager* Manager = World->GetSubsystem<UProjectileManager>();
            if (!Manager)
            {
                UE_LOG(LogProjectilePlugin, Warning, TEXT("ProjectileManager not found"));
                return;
            }

            APlayerController* PC = World->GetFirstPlayerController();
            if (!PC)
            {
                UE_LOG(LogProjectilePlugin, Warning, TEXT("No player controller found"));
                return;
            }

            FVector Origin = PC->GetPawn()->GetActorLocation() + FVector::UpVector * 100.0f;
            FRotator Rotation = PC->GetPawn()->GetActorRotation();

            FireProjectileAtDirection(Manager, Origin, Rotation);
        }),
        ECVF_Default
    );
    RegisteredConsoleCommands.Add(FireCommand);

    auto FireMultipleCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("Projectile.FireMultiple"),
        TEXT("Fire multiple projectiles in a spread pattern.\nUsage: Projectile.FireMultiple <Count> [SpreadAngle]\nExample: Projectile.FireMultiple 10 15.0"),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
        {
            UWorld* World = GWorld;
            if (!World) return;

            UProjectileManager* Manager = World->GetSubsystem<UProjectileManager>();
            if (!Manager)
            {
                UE_LOG(LogProjectilePlugin, Warning, TEXT("ProjectileManager not found"));
                return;
            }

            int32 Count = 1;
            float SpreadAngle = 10.0f;

            if (Args.Num() >= 1)
            {
                Count = FCString::Atoi(*Args[0]);
                Count = FMath::Clamp(Count, 1, 100);
            }

            if (Args.Num() >= 2)
            {
                SpreadAngle = FCString::Atof(*Args[1]);
                SpreadAngle = FMath::Clamp(SpreadAngle, 0.0f, 180.0f);
            }

            FireTestProjectiles(Manager, Count, SpreadAngle);
        }),
        ECVF_Default
    );
    RegisteredConsoleCommands.Add(FireMultipleCommand);

    auto FireBurstCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("Projectile.FireBurst"),
        TEXT("Fire a burst of projectiles rapidly.\nUsage: Projectile.FireBurst <Count>\nExample: Projectile.FireBurst 50"),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
        {
            UWorld* World = GWorld;
            if (!World) return;

            UProjectileManager* Manager = World->GetSubsystem<UProjectileManager>();
            if (!Manager)
            {
                UE_LOG(LogProjectilePlugin, Warning, TEXT("ProjectileManager not found"));
                return;
            }

            APlayerController* PC = World->GetFirstPlayerController();
            if (!PC) return;

            APlayerCameraManager* CameraManager = PC->PlayerCameraManager;
            if (!CameraManager) return;

            int32 Count = 50;
            if (Args.Num() >= 1)
            {
                Count = FCString::Atoi(*Args[0]);
                Count = FMath::Clamp(Count, 1, 500);
            }

            FVector Origin = CameraManager->GetCameraLocation();
            FRotator BaseRotation = CameraManager->GetCameraRotation();

            for (int32 i = 0; i < Count; i++)
            {
                float YawOffset = FMath::FRandRange(-5.0f, 5.0f);
                float PitchOffset = FMath::FRandRange(-5.0f, 5.0f);

                FRotator Rot = BaseRotation;
                Rot.Yaw += YawOffset;
                Rot.Pitch += PitchOffset;

                FireProjectileAtDirection(Manager, Origin, Rot);
            }

            UE_LOG(LogProjectilePlugin, Log, TEXT("Fired burst of %d projectiles"), Count);
        }),
        ECVF_Default
    );
    RegisteredConsoleCommands.Add(FireBurstCommand);

    auto ClearCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("Projectile.Clear"),
        TEXT("Clear all active projectiles."),
        FConsoleCommandDelegate::CreateLambda([]()
        {
            UWorld* World = GWorld;
            if (!World) return;

            UProjectileManager* Manager = World->GetSubsystem<UProjectileManager>();
            if (!Manager)
            {
                UE_LOG(LogProjectilePlugin, Warning, TEXT("ProjectileManager not found"));
                return;
            }

            Manager->ClearAllProjectiles();
            UE_LOG(LogProjectilePlugin, Log, TEXT("All projectiles cleared"));
        }),
        ECVF_Default
    );
    RegisteredConsoleCommands.Add(ClearCommand);

    auto StatsCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("Projectile.Stats"),
        TEXT("Show projectile system statistics."),
        FConsoleCommandDelegate::CreateLambda([]()
        {
            UWorld* World = GWorld;
            if (!World) return;

            UProjectileManager* Manager = World->GetSubsystem<UProjectileManager>();
            if (!Manager)
            {
                UE_LOG(LogProjectilePlugin, Warning, TEXT("ProjectileManager not found"));
                return;
            }

            UE_LOG(LogProjectilePlugin, Log, TEXT("=== Projectile System Stats ==="));
            UE_LOG(LogProjectilePlugin, Log, TEXT("Active Projectiles: %d"), Manager->GetActiveProjectileCount());
            UE_LOG(LogProjectilePlugin, Log, TEXT("Total Capacity: %d"), Manager->GetTotalProjectileCount());
        }),
        ECVF_Default
    );
    RegisteredConsoleCommands.Add(StatsCommand);

    auto SetGroundCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("Projectile.SetGroundHeight"),
        TEXT("Set the default ground height for projectiles.\nUsage: Projectile.SetGroundHeight <Height>\nExample: Projectile.SetGroundHeight 0.0"),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
        {
            UWorld* World = GWorld;
            if (!World) return;

            UProjectileManager* Manager = World->GetSubsystem<UProjectileManager>();
            if (!Manager)
            {
                UE_LOG(LogProjectilePlugin, Warning, TEXT("ProjectileManager not found"));
                return;
            }

            if (Args.Num() < 1)
            {
                UE_LOG(LogProjectilePlugin, Warning, TEXT("Usage: Projectile.SetGroundHeight <Height>"));
                return;
            }

            float Height = FCString::Atof(*Args[0]);
            Manager->SetGroundHeight(Height);
            UE_LOG(LogProjectilePlugin, Log, TEXT("Ground height set to: %.2f"), Height);
        }),
        ECVF_Default
    );
    RegisteredConsoleCommands.Add(SetGroundCommand);

    auto DrawTrajectoryCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("Projectile.DrawTrajectory"),
        TEXT("Toggle drawing trajectory for all active projectiles.\nUsage: Projectile.DrawTrajectory [0|1]\n0=Off, 1=On, no arg=Toggle"),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
        {
            UWorld* World = GWorld;
            if (!World) return;

            UProjectileManager* Manager = World->GetSubsystem<UProjectileManager>();
            if (!Manager)
            {
                UE_LOG(LogProjectilePlugin, Warning, TEXT("ProjectileManager not found"));
                return;
            }

            bool bEnabled = false;
            if (Args.Num() == 0)
            {
                bEnabled = !Manager->IsDrawTrajectoryDebugEnabled();
            }
            else
            {
                bEnabled = (Args[0] == TEXT("1"));
            }

            Manager->SetDrawTrajectoryDebug(bEnabled);
            UE_LOG(LogProjectilePlugin, Log, TEXT("Trajectory drawing: %s"), 
                bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
        }),
        ECVF_Default
    );
    RegisteredConsoleCommands.Add(DrawTrajectoryCommand);
}

void FProjectileDebug::UnregisterConsoleCommands()
{
    for (IConsoleObject* Cmd : RegisteredConsoleCommands)
    {
        IConsoleManager::Get().UnregisterConsoleObject(Cmd);
    }
    RegisteredConsoleCommands.Empty();
}

void FProjectileDebug::FireTestProjectiles(UProjectileManager* Manager, int32 Count, float SpreadAngle)
{
    if (!Manager) return;

    UWorld* World = Manager->GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogProjectilePlugin, Warning, TEXT("No player controller found"));
        return;
    }

	FVector Origin = PC->GetPawn()->GetActorLocation() + FVector::UpVector * 100.0f;
	FRotator Rotation = PC->GetPawn()->GetActorRotation();

    float HalfSpread = SpreadAngle * 0.5f;

    for (int32 i = 0; i < Count; i++)
    {
        float YawOffset = FMath::FRandRange(-HalfSpread, HalfSpread);
        float PitchOffset = FMath::FRandRange(-HalfSpread, HalfSpread);

        FRotator Rot = Rotation;
        Rot.Yaw += YawOffset;
        Rot.Pitch += PitchOffset;

        FireProjectileAtDirection(Manager, Origin, Rot);
    }

    UE_LOG(LogProjectilePlugin, Log, TEXT("Fired %d projectiles with %.1f degree spread"), Count, SpreadAngle);
}

void FProjectileDebug::FireProjectileAtDirection(UProjectileManager* Manager, const FVector& Origin, const FRotator& Rotation)
{
    if (!Manager) return;

    FProjectileSpawnParams Params;
    Params.Origin = Origin;
    
    FVector Direction = Rotation.Vector();
    float TargetDistance = 2000.0f;
    Params.TargetLocation = Origin + Direction * TargetDistance;
    
    Params.Config.Speed = 3000.0f;
    Params.Config.Height = 200.0f;
    Params.Config.Damage = 10.0f;
    Params.Config.CollisionRadius = 15.0f;
    Params.Config.MaxLifetime = 10.0f;
    Params.Config.Faction = 0;
    Params.Config.CollisionHeightThreshold = 100.0f;

    int32 ProjectileID = Manager->SpawnProjectile(Params);

    if (ProjectileID < 0)
    {
        UE_LOG(LogProjectilePlugin, Warning, TEXT("Failed to spawn projectile"));
    }
}
