#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterAnimData.generated.h"

USTRUCT(BlueprintType)
struct FCharacterAnimStateEntry
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    FName StateName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    TObjectPtr<UAnimSequence> AnimSequence;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    float OverridePlayRate = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    bool bLooping = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    int32 AnimBankIndex = 0;
};

UCLASS(BlueprintType, Category = "Character Animation")
class CHARACTERINSTANCINGPLUGIN_API UCharacterAnimData : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Animation")
    TObjectPtr<USkeletalMesh> SkeletalMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Animation")
    TArray<FCharacterAnimStateEntry> AnimStates;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Animation")
    float DefaultPlayRate = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Animation")
    FName DefaultStateName = "Idle";

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Animation")
    FName DeathStateName = "Death";

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Animation")
    FName AttackStateName = "Attack";

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Animation")
    FName WalkStateName = "Walk";

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Animation")
    FName RunStateName = "Run";

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Animation")
    float WalkSpeedThreshold = 50.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Animation")
    float RunSpeedThreshold = 300.0f;

    UAnimSequence* GetAnimByState(FName StateName) const;
    const FCharacterAnimStateEntry* GetAnimStateEntry(FName StateName) const;
    bool HasState(FName StateName) const;
    int32 GetAnimStateIndex(FName StateName) const;
};
