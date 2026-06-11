#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CharacterInstanceData.h"
#include "CharacterAnimManager.generated.h"

UCLASS()
class CHARACTERINSTANCINGPLUGIN_API UCharacterAnimManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    void Tick(float DeltaTime);

    int32 RegisterComponent(UCharacterSkelMeshComponent* Comp);
    void UnregisterComponent(int32 CharacterID);

    int32 GetActiveCharacterCount() const;

    void SetGlobalTimeDilation(float InDilation) { GlobalTimeDilation = InDilation; }
    float GetGlobalTimeDilation() const { return GlobalTimeDilation; }

    void SetGameplaySuspended(bool bSuspended) { bGameplaySuspended = bSuspended; }
    bool IsGameplaySuspended() const { return bGameplaySuspended; }

protected:
    TArray<FCharacterInstanceData> Instances;
    TMap<int32, int32> CharacterIDToIndex;
    TArray<int32> FreeIndices;
    FCharacterStateBits StateBits;
    FCharacterIndexCache IndexCache;

    int32 NextCharacterID = 1;
    static constexpr int32 INITIAL_CAPACITY = 200;

    FCriticalSection SlotLock;

    float GlobalTimeDilation = 1.0f;
    bool bGameplaySuspended = false;

    void PreAllocateInstances(int32 Count);
    int32 AllocateSlot();
    void ReleaseSlot(int32 Index);

    void CollectActiveIndices();
    void ResolveAnimStates();
    void AdvanceTimeAndSyncTransforms(float DeltaTime);
    void EvaluateAnimationParallel();
    void ApplyBoneTransforms();
};
