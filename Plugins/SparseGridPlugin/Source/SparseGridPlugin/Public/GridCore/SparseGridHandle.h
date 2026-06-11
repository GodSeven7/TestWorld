#pragma once

#include "CoreMinimal.h"
#include "SparseGridHandle.generated.h"

/**
 * Safe handle for objects in the sparse grid system.
 * Uses index + generation to prevent dangling references.
 */
USTRUCT(BlueprintType)
struct SPARSEGRIDPLUGIN_API FSparseGridHandle
{
    GENERATED_BODY()

public:
    static const FSparseGridHandle Invalid;

    FSparseGridHandle() : Data(0) {}
    
    FSparseGridHandle(uint32 InIndex, uint32 InGeneration)
    {
        Data = (InIndex & INDEX_MASK) | ((InGeneration & GENERATION_MASK) << INDEX_BITS);
    }

    bool IsValid() const
    {
        uint32 Index = GetIndex();
        uint32 Generation = GetGeneration();
        return Index != INDEX_MASK && Generation > 0;
    }

    uint32 GetIndex() const { return Data & INDEX_MASK; }
    uint32 GetGeneration() const { return (Data >> INDEX_BITS) & GENERATION_MASK; }

    bool operator==(const FSparseGridHandle& Other) const { return Data == Other.Data; }
    bool operator!=(const FSparseGridHandle& Other) const { return Data != Other.Data; }

    friend uint32 GetTypeHash(const FSparseGridHandle& Handle) { return Handle.Data; }

    FString ToString() const
    {
        return FString::Printf(TEXT("Idx:%d, Gen:%d"), GetIndex(), GetGeneration());
    }

private:
    static constexpr uint32 INDEX_BITS = 24;
    static constexpr uint32 GENERATION_BITS = 8;
    static constexpr uint32 INDEX_MASK = (1 << INDEX_BITS) - 1;
    static constexpr uint32 GENERATION_MASK = (1 << GENERATION_BITS) - 1;

    UPROPERTY()
    uint32 Data;
};