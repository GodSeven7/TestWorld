#pragma once

#include "CoreMinimal.h"
#include "GamePawn.h"
#include "GameCharacter.generated.h"

class UCharacterSkelMeshComponent;
class UCharacterAnimData;
class UCombatComponent;

UCLASS()
class TESTWORLD_API AGameCharacter : public AGamePawn
{
    GENERATED_BODY()

public:
    AGameCharacter();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void HandleDeath(const FDamageInfo& KillingBlow) override;
    virtual void OnActorPoolSpawn() override;
    virtual void ResetActorPoolState() override;

public:
    UCharacterSkelMeshComponent* GetMeshComponent() const { return MeshComponent; }

    UFUNCTION(BlueprintCallable, Category = "Movement")
    void MoveInDirection(const FVector& WorldDirection, float DeltaTime);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MoveSpeed = 500.0f;

	UFUNCTION(BlueprintCallable, Category = "Config")
	void SetCharacterTypeID(int32 InTypeID) { CharacterTypeID = InTypeID; }

	UFUNCTION(BlueprintPure, Category = "Config")
	int32 GetCharacterTypeID() const { return CharacterTypeID; }

	float GetMoveSpeedFromConfig() const;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    TSubclassOf<class UCombatWeapon> DefaultWeaponClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    TObjectPtr<UCharacterAnimData> AnimDataAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    int32 MeshType = 0;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UCharacterSkelMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> MeshRootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> MeshRootSubComponent;
	
private:
    int32 ManagerID = INDEX_NONE;
	int32 CharacterTypeID = -1;
};
