#include "CharacterAnimEvaluator.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AnimTypes.h"
#include "Animation/AttributesRuntime.h"
#include "ReferenceSkeleton.h"
#include "BoneContainer.h"
#include "Engine/SkeletalMesh.h"

void FCharacterAnimEvaluator::SampleAnimation(const UAnimSequence* AnimSeq, const USkeletalMesh* SkelMesh, float Time, TArray<FTransform>& OutLocalTransforms)
{
    if (!AnimSeq || !SkelMesh)
        return;

    USkeleton* Skeleton = AnimSeq->GetSkeleton();
    if (!Skeleton)
        return;

    const FReferenceSkeleton& MeshRefSkeleton = SkelMesh->GetRefSkeleton();
    const int32 NumBones = MeshRefSkeleton.GetNum();
    OutLocalTransforms.SetNumUninitialized(NumBones);

    TArray<FBoneIndexType> RequiredBoneIndices;
    RequiredBoneIndices.SetNum(NumBones);
    for (int32 i = 0; i < NumBones; ++i)
    {
        RequiredBoneIndices[i] = i;
    }

    UE::Anim::FCurveFilterSettings CurveFilterSettings(UE::Anim::ECurveFilterMode::None);
    FBoneContainer BoneContainer;
    BoneContainer.InitializeTo(RequiredBoneIndices, CurveFilterSettings, *SkelMesh);

    FCompactPose CompactPose;
    CompactPose.SetBoneContainer(&BoneContainer);
    CompactPose.ResetToRefPose();

    FBlendedCurve BlendedCurve;
    BlendedCurve.InitFrom(BoneContainer);

    UE::Anim::FStackAttributeContainer Attributes;

    FAnimationPoseData PoseData(CompactPose, BlendedCurve, Attributes);

    FAnimExtractContext Context(static_cast<double>(Time));
    AnimSeq->GetAnimationPose(PoseData, Context);

    for (int32 i = 0; i < NumBones; ++i)
    {
        OutLocalTransforms[i] = CompactPose[FCompactPoseBoneIndex(i)];
    }
}

void FCharacterAnimEvaluator::LocalToComponentSpace(const FReferenceSkeleton& RefSkeleton, const TArray<FTransform>& LocalTransforms, TArray<FTransform>& OutComponentSpaceTransforms)
{
    const int32 NumBones = RefSkeleton.GetNum();
    OutComponentSpaceTransforms.SetNumUninitialized(NumBones);

    if (NumBones == 0)
        return;

    OutComponentSpaceTransforms[0] = LocalTransforms[0];

    for (int32 i = 1; i < NumBones; ++i)
    {
        const int32 ParentIndex = RefSkeleton.GetParentIndex(i);
        check(ParentIndex >= 0 && ParentIndex < i);
        OutComponentSpaceTransforms[i] = LocalTransforms[i] * OutComponentSpaceTransforms[ParentIndex];
    }
}

void FCharacterAnimEvaluator::EvaluateSingleInstance(const UAnimSequence* AnimSeq, const USkeletalMesh* SkelMesh, float Time, const FReferenceSkeleton& RefSkeleton, TArray<FTransform>& OutLocalTransforms, TArray<FTransform>& OutComponentSpaceTransforms)
{
    SampleAnimation(AnimSeq, SkelMesh, Time, OutLocalTransforms);
    LocalToComponentSpace(RefSkeleton, OutLocalTransforms, OutComponentSpaceTransforms);
}
