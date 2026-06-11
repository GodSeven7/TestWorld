#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnifiedDataSnapshot.h"
#include "UnifiedDataExtension.h"
#include "UnifiedDataRegistry.h"
#include "ActorUnifiedDataComponent.generated.h"

/**
 * 通用场景对象数据组件
 * 所有场景对象都应拥有此组件，提供位置/旋转/阵营/存活状态等通用数据
 * 支持快照双缓冲和脏标记机制
 * 通过 IUnifiedDataExtension 接口统一管理扩展数据的 Snapshot/Apply
 */
UCLASS(ClassGroup = "UnifiedData", meta = (BlueprintSpawnableComponent))
class COREGLUE_API UActorUnifiedDataComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UActorUnifiedDataComponent();

	void SetAssociatedComponent(USceneComponent* Component) { AssociatedComponent = Component; }
	USceneComponent* GetAssociatedComponent() const { return AssociatedComponent; }

	void CreateSnapshot();
	void ApplyToActor(float DeltaTime);
	void MarkDirty(EUnifiedDataDirtyFlag Flag);

	FUnifiedDataSnapshot& GetMutableData() { return CurrentData; }
	const FUnifiedDataSnapshot& GetData() const { return CurrentData; }
	const FUnifiedDataSnapshot& GetSnapshotData() const { return SnapshotData; }

	void SyncFromActor();

	/** 初始化统一数据：同步Actor数据并注册到Registry。应在Actor的BeginPlay中所有组件就绪后调用 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedData")
	void InitializeUnifiedData();

	// --- 位置 / 旋转 / 朝向 ---
	FVector GetPosition() const { return CurrentData.Position; }
	void SetPosition(const FVector& InPosition);
	FRotator GetRotation() const { return CurrentData.Rotation; }
	void SetRotation(const FRotator& InRotation);
	FVector GetForwardVector() const { return CurrentData.ForwardVector; }
	void SetForwardVector(const FVector& InForward);

	// --- 通用属性 ---
	int32 GetFaction() const { return CurrentData.Faction; }
	void SetFaction(int32 InFaction);
	bool IsAlive() const { return CurrentData.bIsAlive; }
	void SetAlive(bool bAlive);
	int32 GetObjectType() const { return CurrentData.ObjectType; }
	void SetObjectType(int32 InType);

	// --- 目标信息 ---
	bool HasTarget() const;
	int32 GetTargetID() const;
	FVector GetTargetPosition() const;
	float GetTargetDistance() const;
	void SetTarget(int32 InTargetID, const FVector& InTargetPosition, float InDistance);
	void ClearTarget();

	// --- Movement ---
	FVector GetMovementVelocity() const;
	void SetMovementVelocity(const FVector& InVelocity);
	void ClearMovementVelocity();
	float GetMoveSpeed() const;
	void SetMoveSpeed(float InMoveSpeed);
	bool IsMoving() const;
	void SetIsMoving(bool bInMoving);

	// --- Destination ---
	// DesiredPosition: raw destination set by other plugins (e.g. GOAP)
	FVector GetDesiredPosition() const;
	void SetDesiredPosition(const FVector& InPosition);
	bool HasDesiredPosition() const;

	// CorrectedDesiredPosition: crowd-corrected destination, takes priority when set
	FVector GetCorrectedDesiredPosition() const;
	void SetCorrectedDesiredPosition(const FVector& InPosition);
	bool HasCorrectedDesiredPosition() const;
	void ClearCorrectedDesiredPosition();

	// --- Object ID ---
	int32 GetObjectID() const { return CurrentData.ObjectID; }

	bool IsNavigationObstacle() const;
	void SetNavigationObstacle(bool bObstacle);

	// --- 扩展管理 ---
	void RegisterExtension(IUnifiedDataExtension* Extension);
	void UnregisterExtension(IUnifiedDataExtension* Extension);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY()
	USceneComponent* AssociatedComponent = nullptr;

	UPROPERTY()
	FUnifiedDataSnapshot CurrentData;

	UPROPERTY()
	FUnifiedDataSnapshot SnapshotData;

	IUnifiedDataRegistry* CachedRegistry = nullptr;

	TArray<IUnifiedDataExtension*> Extensions;
};
