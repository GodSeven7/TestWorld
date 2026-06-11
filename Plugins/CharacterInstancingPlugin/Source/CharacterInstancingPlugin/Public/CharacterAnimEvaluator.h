#pragma once

#include "CoreMinimal.h"

class UAnimSequence;
class USkeletalMesh;
struct FReferenceSkeleton;

class CHARACTERINSTANCINGPLUGIN_API FCharacterAnimEvaluator
{
public:
    static void SampleAnimation(const UAnimSequence* AnimSeq, const USkeletalMesh* SkelMesh, float Time, TArray<FTransform>& OutLocalTransforms);
    static void LocalToComponentSpace(const FReferenceSkeleton& RefSkeleton, const TArray<FTransform>& LocalTransforms, TArray<FTransform>& OutComponentSpaceTransforms);
    static void EvaluateSingleInstance(const UAnimSequence* AnimSeq, const USkeletalMesh* SkelMesh, float Time, const FReferenceSkeleton& RefSkeleton, TArray<FTransform>& OutLocalTransforms, TArray<FTransform>& OutComponentSpaceTransforms);
};
