#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SparseGridConfig.generated.h"

UCLASS()
class SPARSEGRIDPLUGIN_API USparseGridConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "SparseGrid")
    float BaseRadius = 50.0f;

    UPROPERTY(EditDefaultsOnly, Category = "SparseGrid")
    int32 NumLevels = 4;

    UPROPERTY(EditDefaultsOnly, Category = "SparseGrid|Navigation")
    int32 CoarsePathLevel = 1;

    float GetNavCellSize() const { return BaseRadius; }

    UPROPERTY(EditDefaultsOnly, Category = "SparseGrid|Steering")
    float SeparationWeight = 1.5f;

    UPROPERTY(EditDefaultsOnly, Category = "SparseGrid|Steering")
    float AlignmentWeight = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "SparseGrid|Steering")
    int32 MaxSeparationNeighbors = 6;

    UPROPERTY(EditDefaultsOnly, Category = "SparseGrid|Steering")
    float AvoidanceWeight = 2.0f;

    UPROPERTY(EditDefaultsOnly, Category = "SparseGrid|Steering")
    float MaxSpeed = 600.0f;

    UPROPERTY(EditDefaultsOnly, Category = "SparseGrid|Steering")
    float MaxForce = 1000.0f;

    UPROPERTY(EditDefaultsOnly, Category = "SparseGrid|Navigation")
    int32 MaxFlowFieldGenerationsPerFrame = 4;

    float GetSeparationRadius() const { return BaseRadius * 4.0f; }
    float GetAvoidanceLookAhead() const { return BaseRadius * 2.0f; }
    float GetDynamicAvoidanceLookAhead() const { return BaseRadius * 5.0f; }
    float GetLocalDirectMoveRadius() const { return BaseRadius * 3.0f; }
    float GetArrivalSlowRadius() const { return BaseRadius * 1.5f; }
    float GetArrivalStopRadius() const { return BaseRadius * 0.8f; }

    UPROPERTY(EditDefaultsOnly, Category = "SparseGrid|CrowdSurround")
    int32 MaxAttackersPerTarget = 0;

    UPROPERTY(EditDefaultsOnly, Category = "SparseGrid|CrowdSurround")
    float CrowdSurroundUpdateInterval = 0.05f;

    UPROPERTY(EditDefaultsOnly, Category = "SparseGrid|CrowdSurround")
    bool bEnableCrowdWallCheck = true;

    float GetAttackRange() const { return BaseRadius * 2.4f; }
    float GetMinCrowdSpacing() const { return BaseRadius * 1.6f; }
    float GetCrowdReachTolerance() const { return BaseRadius * 0.8f; }
    float GetWaitClusterSpacing() const { return BaseRadius * 2.2f; }
    float GetCrowdDirectMoveRadius() const { return BaseRadius * 6.0f; }
    float GetCrowdContactPadding() const { return BaseRadius * 0.2f; }
    float GetAttackBandInnerBuffer() const { return BaseRadius * 0.6f; }
    float GetSurroundReanchorTolerance() const { return BaseRadius * 0.8f; }
    float GetCrowdReservationTimeout() const { return 0.8f; }
    float GetCrowdProgressEpsilon() const { return BaseRadius * 0.1f; }
};
