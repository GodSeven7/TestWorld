#include "CharacterSkelMeshComponent.h"
#include "CharacterAnimEvaluator.h"
#include "Animation/AnimSequence.h"
#include "ReferenceSkeleton.h"

UCharacterSkelMeshComponent::UCharacterSkelMeshComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCharacterSkelMeshComponent::BeginPlay()
{
    Super::BeginPlay();
    PrimaryComponentTick.bCanEverTick = false;
}

void UCharacterSkelMeshComponent::InitFromAnimData(UCharacterAnimData* InAnimData)
{
    if (!InAnimData)
        return;

    AnimConfig = InAnimData;

    if (InAnimData->SkeletalMesh)
    {
        SetSkeletalMesh(InAnimData->SkeletalMesh);
    }

    SetAnimState(InAnimData->DefaultStateName);
}

void UCharacterSkelMeshComponent::SetAnimState(FName StateName)
{
    if (!AnimConfig || !AnimConfig->HasState(StateName))
        return;

    const FCharacterAnimStateEntry* Entry = AnimConfig->GetAnimStateEntry(StateName);

    AnimState.CurrentStateName = StateName;
    AnimState.CurrentTime = 0.f;
    AnimState.bPlaying = true;      
    AnimState.bLooping = Entry->bLooping;

    if (Entry->OverridePlayRate > 0.f)
    {
        AnimState.PlayRate = Entry->OverridePlayRate;
    }
    else
    {
        AnimState.PlayRate = AnimConfig->DefaultPlayRate;
    }
}

UAnimSequence* UCharacterSkelMeshComponent::GetCurrentAnimSequence() const
{
    if (!AnimConfig)
        return nullptr;
    return AnimConfig->GetAnimByState(AnimState.CurrentStateName);
}

float UCharacterSkelMeshComponent::GetAnimLength() const
{
    UAnimSequence* Seq = GetCurrentAnimSequence();
    return Seq ? Seq->GetPlayLength() : 0.f;
}

void UCharacterSkelMeshComponent::StopAnim()
{
    AnimState.bPlaying = false;
    if (AnimConfig)
    {
        SetAnimState(AnimConfig->DefaultStateName);
    }
}

void UCharacterSkelMeshComponent::RefreshBoneTransforms(FActorComponentTickFunction* TickFunction)
{
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        Super::RefreshBoneTransforms(TickFunction);
        return;
    }
}

void UCharacterSkelMeshComponent::AdvanceTime(float DeltaTime)
{
    float AnimLength = GetAnimLength();
    AnimState.AdvanceTime(DeltaTime, AnimLength);
}

void UCharacterSkelMeshComponent::EvaluateAnimation()
{
    if (!AnimState.IsValid() || !GetSkeletalMeshAsset())
        return;

    UAnimSequence* AnimSeq = GetCurrentAnimSequence();
    if (!AnimSeq)
        return;

    USkeletalMesh* SkelMesh = GetSkeletalMeshAsset();

    TArray<FTransform> LocalTransforms;
    TArray<FTransform> ComponentSpaceTransforms;

    FCharacterAnimEvaluator::EvaluateSingleInstance(
        AnimSeq,
        SkelMesh,
        AnimState.CurrentTime,
        SkelMesh->GetRefSkeleton(),
        LocalTransforms,
        ComponentSpaceTransforms
    );

    {
        FWriteScopeLock WriteLock(CacheLock);
        CachedComponentSpaceTransforms = MoveTemp(ComponentSpaceTransforms);
        bNeedsRenderUpdate = true;
    }
}

void UCharacterSkelMeshComponent::ApplyBoneTransforms()
{
    FReadScopeLock ReadLock(CacheLock);

    if (!bNeedsRenderUpdate || CachedComponentSpaceTransforms.IsEmpty())
        return;

    TArray<FTransform>& EditableTransforms = GetEditableComponentSpaceTransforms();
    EditableTransforms = CachedComponentSpaceTransforms;

    bNeedToFlipSpaceBaseBuffers = true;
    FinalizeBoneTransform();

    InvalidateCachedBounds();
    UpdateBounds();

    MarkRenderTransformDirty();
    MarkRenderDynamicDataDirty();

    bNeedsRenderUpdate = false;
}
