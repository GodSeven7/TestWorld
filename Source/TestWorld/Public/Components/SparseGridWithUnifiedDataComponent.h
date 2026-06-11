#pragma once

#include "CoreMinimal.h"
#include "SparseGridComponent.h"
#include "WithUnifiedData.h"
#include "SparseGridWithUnifiedDataComponent.generated.h"

class UActorUnifiedDataComponent;

UCLASS(ClassGroup = "SparseGrid", meta = (BlueprintSpawnableComponent))
class TESTWORLD_API USparseGridWithUnifiedDataComponent 
	: public USparseGridComponent
	, public IWithUnifiedData
{
	GENERATED_BODY()

public:
	USparseGridWithUnifiedDataComponent();

	virtual UActorUnifiedDataComponent* GetUnifiedDataComponent() const override;

	virtual FVector GetSparseGridPosition() const override;
	virtual FVector GetSparseGridForwardVector() const override;
	virtual FVector QueryMovementVelocity() const override;
	virtual float QueryMoveSpeed() const override;
	virtual FVector QueryDesiredPosition() const override;
	virtual bool IsNavigationObstacle() const override;
	virtual int32 GetFaction() const override;

	virtual FVector GetPredictedPosition(float DeltaTime) const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UActorUnifiedDataComponent> UnifiedDataComponent;
};
