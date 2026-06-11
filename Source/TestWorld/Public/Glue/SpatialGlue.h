#pragma once

#include "CoreMinimal.h"
#include "GlueBase.h"
#include "EventBus.h"
#include "SparseGridCollision.h"
#include "SpatialGlue.generated.h"

class USparseGridManager;
class UFlowFieldManager;
class UCrowdSurroundManager;
class UActorUnifiedDataComponent;

UCLASS()
class TESTWORLD_API USpatialGlue : public UGlueBase
{
	GENERATED_BODY()

public:
	virtual void Initialize(UWorld* World) override;
	virtual void Shutdown() override;

	void Bind(
		USparseGridManager* InGridManager,
		UFlowFieldManager* InFlowFieldManager,
		UCrowdSurroundManager* InCrowdSurroundManager);

	USparseGridManager* GetGridManager() const { return GridManager; }
	UFlowFieldManager* GetFlowFieldManager() const { return FlowFieldManager; }

	// New tick flow order:
	// 1. TickCrowdSurround         → CrowdSurroundManager->Update()
	// 2. TickCrowdSurroundCorrection → bridge CrowdSurround→CorrectedDesiredPosition
	// 3. TickFlowFieldQuery        → GridManager->ComputeFlowFieldDirections()
	// 4. TickSteeringCorrection    → GridManager->ApplySteeringCorrections() + write ComputedVelocity
	// 5. TickSpatialUpdate         → sync predicted positions to spatial grid
	// 6. TickCollisionDetection    → GridManager->ProcessCollisions() (event-only, no push)
	// 7. TickNavigationUpdate      → FlowFieldManager updates

	// Step 1: Crowd surround update
	void TickCrowdSurround(float DeltaTime);

	// Step 2: Bridge CrowdSurround assignments to UnifiedData CorrectedDesiredPosition
	void TickCrowdSurroundCorrection();

	// Step 3: Flow field direction computation
	void TickFlowFieldQuery();

	// Step 4: Steering correction + velocity write-back to UnifiedData
	void TickSteeringCorrection();

	// Step 5: Spatial position prediction and batch update
	void TickSpatialUpdate(float DeltaTime);

	// Step 6: Collision detection (event dispatch only, no push vectors)
	void TickCollisionDetection(const TArray<TObjectPtr<UActorUnifiedDataComponent>>& InUnifiedDataComponents);

	// Step 7: Dynamic obstacles + flow field processing
	void TickNavigationUpdate();

	// Faction change notification (delegated from Coordinator)
	void NotifyFactionChanged(UActorUnifiedDataComponent* Component, int32 OldFaction, int32 NewFaction);

private:
	UPROPERTY()
	TObjectPtr<USparseGridManager> GridManager;

	UPROPERTY()
	TObjectPtr<UFlowFieldManager> FlowFieldManager;

	// CrowdSurroundManager for bridging assignments to UnifiedData
	UPROPERTY()
	TObjectPtr<UCrowdSurroundManager> CrowdSurroundManager;

	static constexpr int32 BATCH_THRESHOLD = 100;
};
