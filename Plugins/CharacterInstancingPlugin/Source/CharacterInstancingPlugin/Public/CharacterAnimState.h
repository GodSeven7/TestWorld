#pragma once

#include "CoreMinimal.h"
#include "CharacterAnimState.generated.h"

USTRUCT(BlueprintType)
struct FCharacterAnimState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    FName CurrentStateName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    float CurrentTime = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float PlayRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    bool bLooping = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    bool bPlaying = true;

    void AdvanceTime(float DeltaTime, float AnimLength)
    {
        if (!bPlaying || AnimLength <= 0.f)
            return;

        CurrentTime += DeltaTime * PlayRate;

        if (bLooping)
        {
            CurrentTime = FMath::Fmod(CurrentTime, AnimLength);
            if (CurrentTime < 0.f)
                CurrentTime += AnimLength;
        }
        else
        {
            if (CurrentTime >= AnimLength)
            {
                CurrentTime = AnimLength;
                bPlaying = false;
            }
            else if (CurrentTime < 0.f)
            {
                CurrentTime = 0.f;
            }
        }
    }

    void Reset()
    {
        CurrentTime = 0.f;
        bPlaying = true;
    }

    bool IsValid() const
    {
        return CurrentStateName != NAME_None;
    }
};
