#include "Coordinator/PluginCoordinator.h"
#include "Glue/CombatGlue.h"
#include "Glue/GOAPGlue.h"
#include "Glue/ProjectileGlue.h"
#include "Glue/QuestGlue.h"
#include "Glue/CharacterGlue.h"
#include "Glue/GameUIGlue.h"
#include "Glue/SpawnerGlue.h"
#include "Glue/CameraAdvanceGlue.h"
#include "Glue/SpatialGlue.h"
#include "Glue/ObjectPoolGlue.h"
#include "SparseGridManager.h"
#include "FlowFieldManager.h"
#include "SparseGridObject.h"
#include "SparseGridComponent.h"
#include "SparseGridCollision.h"
#include "SparseGridQueryUtils.h"
#include "SparseGridDebug.h"
#include "CrowdSurroundManager.h"
#include "ObjectPoolManager.h"
#include "GOAPManager.h"
#include "BattleDamageManager.h"
#include "CombatManager.h"
#include "ProjectileManager.h"
#include "QuestManager.h"
#include "GameDataSubsystem.h"
#include "Subsystems/SpawnerManager.h"
#include "CharacterAnimManager.h"
#include "CameraShakeManager.h"
#include "ActorUnifiedDataComponent.h"
#include "Components/SparseGridWithUnifiedDataComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<int32> CVarDisableGOAP(
    TEXT("Game.DisableGOAP"),
    0,
    TEXT("Disable GOAP (0=Off, 1=On)"),
    ECVF_Default
);
#endif

void UPluginCoordinator::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    GridManager = GetWorld()->GetSubsystem<USparseGridManager>();
    FlowFieldManager = GetWorld()->GetSubsystem<UFlowFieldManager>();
    GOAPManager = GetWorld()->GetSubsystem<UGOAPManager>();
    BattleDamageManager = GetWorld()->GetSubsystem<UBattleDamageManager>();
    CombatManager = GetWorld()->GetSubsystem<UCombatManager>();
    ProjectileManager = GetWorld()->GetSubsystem<UProjectileManager>();
    QuestManager = GetWorld()->GetSubsystem<UQuestManager>();

    if (UGameInstance* GI = GetWorld()->GetGameInstance())
    {
        GameDataSubsystem = GI->GetSubsystem<UGameDataSubsystem>();
    }

    SpawnerManager = GetWorld()->GetSubsystem<USpawnerManager>();
    CharacterAnimManager = GetWorld()->GetSubsystem<UCharacterAnimManager>();
    CameraShakeManager = GetWorld()->GetSubsystem<UCameraShakeManager>();

    if (GridManager)
    {
        bInitialized = true;

        FSparseGridQueryUtils::SetWorldContext(GetWorld());
    }

    CrowdSurroundManager = GetWorld()->GetSubsystem<UCrowdSurroundManager>();

    EventBus = NewObject<UEventBus>(this);

    CombatGlue = NewObject<UCombatGlue>(this);
    GOAPGlue = NewObject<UGOAPGlue>(this);
    ProjectileGlue = NewObject<UProjectileGlue>(this);
    QuestGlue = NewObject<UQuestGlue>(this);
    CharacterGlue = NewObject<UCharacterGlue>(this);
    GameUIGlue = NewObject<UGameUIGlue>(this);
    SpawnerGlue = NewObject<USpawnerGlue>(this);
    CameraAdvanceGlue = NewObject<UCameraAdvanceGlue>(this);
    SpatialGlue = NewObject<USpatialGlue>(this);
    ObjectPoolGlue = NewObject<UObjectPoolGlue>(this);

    CombatGlue->Bind(CombatManager, GridManager, BattleDamageManager, EventBus);
    GOAPGlue->Bind(GOAPManager, GridManager, CombatManager, CrowdSurroundManager);
    ProjectileGlue->Bind(ProjectileManager, GridManager, BattleDamageManager);
    QuestGlue->Bind(QuestManager, EventBus);
    CharacterGlue->Bind(CharacterAnimManager, EventBus);
    CharacterGlue->SetUnifiedDataComponents(&UnifiedDataComponents);
    GameUIGlue->Bind(GameDataSubsystem, QuestManager, CombatManager, EventBus);
    SpawnerGlue->Bind(SpawnerManager, GridManager, EventBus);
    CameraAdvanceGlue->Bind(CameraShakeManager, EventBus);
    SpatialGlue->Bind(GridManager, FlowFieldManager, CrowdSurroundManager);

    CombatGlue->Initialize(GetWorld());
    GOAPGlue->Initialize(GetWorld());
    ProjectileGlue->Initialize(GetWorld());
    QuestGlue->Initialize(GetWorld());
    CharacterGlue->Initialize(GetWorld());
    GameUIGlue->Initialize(GetWorld());
    SpawnerGlue->Initialize(GetWorld());
    CameraAdvanceGlue->Initialize(GetWorld());
    SpatialGlue->Initialize(GetWorld());

    if (UGameInstance* GI = GetWorld()->GetGameInstance())
    {
        if (UObjectPoolManager* PoolMgr = GI->GetSubsystem<UObjectPoolManager>())
        {
            ObjectPoolGlue->Bind(PoolMgr, GridManager);
            ObjectPoolGlue->Initialize(GetWorld());
        }
    }

    UE_LOG(LogTemp, Log, TEXT("PluginCoordinator initialized"));
}

void UPluginCoordinator::Deinitialize()
{
    if (CombatGlue) { CombatGlue->Shutdown(); }
    if (GOAPGlue) { GOAPGlue->Shutdown(); }
    if (ProjectileGlue) { ProjectileGlue->Shutdown(); }
    if (QuestGlue) { QuestGlue->Shutdown(); }
    if (CharacterGlue) { CharacterGlue->Shutdown(); }
    if (GameUIGlue) { GameUIGlue->Shutdown(); }
    if (SpawnerGlue) { SpawnerGlue->Shutdown(); }
    if (CameraAdvanceGlue) { CameraAdvanceGlue->Shutdown(); }
    if (SpatialGlue) { SpatialGlue->Shutdown(); }
    if (ObjectPoolGlue) { ObjectPoolGlue->Shutdown(); }

    FSparseGridQueryUtils::ClearWorldContext();

    bInitialized = false;
    Super::Deinitialize();
}

void UPluginCoordinator::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    TRACE_CPUPROFILER_EVENT_SCOPE(UPluginCoordinator_Tick);

    if (!bInitialized || !GridManager)
        return;

    // =====================================================================
    // Stage 1: Data Preparation
    // =====================================================================

    {
        SCOPED_NAMED_EVENT(Coordinator_CreateSnapshots, FColor::Cyan);
        for (UActorUnifiedDataComponent* UnifiedComp : UnifiedDataComponents)
        {
            if (UnifiedComp)
            {
                UnifiedComp->CreateSnapshot();
            }
        }
    }

    // =====================================================================
    // Stage 2: Gameplay Logic (suspended when bGameplaySuspended)
    // =====================================================================

#if !UE_BUILD_SHIPPING
    if (CVarDisableGOAP.GetValueOnGameThread() == 0)
#endif
    {
        if (!bGameplaySuspended)
        {
			if (GOAPGlue)
			{
				SCOPED_NAMED_EVENT(Coordinator_GOAP, FColor::Green);
				GOAPGlue->Tick(DeltaTime);
			}

            // Crowd surround update + correction (SpatialGlue owns the full pipeline)
            if (SpatialGlue)
            {
                SCOPED_NAMED_EVENT(Coordinator_CrowdSurround, FColor::Cyan);
                SpatialGlue->TickCrowdSurround(DeltaTime);
            }

            // Write CrowdSurround results to CorrectedDesiredPosition
            if (SpatialGlue)
            {
                SCOPED_NAMED_EVENT(Coordinator_CrowdSurroundCorrection, FColor::Cyan);
                SpatialGlue->TickCrowdSurroundCorrection();
            }

            // Flow field query (extracted from TickSteeringUpdate)
            if (SpatialGlue)
            {
                SCOPED_NAMED_EVENT(Coordinator_FlowFieldQuery, FColor::Magenta);
                SpatialGlue->TickFlowFieldQuery();
            }

            // Steering correction only (renamed from TickSteeringUpdate)
            if (SpatialGlue)
            {
                SCOPED_NAMED_EVENT(Coordinator_SteeringCorrection, FColor::Magenta);
                SpatialGlue->TickSteeringCorrection();
            }
        }
    }

    {
        SCOPED_NAMED_EVENT(Coordinator_SpatialUpdate, FColor::Yellow);
        if (SpatialGlue)
        {
            SpatialGlue->TickSpatialUpdate(DeltaTime);
        }
    }

    if (!bGameplaySuspended)
    {
        {
            SCOPED_NAMED_EVENT(Coordinator_CollisionDetection, FColor::Orange);
            if (SpatialGlue)
            {
                SpatialGlue->TickCollisionDetection(UnifiedDataComponents);
            }
        }

        {
            SCOPED_NAMED_EVENT(Coordinator_NavigationUpdate, FColor::Magenta);
            if (SpatialGlue)
            {
                SpatialGlue->TickNavigationUpdate();
            }
        }

        if (ProjectileGlue)
        {
            SCOPED_NAMED_EVENT(Coordinator_Projectile, FColor::Emerald);
            ProjectileGlue->Tick(DeltaTime);
        }

        if (CombatGlue)
        {
            SCOPED_NAMED_EVENT(Coordinator_Combat, FColor::Red);
            CombatGlue->Tick(DeltaTime);
        }

        if (CombatGlue)
        {
            SCOPED_NAMED_EVENT(Coordinator_DamageProcessing, FColor::Purple);
            CombatGlue->TickDamageProcessing();
        }
    }

    // =====================================================================
    // Stage 3: Data Application
    // =====================================================================

    {
        SCOPED_NAMED_EVENT(Coordinator_ApplyToActors, FColor::Cyan);
        if (bGameplaySuspended)
        {
            for (UActorUnifiedDataComponent* UnifiedComp : UnifiedDataComponents)
            {
                UnifiedComp->ClearMovementVelocity();
            }
        }
        for (UActorUnifiedDataComponent* UnifiedComp : UnifiedDataComponents)
        {
            UnifiedComp->ApplyToActor(DeltaTime);
        }
    }

    if (CharacterGlue)
    {
        SCOPED_NAMED_EVENT(Coordinator_CharacterVisual, FColor::Turquoise);
        CharacterGlue->Tick(DeltaTime);
    }

    if (CameraAdvanceGlue)
    {
        SCOPED_NAMED_EVENT(Coordinator_CameraShake, FColor::Turquoise);
        CameraAdvanceGlue->Tick(DeltaTime);
    }

    // =====================================================================
    // Stage 4: Game Flow
    // =====================================================================

    if (!bGameplaySuspended)
    {
        if (QuestGlue)
        {
            SCOPED_NAMED_EVENT(Coordinator_Quest, FColor::Silver);
            QuestGlue->Tick(DeltaTime);
        }
    }

    if (GameUIGlue)
    {
        GameUIGlue->Tick(DeltaTime);
    }

    if (!bGameplaySuspended)
    {
        if (SpawnerGlue)
        {
            SCOPED_NAMED_EVENT(Coordinator_Spawner, FColor::Turquoise);
            SpawnerGlue->Tick(DeltaTime);
        }
    }

    // Debug visualization
    if (SparseGridDebug::IsDebugEnabled())
    {
        SparseGridDebug::DrawDebug(GetWorld(), GridManager);
    }

    if (SparseGridDebug::IsCollisionDebugEnabled())
    {
        if (UWorld* World = GetWorld())
        {
            TArray<ISparseGridObject*> Objects = GridManager->GetAllObjects();
            for (ISparseGridObject* Object : Objects)
            {
                if (!Object)
                    continue;
                if (!Object->EnableSparseGridCollision())
                    continue;
                FVector Position = Object->GetSparseGridPosition();
                Position.Z = 0.0f;
                float CollisionRadius = Object->GetSparseGridCollisionRadius();
                int32 Layer = Object->GetSparseGridCollisionLayer();
                FColor Color = FColor::MakeRedToGreenColorFromScalar(Layer / 10.0f);
                DrawDebugCircle(World, Position, CollisionRadius, 32, Color, false, -1.0f, SDPG_World, 1.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);
            }

            TArray<FSparseGridCollisionPair> ActiveCollisions = GridManager->GetActiveCollisions();
            for (const FSparseGridCollisionPair& Pair : ActiveCollisions)
            {
                AActor* ActorA = Pair.ActorA.Get();
                AActor* ActorB = Pair.ActorB.Get();
                if (!ActorA || !ActorB)
                    continue;
                USparseGridComponent* CompA = ActorA->FindComponentByClass<USparseGridComponent>();
                USparseGridComponent* CompB = ActorB->FindComponentByClass<USparseGridComponent>();
                if (!CompA || !CompB)
                    continue;
                FVector PosA = CompA->GetSparseGridPosition();
                FVector PosB = CompB->GetSparseGridPosition();
                PosA.Z = 0.0f;
                PosB.Z = 0.0f;
                DrawDebugLine(World, PosA, PosB, FColor::Yellow, false, -1.0f, SDPG_World, 2.0f);
            }
        }
    }

    if (SparseGridDebug::IsRaycastTestEnabled())
    {
        SparseGridDebug::DrawRaycastTest(GetWorld(), GridManager);
    }

    if (SparseGridDebug::IsFlowFieldPathDebugEnabled() || SparseGridDebug::IsFlowFieldDirectionDebugEnabled())
    {
        SparseGridDebug::DrawFlowFieldDebug(GetWorld(), GridManager);
    }
}

TStatId UPluginCoordinator::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UPluginCoordinator, STATGROUP_Tickables);
}

void UPluginCoordinator::SetGameplaySuspended(bool bSuspended)
{
    bGameplaySuspended = bSuspended;

    if (GOAPGlue) bSuspended ? GOAPGlue->Suspend() : GOAPGlue->Resume();
    if (ProjectileGlue) bSuspended ? ProjectileGlue->Suspend() : ProjectileGlue->Resume();
    if (CombatGlue) bSuspended ? CombatGlue->Suspend() : CombatGlue->Resume();
    if (QuestGlue) bSuspended ? QuestGlue->Suspend() : QuestGlue->Resume();
    if (SpawnerGlue) bSuspended ? SpawnerGlue->Suspend() : SpawnerGlue->Resume();
    if (CharacterGlue) bSuspended ? CharacterGlue->Suspend() : CharacterGlue->Resume();
}

int32 UPluginCoordinator::GetTrackedActorCount() const
{
    if (GridManager)
    {
        return GridManager->GetAllObjects().Num();
    }
    return 0;
}

TArray<ISparseGridObject*> UPluginCoordinator::GetAllSparseGridObjects() const
{
    if (GridManager)
    {
        return GridManager->GetAllObjects();
    }
    return {};
}

void UPluginCoordinator::DestroyObject(const FSparseGridHandle& Handle)
{
    if (bInitialized && GridManager)
        GridManager->DestroyObject(Handle);
}

void UPluginCoordinator::UpdateObjectPosition(const FSparseGridHandle& Handle, const FVector& NewPosition)
{
    if (bInitialized && GridManager)
        GridManager->UpdateObjectPosition(Handle, NewPosition);
}

TArray<FSparseGridHandle> UPluginCoordinator::QuerySphere(const FVector& Center, float Radius) const
{
    if (!bInitialized || !GridManager)
        return {};
    return GridManager->QuerySphere(Center, Radius);
}

TArray<ISparseGridObject*> UPluginCoordinator::QuerySparseGridObjectsInSphere(const FVector& Center, float Radius) const
{
    TArray<ISparseGridObject*> Results;
    if (!bInitialized || !GridManager)
        return Results;

    TArray<FSparseGridHandle> Handles = GridManager->QuerySphere(Center, Radius);
    for (const FSparseGridHandle& Handle : Handles)
    {
        if (ISparseGridObject* Object = GridManager->GetObjectInterface(Handle))
        {
            if (FVector::DistSquared(Center, Object->GetSparseGridPosition()) <= Radius * Radius)
            {
                Results.Add(Object);
            }
        }
    }
    return Results;
}

AActor* UPluginCoordinator::FindNearestActor(const FVector& Position, float MaxRadius) const
{
    TArray<ISparseGridObject*> Objects = QuerySparseGridObjectsInSphere(Position, MaxRadius);
    if (Objects.Num() == 0)
        return nullptr;

    AActor* Nearest = nullptr;
    float MinDist = MAX_FLT;
    for (ISparseGridObject* Object : Objects)
    {
        AActor* Actor = Cast<AActor>(Object);
        if (!Actor)
            continue;

        float Dist = FVector::DistSquared(Position, Actor->GetActorLocation());
        if (Dist < MinDist)
        {
            MinDist = Dist;
            Nearest = Actor;
        }
    }
    return Nearest;
}

void UPluginCoordinator::ClearAll()
{
    if (GridManager)
    {
        TArray<ISparseGridObject*> AllObjects = GridManager->GetAllObjects();
        for (ISparseGridObject* Object : AllObjects)
        {
            if (AActor* Actor = Cast<AActor>(Object))
            {
                GridManager->NotifyActorRemoved(Actor);
            }
        }
    }
}

void UPluginCoordinator::SetCollisionEnabled(bool bEnabled)
{
    if (GridManager) GridManager->SetCollisionEnabled(bEnabled);
}

bool UPluginCoordinator::IsCollisionEnabled() const
{
    return GridManager ? GridManager->IsCollisionEnabled() : false;
}

void UPluginCoordinator::SetCollisionMatrix(const FSparseGridCollisionMatrix& InMatrix)
{
    if (GridManager) GridManager->SetCollisionMatrix(InMatrix);
}

FSparseGridCollisionMatrix UPluginCoordinator::GetCollisionMatrix() const
{
    return GridManager ? GridManager->GetCollisionMatrix() : FSparseGridCollisionMatrix();
}

FSparseGridCollisionStats UPluginCoordinator::GetCollisionStats() const
{
    return GridManager ? GridManager->GetCollisionStats() : FSparseGridCollisionStats();
}

int32 UPluginCoordinator::GetActiveCollisionCount() const
{
    return GridManager ? GridManager->GetActiveCollisionCount() : 0;
}

void UPluginCoordinator::RegisterUnifiedDataComponent(UActorUnifiedDataComponent* Component)
{
    if (Component && !UnifiedDataComponents.Contains(Component))
    {
        UnifiedDataComponents.Add(Component);
        // Maintain ObjectID mapping for O(1) lookup
        int32 ObjID = Component->GetObjectID();
        if (ObjID != INDEX_NONE)
        {
            ObjectIDToDataMap.Add(ObjID, Component);
        }
    }
}

void UPluginCoordinator::UnregisterUnifiedDataComponent(UActorUnifiedDataComponent* Component)
{
    if (Component)
    {
        // Remove from ObjectID mapping
        int32 ObjID = Component->GetObjectID();
        if (ObjID != INDEX_NONE)
        {
            ObjectIDToDataMap.Remove(ObjID);
        }
        UnifiedDataComponents.RemoveSwap(Component);
    }
}

UActorUnifiedDataComponent* UPluginCoordinator::GetObjectByID(int32 ObjectID) const
{
    const TObjectPtr<UActorUnifiedDataComponent>* Found = ObjectIDToDataMap.Find(ObjectID);
    return Found ? *Found : nullptr;
}

void UPluginCoordinator::NotifyFactionChanged(UActorUnifiedDataComponent* Component, int32 OldFaction, int32 NewFaction)
{
    if (SpatialGlue)
    {
        SpatialGlue->NotifyFactionChanged(Component, OldFaction, NewFaction);
    }

    // 发布 EventBus 事件（供其他 Glue 订阅，可扩展）
    if (EventBus)
    {
        FFactionChangedEvent Event;
        Event.ChangedActor = Component ? Component->GetOwner() : nullptr;
        Event.OldFaction = OldFaction;
        Event.NewFaction = NewFaction;
        EventBus->OnFactionChanged.Broadcast(Event);
    }
}
