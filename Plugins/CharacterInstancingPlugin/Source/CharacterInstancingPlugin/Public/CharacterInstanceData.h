#pragma once

#include "CoreMinimal.h"
#include "ICharacterDataSource.h"
#include "CharacterSkelMeshComponent.h"
#include "CharacterInstanceData.generated.h"

USTRUCT()
struct FCharacterInstanceData
{
    GENERATED_BODY()

    int32 CharacterID = -1;

    FTransform Transform = FTransform::Identity;
    FVector PrevPosition = FVector::ZeroVector;

    float MovementSpeed = 0.0f;

    UPROPERTY(Transient)
    TObjectPtr<UCharacterSkelMeshComponent> SkelMeshComp = nullptr;

    ICharacterDataSource* GetDataSource() const
    {
        return SkelMeshComp ? Cast<ICharacterDataSource>(SkelMeshComp) : nullptr;
    }

    UCharacterSkelMeshComponent* GetComponent() const
    {
        return SkelMeshComp;
    }

    void Deactivate()
    {
        CharacterID = -1;
        MovementSpeed = 0.0f;
        SkelMeshComp = nullptr;
    }
};

USTRUCT()
struct FCharacterStateBits
{
    GENERATED_BODY()

    TBitArray<> IsActive;

    void InitAll(int32 Count)
    {
        IsActive.Init(false, Count);
    }

    void ResizeAll(int32 OldCount, int32 NewCount)
    {
        IsActive.SetNum(NewCount, false);
    }

    void ClearAll()
    {
        IsActive.Init(false, IsActive.Num());
    }
};

USTRUCT()
struct FCharacterIndexCache
{
    GENERATED_BODY()

    TArray<int32> ActiveIndices;

    void Clear()
    {
        ActiveIndices.Reset();
    }
};
