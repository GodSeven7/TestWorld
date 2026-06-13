#include "GOAPAgentComponent.h"
#include "GOAPManager.h"
#include "GOAPGameTypes.h"
#include "GOAP.h"
#include "CrowdSurroundService.h"

UGOAPAgentComponent::UGOAPAgentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGOAPAgentComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!AssociatedComponent)
    {
        AActor* Owner = GetOwner();
        if (Owner)
        {
            AssociatedComponent = Owner->GetRootComponent();
        }
    }

    RegisterGOAPDefaultActions(GetWorld());

    if (UGOAPManager* Manager = GetWorld()->GetSubsystem<UGOAPManager>())
    {
        Manager->RegisterAgent(this);
    }
}

void UGOAPAgentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetObjectID() >= 0)
    {
        if (UGOAPManager* Manager = GetWorld()->GetSubsystem<UGOAPManager>())
        {
            ICrowdSurroundService* SurroundService = Manager->GetCrowdSurroundService();
            if (SurroundService)
            {
                SurroundService->ClearSurroundState(GetObjectID());
            }
            Manager->UnregisterAgent(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void UGOAPAgentComponent::OnTargetLost()
{
    if (GetObjectID() >= 0)
    {
        if (UGOAPManager* Manager = GetWorld()->GetSubsystem<UGOAPManager>())
        {
            ICrowdSurroundService* SurroundService = Manager->GetCrowdSurroundService();
            if (SurroundService)
            {
                SurroundService->ClearSurroundState(GetObjectID());
            }
        }
    }
}

void UGOAPAgentComponent::SetGoal(const FGOAPWorldState& NewGoal)
{
    CachedGoal = NewGoal;
    bHasCustomGoal = true;

    if (GetObjectID() >= 0)
    {
        if (UGOAPManager* Manager = GetWorld()->GetSubsystem<UGOAPManager>())
        {
            Manager->SetAgentGoal(GetObjectID(), NewGoal);
        }
    }
}

void UGOAPAgentComponent::RequestReplan()
{
    if (GetObjectID() >= 0)
    {
        if (UGOAPManager* Manager = GetWorld()->GetSubsystem<UGOAPManager>())
        {
            Manager->RequestReplan(GetObjectID());
        }
    }
}

const TArray<int32>& UGOAPAgentComponent::GetCurrentPlan() const
{
    static TArray<int32> EmptyPlan;
    if (UGOAPManager* Manager = GetWorld()->GetSubsystem<UGOAPManager>())
    {
        return Manager->GetAgentPlan(GetObjectID());
    }
    return EmptyPlan;
}

int32 UGOAPAgentComponent::GetCurrentActionIndex() const
{
    if (UGOAPManager* Manager = GetWorld()->GetSubsystem<UGOAPManager>())
    {
        return Manager->GetAgentActionIndex(GetObjectID());
    }
    return 0;
}

bool UGOAPAgentComponent::HasValidPlan() const
{
    return GetCurrentPlan().Num() > 0;
}

bool UGOAPAgentComponent::IsExecutingPlan() const
{
    return GetAgentState() == EGOAPAgentState::Executing;
}

float UGOAPAgentComponent::QueryAttackRange() const
{
    if (const UGOAPAgentConfig* Config = GetAgentConfig())
    {
        return Config->AttackRange;
    }
    return 100.0f;
}

void UGOAPAgentComponent::UpdateWorldState(FGOAPWorldState& OutWorldState)
{
    const FVector Location = QueryWorldPosition();
    const UGOAPAgentConfig* Config = GetAgentConfig();
    float AttackRange = QueryAttackRange();
    float AbandonChaseRange = Config ? Config->AbandonChaseRange : 1000.0f;
    float AtHomeDist = Config ? Config->AtHomeDistance : 100.0f;

    OutWorldState.SetState(EGOAPGameStateKey::HasPatrolPath, QueryPatrolPoints().Num() > 0 ? 1 : 0);

    UActorUnifiedDataComponent* NearestEnemyData = QueryNearestEnemy(AbandonChaseRange);
    if (NearestEnemyData)
    {
        FVector TargetPos = NearestEnemyData->GetPosition();
        float Distance = FVector::Dist2D(Location, TargetPos);

        OutWorldState.SetState(EGOAPGameStateKey::HasTarget, 1);
        OutWorldState.SetState(EGOAPGameStateKey::TargetInAttackRange, Distance <= AttackRange ? 1 : 0);
        OutWorldState.SetState(EGOAPGameStateKey::TargetOutOfRange, Distance > AbandonChaseRange ? 1 : 0);
        OutWorldState.SetState(EGOAPGameStateKey::EnemyDead, NearestEnemyData->IsAlive() ? 0 : 1);

        OnTargetFound(NearestEnemyData);
    }
    else
    {
        OutWorldState.SetState(EGOAPGameStateKey::HasTarget, 0);
        OutWorldState.SetState(EGOAPGameStateKey::TargetInAttackRange, 0);
        OutWorldState.SetState(EGOAPGameStateKey::TargetOutOfRange, 1);
        OutWorldState.SetState(EGOAPGameStateKey::EnemyDead, 0);

        OnTargetLost();
    }

    bool bAtHome = FVector::Dist2D(Location, QuerySpawnedPosition()) <= AtHomeDist;
    OutWorldState.SetState(EGOAPGameStateKey::AtHomeLocation, bAtHome ? 1 : 0);

    // ─── CrowdSurround 状态更新 ───
    UGOAPManager* GOAPManager = GetWorld()->GetSubsystem<UGOAPManager>();
    ICrowdSurroundService* SurroundService = GOAPManager ? GOAPManager->GetCrowdSurroundService() : nullptr;

    if (SurroundService)
    {
        bool bHasRequest = SurroundService->IsInSurroundGroup(GetObjectID());
        OutWorldState.SetState(EGOAPGameStateKey::HasSurroundRequest, bHasRequest ? 1 : 0);

        FCrowdSurroundAssignment Assignment;
        bool bHasAssignment = SurroundService->GetSurroundAssignment(GetObjectID(), Assignment) && Assignment.bHasAssignment;
        OutWorldState.SetState(EGOAPGameStateKey::HasSurroundAssignment, bHasAssignment ? 1 : 0);

        bool bAtSurroundPos = false;
        if (bHasAssignment)
        {
            if (Assignment.bIsInPosition)
            {
                bAtSurroundPos = true;
            }
            else
            {
                float ArrivalDist = Config ? Config->ArrivalDistance : 50.0f;
                bAtSurroundPos = FVector::Dist2D(Location, Assignment.DesiredPosition) <= ArrivalDist;
            }
        }
        OutWorldState.SetState(EGOAPGameStateKey::AtSurroundPosition, bAtSurroundPos ? 1 : 0);
    }
    else
    {
        OutWorldState.SetState(EGOAPGameStateKey::HasSurroundRequest, 0);
        OutWorldState.SetState(EGOAPGameStateKey::HasSurroundAssignment, 0);
        OutWorldState.SetState(EGOAPGameStateKey::AtSurroundPosition, 0);
    }
}

TArray<UActorUnifiedDataComponent*> UGOAPAgentComponent::QueryNearbyUnifiedData(float Radius, int32 ExcludeFactionID) const
{
    TArray<UActorUnifiedDataComponent*> Result;

    UGOAPManager* Manager = GetWorld()->GetSubsystem<UGOAPManager>();
    if (!Manager)
    {
        return Result;
    }

    ISpatialQueryService* SQS = Manager->GetSpatialQueryService();
    if (!SQS)
    {
        return Result;
    }

    FSpatialQueryFilter Filter;
    Filter.FactionFilter = ExcludeFactionID;
    Filter.MaxDistance = Radius;

    TArray<AActor*> Actors = SQS->QueryActorsInSphere(QueryWorldPosition(), Radius, Filter);

    for (AActor* Actor : Actors)
    {
        if (Actor == GetOwner())
        {
            continue;
        }
        if (UActorUnifiedDataComponent* UDC = Actor->FindComponentByClass<UActorUnifiedDataComponent>())
        {
            if (UDC->IsAlive())
            {
                Result.Add(UDC);
            }
        }
    }

    return Result;
}

UActorUnifiedDataComponent* UGOAPAgentComponent::QueryNearestEnemy(float Radius) const
{
    TArray<UActorUnifiedDataComponent*> Nearby = QueryNearbyUnifiedData(Radius, QueryFaction());

    UActorUnifiedDataComponent* Nearest = nullptr;
    float NearestDist = Radius;
    FVector MyPos = QueryWorldPosition();

    for (UActorUnifiedDataComponent* UDC : Nearby)
    {
        float Dist = FVector::Dist2D(MyPos, UDC->GetPosition());
        if (Dist < NearestDist)
        {
            NearestDist = Dist;
            Nearest = UDC;
        }
    }

    return Nearest;
}

void UGOAPAgentComponent::SelectGoal(const FGOAPWorldState& CurrentState)
{
    if (CurrentState.GetState(EGOAPGameStateKey::HasTarget) == 1)
    {
        FGOAPWorldState CombatGoal;
        CombatGoal.SetState(EGOAPGameStateKey::EnemyDead, 1);
        SetGoal(CombatGoal);
    }
    else if (CurrentState.GetState(EGOAPGameStateKey::HasPatrolPath) == 1)
    {
        FGOAPWorldState PatrolGoal;
        PatrolGoal.SetState(EGOAPGameStateKey::IsPatrolling, 1);
        SetGoal(PatrolGoal);
    }
    else
    {
        FGOAPWorldState ReturnGoal;
        ReturnGoal.SetState(EGOAPGameStateKey::AtHomeLocation, 1);
        SetGoal(ReturnGoal);
    }
}

void UGOAPAgentComponent::SetPatrolGoal()
{
    FGOAPWorldState Goal;
    Goal.SetState(EGOAPGameStateKey::IsPatrolling, 1);
    SetGoal(Goal);
}

void UGOAPAgentComponent::SetCombatGoal()
{
    FGOAPWorldState Goal;
    Goal.SetState(EGOAPGameStateKey::EnemyDead, 1);
    SetGoal(Goal);
}

FGOAPWorldState UGOAPAgentComponent::GetGoalState() const
{
    return CachedGoal;
}

FVector UGOAPAgentComponent::QueryWorldPosition() const
{
    if (AssociatedComponent)
    {
        return AssociatedComponent->GetComponentLocation();
    }

    AActor* Owner = GetOwner();
    return Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
}

float UGOAPAgentComponent::QueryMoveSpeed() const
{
    if (const UGOAPAgentConfig* Config = GetAgentConfig())
    {
        return Config->MoveSpeed;
    }
    return 400.0f;
}

const TArray<FVector>& UGOAPAgentComponent::QueryPatrolPoints() const
{
    return PatrolPoints;
}

void UGOAPAgentComponent::ApplyWorldPosition(const FVector& Position)
{
    if (AssociatedComponent)
    {
        AssociatedComponent->SetWorldLocation(Position);
        return;
    }

    AActor* Owner = GetOwner();
    if (Owner)
    {
        Owner->SetActorLocation(Position);
    }
}

void UGOAPAgentComponent::ApplyWorldRotation(const FRotator& Rotation)
{
    if (AssociatedComponent)
    {
        AssociatedComponent->SetWorldRotation(Rotation);
        return;
    }

    AActor* Owner = GetOwner();
    if (Owner)
    {
        Owner->SetActorRotation(Rotation);
    }
}

void UGOAPAgentComponent::ApplyForwardVector(const FVector& Forward)
{
    if (AssociatedComponent)
    {
        AssociatedComponent->SetWorldRotation(Forward.Rotation());
        return;
    }

    AActor* Owner = GetOwner();
    if (Owner)
    {
        Owner->SetActorRotation(Forward.Rotation());
    }
}

void UGOAPAgentComponent::ApplyContext(const FGOAPAgentContext& Context, float DeltaTime)
{
    if (!Context.LookDirection.IsNearlyZero())
    {
        ApplyWorldRotation(Context.LookDirection.Rotation());
        ApplyForwardVector(Context.LookDirection.GetSafeNormal());
    }
}
