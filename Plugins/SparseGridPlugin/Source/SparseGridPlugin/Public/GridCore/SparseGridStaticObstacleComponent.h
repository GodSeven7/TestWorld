// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SparseGridComponent.h"
#include "SparseGridStaticObstacleComponent.generated.h"

UENUM(BlueprintType)
enum class ESparseGridStaticObstacleShape : uint8
{
    Box,
    Circle
};

UCLASS(ClassGroup = "SparseGrid", meta = (BlueprintSpawnableComponent))
class SPARSEGRIDPLUGIN_API USparseGridStaticObstacleComponent : public USparseGridComponent
{
    GENERATED_BODY()

public:
    USparseGridStaticObstacleComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SparseGrid|StaticObstacle")
    bool bAffectsFlowField = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SparseGrid|StaticObstacle")
    bool bRegistersLocalAvoidance = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SparseGrid|StaticObstacle")
    bool bAutoRegisterToSparseGrid = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SparseGrid|StaticObstacle")
    bool bUseOwnerBounds = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SparseGrid|StaticObstacle")
    ESparseGridStaticObstacleShape Shape = ESparseGridStaticObstacleShape::Box;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SparseGrid|StaticObstacle", meta = (EditCondition = "!bUseOwnerBounds"))
    FVector BoxExtent = FVector(100.0f, 100.0f, 100.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SparseGrid|StaticObstacle", meta = (EditCondition = "!bUseOwnerBounds", ClampMin = "0.0"))
    float Radius = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SparseGrid|StaticObstacle", meta = (ClampMin = "0.0"))
    float AgentClearance = 0.0f;

    UFUNCTION(BlueprintCallable, Category = "SparseGrid|StaticObstacle")
    void RefreshStaticObstacle();

    virtual bool IsNavigationObstacle() const override { return true; }
    virtual bool IsCrowdPushLocked() const override { return true; }
    virtual FVector QueryMovementVelocity() const override { return FVector::ZeroVector; }
    virtual float QueryMoveSpeed() const override { return 0.0f; }
    virtual FVector QueryDesiredPosition() const override { return FVector::ZeroVector; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void RegisterStaticObstacleCells();
    void UnregisterStaticObstacleCells();
    void BuildStaticObstacleCells(TArray<FIntVector>& OutCells) const;
    FBox GetStaticObstacleWorldBounds() const;
    float GetLocalAvoidanceRadiusFromBounds(const FBox& Bounds) const;

    TArray<FIntVector> RegisteredStaticCells;
};
