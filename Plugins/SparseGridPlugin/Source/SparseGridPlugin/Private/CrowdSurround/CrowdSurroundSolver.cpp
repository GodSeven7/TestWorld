#include "CrowdSurroundManager.h"
#include "CrowdSurroundUtilities.h"
#include "SparseGridComponent.h"
#include "SparseGridManager.h"
#include "SparseGridConfig.h"

namespace
{
    struct FCrowdPointCandidate
    {
        FVector Position = FVector::ZeroVector;
        float Cost = TNumericLimits<float>::Max();
    };

    bool IsAttackAnchorOccupied(const FCrowdAttackAnchor& Anchor)
    {
        return Anchor.OwnerObjectID != INDEX_NONE
            && Anchor.State == ECrowdAttackAnchorState::Occupied;
    }

    FVector GetDirectionFromTarget(const FCrowdSurroundGroup& Group, const FVector& Position)
    {
        const FVector Direction = (Position - Group.AnchorPosition).GetSafeNormal2D();
        return Direction.IsNearlyZero() ? FVector::ForwardVector : Direction;
    }

    float GetCandidatePhysicalRadius(const FCrowdSurroundCandidate& Candidate)
    {
        return FMath::Max(1.0f, Candidate.CollisionRadius);
    }

    float GetAttackContactRadius(const FCrowdSurroundGroup& Group, const FCrowdSurroundCandidate& Candidate)
    {
        return Group.TargetRadius + GetCandidatePhysicalRadius(Candidate);
    }

    float GetSeparationDistance(const FCrowdSurroundCandidate& Candidate, const FCrowdSurroundOccupiedPoint& OccupiedPoint)
    {
        return GetCandidatePhysicalRadius(Candidate)
            + FMath::Max(1.0f, OccupiedPoint.Radius)
            + FMath::Max(Candidate.ContactPadding, OccupiedPoint.ContactPadding);
    }

    bool DoesOverlapOccupiedPoint(const FVector& Position, const FCrowdSurroundCandidate& Candidate, const FCrowdSurroundOccupiedPoint& OccupiedPoint)
    {
        constexpr float OverlapTolerance = 1.0f;
        return FVector::Dist2D(Position, OccupiedPoint.Position) < GetSeparationDistance(Candidate, OccupiedPoint) + OverlapTolerance;
    }

    bool FindCircleIntersections2D(const FVector& FirstCenter, float FirstRadius, const FVector& SecondCenter, float SecondRadius, FVector& OutA, FVector& OutB)
    {
        const FVector CenterDelta = SecondCenter - FirstCenter;
        const float Distance = CenterDelta.Size2D();
        if (Distance <= KINDA_SMALL_NUMBER)
        {
            return false;
        }

        if (Distance > FirstRadius + SecondRadius + KINDA_SMALL_NUMBER)
        {
            return false;
        }

        if (Distance < FMath::Abs(FirstRadius - SecondRadius) - KINDA_SMALL_NUMBER)
        {
            return false;
        }

        const float Along = (FMath::Square(FirstRadius) - FMath::Square(SecondRadius) + FMath::Square(Distance)) / (2.0f * Distance);
        const float HeightSquared = FMath::Square(FirstRadius) - FMath::Square(Along);
        if (HeightSquared < -KINDA_SMALL_NUMBER)
        {
            return false;
        }

        const float Height = FMath::Sqrt(FMath::Max(0.0f, HeightSquared));
        const FVector Axis = CenterDelta.GetSafeNormal2D();
        const FVector BasePoint = FirstCenter + Axis * Along;
        const FVector Perpendicular(-Axis.Y, Axis.X, 0.0f);

        OutA = BasePoint + Perpendicular * Height;
        OutB = BasePoint - Perpendicular * Height;
        OutA.Z = FirstCenter.Z;
        OutB.Z = FirstCenter.Z;
        return true;
    }

    float GetAngularCostFromDirection(const FCrowdSurroundGroup& Group, const FVector& Position, const FVector& PreferredDirection)
    {
        const FVector CandidateDirection = GetDirectionFromTarget(Group, Position);
        const float Dot = FMath::Clamp(FVector::DotProduct(CandidateDirection, PreferredDirection), -1.0f, 1.0f);
        return FMath::Acos(Dot);
    }

    float GetReservationTimeout(const USparseGridConfig* Config)
    {
        return Config ? Config->GetCrowdReservationTimeout() : 0.8f;
    }

    float GetReachTolerance(const USparseGridConfig* Config)
    {
        const float ConfigReachTolerance = Config ? Config->GetCrowdReachTolerance() : 40.0f;
        const float StopRadius = Config ? Config->GetArrivalStopRadius() : 40.0f;
        return FMath::Max(ConfigReachTolerance, StopRadius);
    }
}

// ---------------------------------------------------------------------------
// BuildCandidatesForGroup: construct per-agent candidate data from AgentIntents
// ---------------------------------------------------------------------------
TMap<int32, FCrowdSurroundCandidate> UCrowdSurroundManager::BuildCandidatesForGroup(const FCrowdSurroundGroup& Group) const
{
    TMap<int32, FCrowdSurroundCandidate> Candidates;

    for (const auto& IntentPair : AgentIntents)
    {
        if (IntentPair.Value.TargetObjectID != Group.TargetObjectID)
        {
            continue;
        }

        const int32 ObjectID = IntentPair.Key;
        const FCrowdSurroundAgentIntent& Intent = IntentPair.Value;

        FCrowdSurroundCandidate Candidate;
        Candidate.ObjectID = ObjectID;
        Candidate.Position = GetAgentPosition(ObjectID, Group);
        Candidate.DistanceToTarget = FVector::Dist2D(Candidate.Position, Group.AnchorPosition);

        // Derive attack parameters from intent + config
        const float AttackRange = GetAgentAttackRange(ObjectID);
        const float CollisionRadius = GetAgentCollisionRadius(ObjectID);
        const float ContactPadding = GetAgentContactPadding(ObjectID);
        const float Spacing = FMath::Max(CollisionRadius * 2.0f + ContactPadding, Config ? Config->GetMinCrowdSpacing() : 80.0f);

        Candidate.AttackRange = AttackRange;
        Candidate.CollisionRadius = CollisionRadius;
        Candidate.ContactPadding = ContactPadding;
        Candidate.Spacing = Spacing;
        Candidate.RequestTime = Intent.RequestTime;

        // Compute attack band from attack range + target radius
        const float ContactRadius = Group.TargetRadius + CollisionRadius;
        Candidate.BandMin = FMath::Clamp(ContactRadius, 0.0f, AttackRange);
        Candidate.BandMax = FMath::Max(0.0f, AttackRange);

        const float InnerBuffer = FMath::Max(Config ? Config->GetAttackBandInnerBuffer() : 30.0f, CollisionRadius * 0.5f);
        Candidate.DesiredAttackRadius = AttackRange > KINDA_SMALL_NUMBER
            ? FMath::Clamp(AttackRange - InnerBuffer, Candidate.BandMin, AttackRange)
            : 0.0f;

        Candidates.Add(ObjectID, Candidate);
    }

    return Candidates;
}

// ---------------------------------------------------------------------------
// GetAgentsForGroup: collect all ObjectIDs whose AgentIntent targets the given group
// ---------------------------------------------------------------------------
TArray<int32> UCrowdSurroundManager::GetAgentsForGroup(int32 TargetObjectID) const
{
    TArray<int32> Agents;
    for (const auto& Pair : AgentIntents)
    {
        if (Pair.Value.TargetObjectID == TargetObjectID)
        {
            Agents.Add(Pair.Key);
        }
    }
    return Agents;
}

// ---------------------------------------------------------------------------
// GetAgentPosition: resolve agent world position via SparseGridComponent
// ---------------------------------------------------------------------------
FVector UCrowdSurroundManager::GetAgentPosition(int32 ObjectID, const FCrowdSurroundGroup& Group) const
{
    // Try to find the agent's SparseGridComponent through the intent's target context
    // The agent position is resolved from the SparseGrid system
    // Fallback: use the group anchor position if the agent cannot be resolved
    if (USparseGridComponent* TargetComponent = Group.TargetComponent.Get())
    {
        // Query the SparseGrid manager for the agent's position by ObjectID
        // This replaces the old Participant.AgentComponent path
        if (UWorld* World = GetWorld())
        {
            if (USparseGridManager* GridManager = World->GetSubsystem<USparseGridManager>())
            {
                if (IGridObjectInfo* GridObject = GridManager->GetObjectByID(ObjectID))
                {
                    return CrowdSurroundUtilities::FlattenPosition(GridObject->GetGridPosition());
                }
            }
        }
    }

    return Group.AnchorPosition;
}

// ---------------------------------------------------------------------------
// GetAgentOccupancyRadius: collision radius for occupied-point computation
// ---------------------------------------------------------------------------
float UCrowdSurroundManager::GetAgentOccupancyRadius(int32 ObjectID) const
{
    const FCrowdSurroundAgentIntent* Intent = AgentIntents.Find(ObjectID);
    if (Intent && Intent->CollisionRadius > KINDA_SMALL_NUMBER)
    {
        return Intent->CollisionRadius;
    }
    return Config ? Config->BaseRadius : 50.0f;
}

// ---------------------------------------------------------------------------
// GetAgentContactPadding: contact padding for occupied-point computation
// ---------------------------------------------------------------------------
float UCrowdSurroundManager::GetAgentContactPadding(int32 ObjectID) const
{
    (void)ObjectID;
    return Config ? Config->GetCrowdContactPadding() : 10.0f;
}

// ---------------------------------------------------------------------------
// GetAgentCollisionRadius: collision radius from intent or config fallback
// ---------------------------------------------------------------------------
float UCrowdSurroundManager::GetAgentCollisionRadius(int32 ObjectID) const
{
    const FCrowdSurroundAgentIntent* Intent = AgentIntents.Find(ObjectID);
    if (Intent && Intent->CollisionRadius > KINDA_SMALL_NUMBER)
    {
        return Intent->CollisionRadius;
    }
    return Config ? Config->BaseRadius : 50.0f;
}

// ---------------------------------------------------------------------------
// GetAgentAttackRange: attack range from intent or config fallback
// ---------------------------------------------------------------------------
float UCrowdSurroundManager::GetAgentAttackRange(int32 ObjectID) const
{
    const FCrowdSurroundAgentIntent* Intent = AgentIntents.Find(ObjectID);
    if (Intent && Intent->AttackRange > KINDA_SMALL_NUMBER)
    {
        return Intent->AttackRange;
    }
    return Config ? Config->GetAttackRange() : 120.0f;
}

// ---------------------------------------------------------------------------
// ComputeGroupMetrics: derive group-level metrics from candidate set
// ---------------------------------------------------------------------------
void UCrowdSurroundManager::ComputeGroupMetrics(FCrowdSurroundGroup& Group, const TMap<int32, FCrowdSurroundCandidate>& Candidates) const
{
    Group.AttackBandMin = 0.0f;
    Group.AttackBandMax = 0.0f;
    Group.AverageSpacing = Config ? Config->GetMinCrowdSpacing() : 80.0f;

    float SpacingSum = 0.0f;
    int32 SpacingCount = 0;
    float CapacityRadius = 0.0f;
    int32 CapacityRadiusCount = 0;

    for (const auto& Pair : Candidates)
    {
        const FCrowdSurroundCandidate& Candidate = Pair.Value;

        Group.AttackBandMin = Group.AttackBandMin <= KINDA_SMALL_NUMBER ? Candidate.BandMin : FMath::Min(Group.AttackBandMin, Candidate.BandMin);
        Group.AttackBandMax = FMath::Max(Group.AttackBandMax, FMath::Max(Candidate.BandMin, Candidate.BandMax));

        if (Candidate.DesiredAttackRadius > KINDA_SMALL_NUMBER)
        {
            CapacityRadius = CapacityRadiusCount == 0 ? Candidate.DesiredAttackRadius : FMath::Min(CapacityRadius, Candidate.DesiredAttackRadius);
            ++CapacityRadiusCount;
        }

        SpacingSum += Candidate.Spacing;
        ++SpacingCount;
    }

    if (SpacingCount > 0)
    {
        Group.AverageSpacing = FMath::Max(Group.AverageSpacing, SpacingSum / static_cast<float>(SpacingCount));
    }

    if (Group.AttackBandMin <= KINDA_SMALL_NUMBER)
    {
        Group.AttackBandMin = Group.TargetRadius + Group.AverageSpacing * 0.5f;
    }

    if (Group.AttackBandMax <= KINDA_SMALL_NUMBER)
    {
        Group.AttackBandMax = Group.TargetRadius + (Config ? Config->GetAttackRange() : 120.0f);
    }

    const float EffectiveCapacityRadius = CapacityRadiusCount > 0 ? CapacityRadius : Group.AttackBandMax;
    const int32 NaturalAttackCapacity = CalculateCircularCapacity(EffectiveCapacityRadius, Group.AverageSpacing);
    const int32 ConfiguredAttackCapacity = Config ? FMath::Max(0, Config->MaxAttackersPerTarget) : 0;
    const int32 RequestedCapacity = ConfiguredAttackCapacity > 0
        ? FMath::Min(NaturalAttackCapacity, ConfiguredAttackCapacity)
        : NaturalAttackCapacity;
    Group.AttackCapacity = FMath::Clamp(RequestedCapacity, 0, Candidates.Num());
}

// ---------------------------------------------------------------------------
// CollectOccupiedPoints: gather all occupied attack anchors and wait points
// ---------------------------------------------------------------------------
void UCrowdSurroundManager::CollectOccupiedPoints(const FCrowdSurroundGroup& Group, TArray<FCrowdSurroundOccupiedPoint>& OutOccupiedPoints) const
{
    OutOccupiedPoints.Reset();

    for (const FCrowdAttackAnchor& Anchor : Group.AttackAnchors)
    {
        if (!IsAttackAnchorOccupied(Anchor))
        {
            continue;
        }

        FCrowdSurroundOccupiedPoint OccupiedPoint;
        OccupiedPoint.OwnerObjectID = Anchor.OwnerObjectID;
        OccupiedPoint.AnchorID = Anchor.AnchorID;
        OccupiedPoint.Position = Anchor.Position;
        OccupiedPoint.Radius = GetAgentOccupancyRadius(Anchor.OwnerObjectID);
        OccupiedPoint.ContactPadding = GetAgentContactPadding(Anchor.OwnerObjectID);
        OccupiedPoint.bAttackPoint = true;
        OccupiedPoint.bLockedAttackPoint = LockedSlots.Contains(Anchor.OwnerObjectID);
        OutOccupiedPoints.Add(OccupiedPoint);
    }

    for (const auto& ClusterPair : Group.WaitClusters)
    {
        for (const FCrowdWaitPoint& WaitPoint : ClusterPair.Value.WaitPoints)
        {
            if (WaitPoint.OwnerObjectID == INDEX_NONE)
            {
                continue;
            }

            // Only include wait points whose owner has a valid cached assignment and is in position
            const FCrowdSurroundAssignment* Assignment = Assignments.Find(WaitPoint.OwnerObjectID);
            if (!Assignment
                || Assignment->Type != ECrowdSurroundAssignmentType::WaitPoint
                || !Assignment->bIsInPosition)
            {
                continue;
            }

            FCrowdSurroundOccupiedPoint OccupiedPoint;
            OccupiedPoint.OwnerObjectID = WaitPoint.OwnerObjectID;
            OccupiedPoint.AnchorID = WaitPoint.AnchorID;
            OccupiedPoint.WaitIndex = WaitPoint.WaitIndex;
            OccupiedPoint.Position = WaitPoint.Position;
            OccupiedPoint.Radius = GetAgentOccupancyRadius(WaitPoint.OwnerObjectID);
            OccupiedPoint.ContactPadding = GetAgentContactPadding(WaitPoint.OwnerObjectID);
            OccupiedPoint.bAttackPoint = false;
            OccupiedPoint.bLockedAttackPoint = false;
            OutOccupiedPoints.Add(OccupiedPoint);
        }
    }
}

// ---------------------------------------------------------------------------
// IsAttackPositionAvailable: check if a position is free of collisions
// ---------------------------------------------------------------------------
bool UCrowdSurroundManager::IsAttackPositionAvailable(const FCrowdSurroundCandidate& Candidate, const FVector& Position, const TArray<FCrowdSurroundOccupiedPoint>& OccupiedPoints) const
{
    if (!HasLineOfMovement(Position))
    {
        return false;
    }

    for (const FCrowdSurroundOccupiedPoint& OccupiedPoint : OccupiedPoints)
    {
        if (DoesOverlapOccupiedPoint(Position, Candidate, OccupiedPoint))
        {
            return false;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// IsWaitPositionAvailable: check if a wait position is valid and collision-free
// ---------------------------------------------------------------------------
bool UCrowdSurroundManager::IsWaitPositionAvailable(
    const FCrowdSurroundGroup& Group,
    const FCrowdSurroundCandidate& Candidate,
    const FVector& Position,
    const TArray<FCrowdSurroundOccupiedPoint>& OccupiedPoints) const
{
    if (!HasLineOfMovement(Position))
    {
        return false;
    }

    const float DistanceToTarget = FVector::Dist2D(Position, Group.AnchorPosition);
    const float TargetSeparation = Group.TargetRadius + GetCandidatePhysicalRadius(Candidate) + Candidate.ContactPadding;
    if (DistanceToTarget <= TargetSeparation)
    {
        return false;
    }

    if (Candidate.BandMax > KINDA_SMALL_NUMBER && DistanceToTarget <= Candidate.BandMax + Candidate.ContactPadding)
    {
        return false;
    }

    for (const FCrowdSurroundOccupiedPoint& OccupiedPoint : OccupiedPoints)
    {
        if (DoesOverlapOccupiedPoint(Position, Candidate, OccupiedPoint))
        {
            return false;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// FindNearestAttackAnchorID: find the closest occupied attack anchor
// ---------------------------------------------------------------------------
int32 UCrowdSurroundManager::FindNearestAttackAnchorID(const FCrowdSurroundGroup& Group, const FVector& Position) const
{
    int32 BestAnchorID = INDEX_NONE;
    float BestDistanceSq = TNumericLimits<float>::Max();

    for (const FCrowdAttackAnchor& Anchor : Group.AttackAnchors)
    {
        if (!IsAttackAnchorOccupied(Anchor))
        {
            continue;
        }

        const float DistanceSq = FVector::DistSquared2D(Anchor.Position, Position);
        if (DistanceSq < BestDistanceSq)
        {
            BestDistanceSq = DistanceSq;
            BestAnchorID = Anchor.AnchorID;
        }
    }

    return BestAnchorID;
}

// ---------------------------------------------------------------------------
// TryGetMoveToAttackPosition: find a valid attack position for a candidate
// ---------------------------------------------------------------------------
bool UCrowdSurroundManager::TryGetMoveToAttackPosition(const FCrowdSurroundGroup& Group, const FCrowdSurroundCandidate& Candidate, FVector& OutMovePosition) const
{
    OutMovePosition = FVector::ZeroVector;
    if (Candidate.BandMax < Candidate.BandMin || Candidate.AttackRange <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    const float ContactRadius = GetAttackContactRadius(Group, Candidate);
    const float AttackTolerance = FMath::Max(1.0f, Candidate.ContactPadding);
    if (ContactRadius > Candidate.BandMax + AttackTolerance)
    {
        return false;
    }

    TArray<FCrowdSurroundOccupiedPoint> OccupiedPoints;
    CollectOccupiedPoints(Group, OccupiedPoints);

    const FVector PreferredDirection = GetDirectionFromTarget(Group, Candidate.Position);
    const FVector DirectContactPosition = Group.AnchorPosition + PreferredDirection * ContactRadius;

    if (IsAttackPositionAvailable(Candidate, DirectContactPosition, OccupiedPoints))
    {
        OutMovePosition = DirectContactPosition;
        return true;
    }

    TArray<FCrowdPointCandidate> TangentCandidates;
    TangentCandidates.Reserve(OccupiedPoints.Num() * 2);

    for (const FCrowdSurroundOccupiedPoint& OccupiedPoint : OccupiedPoints)
    {
        FVector FirstIntersection;
        FVector SecondIntersection;
        const float TangentDistance = GetSeparationDistance(Candidate, OccupiedPoint);
        if (!FindCircleIntersections2D(Group.AnchorPosition, ContactRadius, OccupiedPoint.Position, TangentDistance, FirstIntersection, SecondIntersection))
        {
            continue;
        }

        const FVector Intersections[] = { FirstIntersection, SecondIntersection };
        for (const FVector& Intersection : Intersections)
        {
            FCrowdPointCandidate PointCandidate;
            PointCandidate.Position = Intersection;
            PointCandidate.Cost = FVector::Dist2D(Candidate.Position, Intersection)
                + GetAngularCostFromDirection(Group, Intersection, PreferredDirection) * ContactRadius * 0.75f;
            TangentCandidates.Add(PointCandidate);
        }
    }

    const float AngleStep = FMath::DegreesToRadians(5.0f);
    const float PreferredAngle = FMath::Atan2(PreferredDirection.Y, PreferredDirection.X);
    const int32 MaxStep = FMath::CeilToInt(PI / AngleStep);
    for (int32 Step = 1; Step <= MaxStep; ++Step)
    {
        const float SignedAngles[] = {
            PreferredAngle + AngleStep * static_cast<float>(Step),
            PreferredAngle - AngleStep * static_cast<float>(Step)
        };

        for (float Angle : SignedAngles)
        {
            const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
            const FVector Position = Group.AnchorPosition + Direction.GetSafeNormal2D() * ContactRadius;
            FCrowdPointCandidate PointCandidate;
            PointCandidate.Position = Position;
            PointCandidate.Cost = FVector::Dist2D(Candidate.Position, Position)
                + static_cast<float>(Step) * Candidate.Spacing * 0.25f;
            TangentCandidates.Add(PointCandidate);
        }
    }

    TangentCandidates.Sort([](const FCrowdPointCandidate& A, const FCrowdPointCandidate& B)
    {
        return A.Cost < B.Cost;
    });

    for (const FCrowdPointCandidate& PointCandidate : TangentCandidates)
    {
        if (IsAttackPositionAvailable(Candidate, PointCandidate.Position, OccupiedPoints))
        {
            OutMovePosition = PointCandidate.Position;
            return true;
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
// TryGetMoveToWaitPosition: find a valid wait position for a candidate
// ---------------------------------------------------------------------------
bool UCrowdSurroundManager::TryGetMoveToWaitPosition(const FCrowdSurroundGroup& Group, const FCrowdSurroundCandidate& Candidate, FVector& OutMovePosition) const
{
    OutMovePosition = FVector::ZeroVector;

    TArray<FCrowdSurroundOccupiedPoint> OccupiedPoints;
    CollectOccupiedPoints(Group, OccupiedPoints);
    if (OccupiedPoints.Num() < 2)
    {
        return false;
    }

    bool bHasAttackParent = false;
    for (const FCrowdSurroundOccupiedPoint& OccupiedPoint : OccupiedPoints)
    {
        if (OccupiedPoint.bAttackPoint)
        {
            bHasAttackParent = true;
            break;
        }
    }
    if (!bHasAttackParent)
    {
        return false;
    }

    TArray<FCrowdPointCandidate> WaitCandidates;
    WaitCandidates.Reserve(OccupiedPoints.Num() * OccupiedPoints.Num());

    OccupiedPoints.Sort([&Candidate](const FCrowdSurroundOccupiedPoint& A, const FCrowdSurroundOccupiedPoint& B)
    {
        const float ADistance = FVector::DistSquared2D(A.Position, Candidate.Position);
        const float BDistance = FVector::DistSquared2D(B.Position, Candidate.Position);
        return ADistance < BDistance;
    });

    for (int32 FirstIndex = 0; FirstIndex < OccupiedPoints.Num(); ++FirstIndex)
    {
        for (int32 SecondIndex = FirstIndex + 1; SecondIndex < OccupiedPoints.Num(); ++SecondIndex)
        {
            const FCrowdSurroundOccupiedPoint& First = OccupiedPoints[FirstIndex];
            const FCrowdSurroundOccupiedPoint& Second = OccupiedPoints[SecondIndex];

            FVector FirstIntersection;
            FVector SecondIntersection;
            const float FirstTangentDistance = GetSeparationDistance(Candidate, First);
            const float SecondTangentDistance = GetSeparationDistance(Candidate, Second);
            if (!FindCircleIntersections2D(First.Position, FirstTangentDistance, Second.Position, SecondTangentDistance, FirstIntersection, SecondIntersection))
            {
                continue;
            }

            const FVector Intersections[] = { FirstIntersection, SecondIntersection };
            for (const FVector& Intersection : Intersections)
            {
                FCrowdPointCandidate PointCandidate;
                PointCandidate.Position = Intersection;
                PointCandidate.Cost = FVector::Dist2D(Candidate.Position, Intersection)
                    + FVector::Dist2D(Group.AnchorPosition, Intersection) * 0.2f
                    + static_cast<float>(FirstIndex + SecondIndex) * Candidate.Spacing * 0.05f;
                WaitCandidates.Add(PointCandidate);
            }
        }
    }

    WaitCandidates.Sort([](const FCrowdPointCandidate& A, const FCrowdPointCandidate& B)
    {
        return A.Cost < B.Cost;
    });

    for (const FCrowdPointCandidate& PointCandidate : WaitCandidates)
    {
        if (IsWaitPositionAvailable(Group, Candidate, PointCandidate.Position, OccupiedPoints))
        {
            OutMovePosition = PointCandidate.Position;
            return true;
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
// WriteAttackAssignment: write anchor + cached assignment for an attack agent
// ---------------------------------------------------------------------------
void UCrowdSurroundManager::WriteAttackAssignment(
    FCrowdSurroundGroup& Group,
    int32 ObjectID,
    const FCrowdSurroundCandidate& Candidate,
    const FVector& AttackPosition,
    ECrowdAttackAnchorState AnchorState,
    double Now)
{
    const FVector Direction = GetDirectionFromTarget(Group, AttackPosition);
    const float DistanceToAttackPosition = FVector::Dist2D(Candidate.Position, AttackPosition);
    const float ReachTolerance = FMath::Max(GetReachTolerance(Config), Candidate.Spacing * 0.35f);
    const bool bInPosition = AnchorState == ECrowdAttackAnchorState::Occupied || DistanceToAttackPosition <= ReachTolerance;
    const ECrowdSurroundAssignmentState AssignmentState = bInPosition
        ? ECrowdSurroundAssignmentState::Attacking
        : ECrowdSurroundAssignmentState::MovingToAttack;

    // Derive reservation start time from the existing anchor or locked slot, if any
    double ReservationStartTime = Now;
    for (const FCrowdAttackAnchor& Existing : Group.AttackAnchors)
    {
        if (Existing.OwnerObjectID == ObjectID && Existing.ReservationTime > 0.0)
        {
            ReservationStartTime = Existing.ReservationTime;
            break;
        }
    }
    if (const FLockedSurroundSlot* LockedSlot = LockedSlots.Find(ObjectID))
    {
        if (LockedSlot->LockTime > 0.0)
        {
            ReservationStartTime = LockedSlot->LockTime;
        }
    }

    // Find or create the attack anchor
    FCrowdAttackAnchor* ExistingAnchor = Group.AttackAnchors.FindByPredicate([ObjectID](const FCrowdAttackAnchor& A)
    {
        return A.OwnerObjectID == ObjectID;
    });

    if (ExistingAnchor)
    {
        ExistingAnchor->Direction = Direction;
        ExistingAnchor->Position = AttackPosition;
        ExistingAnchor->Radius = FVector::Dist2D(AttackPosition, Group.AnchorPosition);
        ExistingAnchor->State = bInPosition ? ECrowdAttackAnchorState::Occupied : ECrowdAttackAnchorState::Reserved;
        ExistingAnchor->ReservationTime = bInPosition ? Now : ReservationStartTime;
        ExistingAnchor->LastProgressTime = Now;
        ExistingAnchor->LastProgressDistance = DistanceToAttackPosition;
    }
    else
    {
        FCrowdAttackAnchor Anchor;
        Anchor.AnchorID = ObjectID;
        Anchor.Direction = Direction;
        Anchor.Position = AttackPosition;
        Anchor.Radius = FVector::Dist2D(AttackPosition, Group.AnchorPosition);
        Anchor.State = bInPosition ? ECrowdAttackAnchorState::Occupied : ECrowdAttackAnchorState::Reserved;
        Anchor.OwnerObjectID = ObjectID;
        Anchor.ReservationTime = bInPosition ? Now : ReservationStartTime;
        Anchor.LastProgressTime = Now;
        Anchor.LastProgressDistance = DistanceToAttackPosition;
        Group.AttackAnchors.Add(Anchor);
    }

    // Write cached assignment
    FCrowdSurroundAssignment& Assignment = Assignments.FindOrAdd(ObjectID);
    Assignment.Type = ECrowdSurroundAssignmentType::AttackAnchor;
    Assignment.State = AssignmentState;
    Assignment.AnchorID = ObjectID;
    Assignment.WaitIndex = INDEX_NONE;
    Assignment.WaitLayer = INDEX_NONE;
    Assignment.WaitBranchIndex = INDEX_NONE;
    Assignment.ParentObjectID = INDEX_NONE;
    Assignment.DesiredPosition = AttackPosition;
    Assignment.PathTargetPosition = AttackPosition;
    Assignment.DirectMoveRadius = FMath::Max(Config ? Config->GetCrowdDirectMoveRadius() : 300.0f, FVector::Dist2D(AttackPosition, Group.AnchorPosition) + Candidate.Spacing * 2.0f);
    Assignment.DesiredRadius = FVector::Dist2D(AttackPosition, Group.AnchorPosition);
    Assignment.CurrentRadius = Candidate.DistanceToTarget;
    Assignment.BandMin = Candidate.BandMin;
    Assignment.BandMax = Candidate.BandMax;
    Assignment.Spacing = Candidate.Spacing;
    Assignment.bHasAssignment = true;
    Assignment.bHasAttackPermission = (AnchorState == ECrowdAttackAnchorState::Occupied);
    Assignment.bHasAttackReservation = true;
    Assignment.bShouldMove = (AssignmentState == ECrowdSurroundAssignmentState::MovingToAttack);
    Assignment.bIsInPosition = (AssignmentState == ECrowdSurroundAssignmentState::Attacking);
    Assignment.bLocksCrowdPosition = (AnchorState == ECrowdAttackAnchorState::Occupied);
    Assignment.bIsRefillCandidate = false;
}

// ---------------------------------------------------------------------------
// WriteWaitAssignment: write wait point + cached assignment for a waiting agent
// ---------------------------------------------------------------------------
void UCrowdSurroundManager::WriteWaitAssignment(
    FCrowdSurroundGroup& Group,
    int32 ObjectID,
    const FCrowdSurroundCandidate& Candidate,
    const FVector& WaitPosition,
    int32 ParentAnchorID,
    int32 WaitIndex)
{
    const float ReachTolerance = FMath::Max(GetReachTolerance(Config), Candidate.Spacing * 0.35f);
    const float DistanceToWaitPosition = FVector::Dist2D(Candidate.Position, WaitPosition);
    const bool bInPosition = DistanceToWaitPosition <= ReachTolerance;
    const int32 WaitLayer = FMath::Max(0, FMath::FloorToInt((FVector::Dist2D(WaitPosition, Group.AnchorPosition) - Group.AttackBandMax) / FMath::Max(1.0f, Candidate.Spacing)));

    // Resolve parent ObjectID from anchor
    int32 ParentObjectID = INDEX_NONE;
    for (const FCrowdAttackAnchor& Anchor : Group.AttackAnchors)
    {
        if (Anchor.AnchorID == ParentAnchorID)
        {
            ParentObjectID = Anchor.OwnerObjectID;
            break;
        }
    }

    // Write cached assignment
    FCrowdSurroundAssignment& Assignment = Assignments.FindOrAdd(ObjectID);
    Assignment.Type = ECrowdSurroundAssignmentType::WaitPoint;
    Assignment.State = bInPosition ? ECrowdSurroundAssignmentState::Waiting : ECrowdSurroundAssignmentState::MovingToWait;
    Assignment.AnchorID = ParentAnchorID;
    Assignment.WaitIndex = WaitIndex;
    Assignment.WaitLayer = WaitLayer;
    Assignment.WaitBranchIndex = WaitIndex;
    Assignment.ParentObjectID = ParentObjectID;
    Assignment.DesiredPosition = WaitPosition;
    Assignment.PathTargetPosition = WaitPosition;
    Assignment.DirectMoveRadius = FMath::Max(Config ? Config->GetCrowdDirectMoveRadius() : 300.0f, FVector::Dist2D(WaitPosition, Group.AnchorPosition) + Candidate.Spacing * 2.0f);
    Assignment.DesiredRadius = FVector::Dist2D(WaitPosition, Group.AnchorPosition);
    Assignment.CurrentRadius = Candidate.DistanceToTarget;
    Assignment.BandMin = Candidate.BandMin;
    Assignment.BandMax = Candidate.BandMax;
    Assignment.Spacing = Candidate.Spacing;
    Assignment.bHasAssignment = true;
    Assignment.bHasAttackPermission = false;
    Assignment.bHasAttackReservation = false;
    Assignment.bShouldMove = !bInPosition;
    Assignment.bIsInPosition = bInPosition;
    Assignment.bLocksCrowdPosition = false;
    Assignment.bIsRefillCandidate = WaitLayer == 0;

    // Add wait point to cluster if the agent is in position
    if (bInPosition)
    {
        FCrowdWaitPoint WaitPoint;
        WaitPoint.AnchorID = ParentAnchorID;
        WaitPoint.WaitIndex = WaitIndex;
        WaitPoint.WaitLayer = WaitLayer;
        WaitPoint.WaitBranchIndex = WaitIndex;
        WaitPoint.OwnerObjectID = ObjectID;
        WaitPoint.ParentObjectID = ParentObjectID;
        WaitPoint.Position = WaitPosition;

        FCrowdWaitCluster& Cluster = Group.WaitClusters.FindOrAdd(ParentAnchorID);
        Cluster.AnchorID = ParentAnchorID;
        Cluster.WaitPoints.Add(WaitPoint);
    }
}

// ---------------------------------------------------------------------------
// AssignSurroundIntents: 3-phase assignment (preserve → attack → wait)
// ---------------------------------------------------------------------------
void UCrowdSurroundManager::AssignSurroundIntents(FCrowdSurroundGroup& Group, const TMap<int32, FCrowdSurroundCandidate>& Candidates)
{
    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    const float ReservationTimeout = GetReservationTimeout(Config);
    const float ReachTolerance = GetReachTolerance(Config);

    // Clear assignments only for non-locked agents; locked agents keep their positions
    for (const auto& CandidatePair : Candidates)
    {
        if (LockedSlots.Contains(CandidatePair.Key))
        {
            continue; // Locked agents retain their assignment
        }
        FCrowdSurroundAssignment* Assignment = Assignments.Find(CandidatePair.Key);
        if (Assignment)
        {
            *Assignment = FCrowdSurroundAssignment();
        }
    }

    // Save previous anchors and reset for this frame
    const TArray<FCrowdAttackAnchor> PreviousAttackAnchors = Group.AttackAnchors;
    Group.AttackAnchors.Reset();
    Group.WaitClusters.Reset();

    TSet<int32> AssignedAgents;

    // --- Phase 1: Preserve Occupied anchors ---
    // An occupied anchor is preserved if its owner is still locked and still a candidate
    for (const FCrowdAttackAnchor& PreviousAnchor : PreviousAttackAnchors)
    {
        if (PreviousAnchor.State != ECrowdAttackAnchorState::Occupied)
        {
            continue;
        }

        const int32 OwnerObjectID = PreviousAnchor.OwnerObjectID;
        if (OwnerObjectID == INDEX_NONE)
        {
            continue;
        }

        // Check if the owner is still in LockedSlots (lock still active)
        if (!LockedSlots.Contains(OwnerObjectID))
        {
            continue;
        }

        // Check if the owner is still a candidate (still wants to surround this target)
        const FCrowdSurroundCandidate* Candidate = Candidates.Find(OwnerObjectID);
        if (!Candidate)
        {
            continue;
        }

        FVector PreservedAttackPosition = PreviousAnchor.Position;
        PreservedAttackPosition.Z = Group.AnchorPosition.Z;

        WriteAttackAssignment(Group, OwnerObjectID, *Candidate, PreservedAttackPosition, ECrowdAttackAnchorState::Occupied, Now);
        AssignedAgents.Add(OwnerObjectID);
    }

    // --- Phase 2: Assign attack positions ---
    // Sort candidates by distance (near → far), then by RequestTime (early → late)
    TArray<FCrowdSurroundCandidate> OrderedCandidates;
    Candidates.GenerateValueArray(OrderedCandidates);
    OrderedCandidates.Sort([](const FCrowdSurroundCandidate& A, const FCrowdSurroundCandidate& B)
    {
        if (!FMath::IsNearlyEqual(A.DistanceToTarget, B.DistanceToTarget, 1.0f))
        {
            return A.DistanceToTarget < B.DistanceToTarget;
        }

        if (FMath::Abs(A.RequestTime - B.RequestTime) > KINDA_SMALL_NUMBER)
        {
            return A.RequestTime < B.RequestTime;
        }

        return A.ObjectID < B.ObjectID;
    });

    for (const FCrowdSurroundCandidate& Candidate : OrderedCandidates)
    {
        if (AssignedAgents.Contains(Candidate.ObjectID))
        {
            continue;
        }

        // Skip locked agents — they already have fixed positions
        if (LockedSlots.Contains(Candidate.ObjectID))
        {
            continue;
        }

        FVector AttackPosition;
        if (!TryGetMoveToAttackPosition(Group, Candidate, AttackPosition))
        {
            continue;
        }

        const float DistanceToAttackPosition = FVector::Dist2D(Candidate.Position, AttackPosition);
        const bool bArrived = DistanceToAttackPosition <= FMath::Max(ReachTolerance, Candidate.Spacing * 0.35f);

        // Mark as Reserved (not Occupied — Occupied is only set by Lock)
        const ECrowdAttackAnchorState AnchorState = ECrowdAttackAnchorState::Reserved;

        // Check reservation timeout: if the agent has been reserved for too long without progress, free it
        if (!bArrived)
        {
            // Check for existing reservation from a previous anchor
            double ExistingReservationTime = 0.0;
            for (const FCrowdAttackAnchor& Prev : PreviousAttackAnchors)
            {
                if (Prev.OwnerObjectID == Candidate.ObjectID && Prev.State == ECrowdAttackAnchorState::Reserved)
                {
                    ExistingReservationTime = Prev.ReservationTime;
                    break;
                }
            }

            if (ExistingReservationTime > 0.0 && Now - ExistingReservationTime > ReservationTimeout)
            {
                // Reservation timed out — free the anchor, let agent re-compete next time
                Assignments.Remove(Candidate.ObjectID);
                continue;
            }

            // Progress tracking: check if agent is getting closer to its assigned position
            for (const FCrowdAttackAnchor& Prev : PreviousAttackAnchors)
            {
                if (Prev.OwnerObjectID == Candidate.ObjectID && Prev.State == ECrowdAttackAnchorState::Reserved)
                {
                    if (Prev.LastProgressDistance < TNumericLimits<float>::Max())
                    {
                        const float ProgressEpsilon = Config ? Config->GetCrowdProgressEpsilon() : 5.0f;
                        if (DistanceToAttackPosition > Prev.LastProgressDistance + ProgressEpsilon)
                        {
                            // Agent is moving away — free the reservation
                            Assignments.Remove(Candidate.ObjectID);
                            goto NextCandidate;
                        }
                    }
                    break;
                }
            }
        }

        WriteAttackAssignment(Group, Candidate.ObjectID, Candidate, AttackPosition, AnchorState, Now);
        AssignedAgents.Add(Candidate.ObjectID);

    NextCandidate:
        continue;
    }

    // --- Phase 3: Assign wait positions ---
    int32 NextWaitIndex = 0;
    for (const FCrowdSurroundCandidate& Candidate : OrderedCandidates)
    {
        if (AssignedAgents.Contains(Candidate.ObjectID))
        {
            continue;
        }

        // Skip locked agents — they already have fixed positions
        if (LockedSlots.Contains(Candidate.ObjectID))
        {
            continue;
        }

        FVector WaitPosition;
        if (!TryGetMoveToWaitPosition(Group, Candidate, WaitPosition))
        {
            // No wait position available — clear assignment
            Assignments.Remove(Candidate.ObjectID);
            continue;
        }

        const int32 ParentAnchorID = FindNearestAttackAnchorID(Group, WaitPosition);
        if (ParentAnchorID == INDEX_NONE)
        {
            Assignments.Remove(Candidate.ObjectID);
            continue;
        }

        WriteWaitAssignment(Group, Candidate.ObjectID, Candidate, WaitPosition, ParentAnchorID, NextWaitIndex++);
        AssignedAgents.Add(Candidate.ObjectID);
    }

    Group.AnchorCount = Group.AttackAnchors.Num();
}

// ---------------------------------------------------------------------------
// RecomputeGroupAssignments: full group recomputation triggered by state changes
// ---------------------------------------------------------------------------
void UCrowdSurroundManager::RecomputeGroupAssignments(int32 TargetObjectID)
{
    FCrowdSurroundGroup* Group = SurroundGroups.Find(TargetObjectID);
    if (!Group)
    {
        return;
    }

    USparseGridComponent* TargetComponent = Group->TargetComponent.Get();
    if (!TargetComponent)
    {
        return;
    }

    // Update group anchor position and target radius from the target component
    Group->AnchorPosition = CrowdSurroundUtilities::FlattenPosition(TargetComponent->GetSparseGridPosition());
    Group->TargetRadius = TargetComponent->GetSparseGridCollisionRadius();

    // Build candidate list from AgentIntents
    TMap<int32, FCrowdSurroundCandidate> Candidates = BuildCandidatesForGroup(*Group);

    if (Candidates.Num() == 0)
    {
        Group->AttackAnchors.Empty();
        Group->WaitClusters.Empty();
        Group->AnchorCount = 0;
        return;
    }

    ComputeGroupMetrics(*Group, Candidates);
    AssignSurroundIntents(*Group, Candidates);
    Group->LastAnchorPosition = Group->AnchorPosition;
}
