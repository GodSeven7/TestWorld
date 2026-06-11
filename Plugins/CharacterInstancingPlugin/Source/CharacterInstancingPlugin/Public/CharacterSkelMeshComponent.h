#pragma once

#include "CoreMinimal.h"
#include "Components/SkinnedMeshComponent.h"
#include "ICharacterDataSource.h"
#include "CharacterAnimData.h"
#include "CharacterAnimState.h"
#include "CharacterSkelMeshComponent.generated.h"

UCLASS(ClassGroup = (Rendering), meta = (BlueprintSpawnableComponent))
class CHARACTERINSTANCINGPLUGIN_API UCharacterSkelMeshComponent : public USkeletalMeshComponent, public ICharacterDataSource
{
    GENERATED_BODY()

public:
    UCharacterSkelMeshComponent();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Character Animation")
    void InitFromAnimData(UCharacterAnimData* InAnimData);

    UFUNCTION(BlueprintPure, Category = "Character Animation")
    UCharacterAnimData* GetAnimData() const { return AnimConfig; }

    UFUNCTION(BlueprintCallable, Category = "Character Animation")
    void SetAnimState(FName StateName);

    UFUNCTION(BlueprintPure, Category = "Character Animation")
    FName GetCurrentState() const { return AnimState.CurrentStateName; }

    UFUNCTION(BlueprintCallable, Category = "Character Animation")
    void SetPlayAnimRate(float InPlayRate) { AnimState.PlayRate = InPlayRate; }

    UFUNCTION(BlueprintCallable, Category = "Character Animation")
    void SetLooping(bool bInLooping) { AnimState.bLooping = bInLooping; }

    UFUNCTION(BlueprintCallable, Category = "Character Animation")
    void PlayAnim() { AnimState.bPlaying = true; }

    UFUNCTION(BlueprintCallable, Category = "Character Animation")
    void PauseAnim() { AnimState.bPlaying = false; }

    UFUNCTION(BlueprintCallable, Category = "Character Animation")
    void StopAnim();

    UFUNCTION(BlueprintPure, Category = "Character Animation")
    float GetCurrentTime() const { return AnimState.CurrentTime; }

    UFUNCTION(BlueprintCallable, Category = "Character Animation")
    void SetCurrentTime(float InTime) { AnimState.CurrentTime = InTime; }

    UFUNCTION(BlueprintPure, Category = "Character Animation")
    float GetAnimLength() const;

    UFUNCTION(BlueprintPure, Category = "Character Animation")
    bool IsPlayingAnim() const { return AnimState.bPlaying; }

    UFUNCTION(BlueprintPure, Category = "Character Animation")
    UAnimSequence* GetCurrentAnimSequence() const;

    void SetMeshType(int32 InMeshType) { MeshType = InMeshType; }

    void SetDead(bool bInDead) { bIsDead = bInDead; }

    void SetAttacking(bool bInAttacking) { bIsAttacking = bInAttacking; }

    void SetMoving(bool bInMoving) { bIsMoving = bInMoving; }

    void AdvanceTime(float DeltaTime);
    void EvaluateAnimation();
    void ApplyBoneTransforms();

    // ICharacterDataSource
    virtual FVector GetPosition() const override { return GetComponentLocation(); }
    virtual FRotator GetRotation() const override { return GetComponentRotation(); }
    virtual bool IsDead() const override { return bIsDead; }
    virtual bool IsAttacking() const override { return bIsAttacking; }
    virtual bool IsMoving() const override { return bIsMoving; }
    virtual int32 GetMeshType() const override { return MeshType; }
    virtual void SetMeshVisibility(bool bInVisible) override { SetVisibility(bInVisible); }

protected:
    virtual void RefreshBoneTransforms(FActorComponentTickFunction* TickFunction) override;

    UPROPERTY(Transient)
    TObjectPtr<UCharacterAnimData> AnimConfig;

    FCharacterAnimState AnimState;

    TArray<FTransform> CachedComponentSpaceTransforms;

    mutable FRWLock CacheLock;

    bool bNeedsRenderUpdate = false;

    int32 MeshType = 0;

    bool bIsDead = false;

    bool bIsAttacking = false;

    bool bIsMoving = false;
};
