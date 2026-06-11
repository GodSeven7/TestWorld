// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameCharacter.h"
#include "InputActionValue.h"
#include "Data/HeroConfig.h"
#include "GameHero.generated.h"

class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCineCameraComponent;

UCLASS()
class TESTWORLD_API AGameHero : public AGameCharacter
{
    GENERATED_BODY()

public:
    AGameHero();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void OnPrimaryAction(const FInputActionValue& Value);
    void OnSecondaryAction(const FInputActionValue& Value);

public:
    USpringArmComponent* GetCameraSpringArm() const { return CameraSpringArm; }
    UCineCameraComponent* GetCameraComponent() const { return CameraComponent; }

	UFUNCTION(BlueprintCallable, Category = "Movement")
	FVector GetCameraRelativeMovement(const FVector2D& InputVector, float InCameraYaw) const;

    UFUNCTION(BlueprintCallable, Category = "Respawn")
    void SetSpawnLocation(const FVector& Location) { SpawnLocation = Location; }

    UFUNCTION(BlueprintCallable, Category = "Respawn")
    FVector GetSpawnLocation() const { return SpawnLocation; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Input")
    float GetLastInputTime() const { return LastInputTime; }

protected:
    virtual void HandleDeath(const FDamageInfo& KillingBlow) override;

    void SaveInitialSpawnLocation();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USpringArmComponent* CameraSpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCineCameraComponent* CameraComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputMappingContext* InputMappingContext = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* MoveAction = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* LookAction = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* PrimaryAction = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* SecondaryAction = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float CameraDistance = 10000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float CameraPitch = -45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float CameraYaw = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|TiltShift")
    bool bEnableTiltShift = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|TiltShift")
    float TiltShiftFocusDistance = 10000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|TiltShift")
    float TiltShiftFocusRegion = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|TiltShift")
    float TiltShiftBlurAmount = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn")
    FVector SpawnLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn")
    float RespawnDelay = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn")
    bool bAutoRespawn = true;

	float LastInputTime = 0.f;

public:

	UFUNCTION(BlueprintCallable, Category = "Config")
	void SetHeroConfig(UHeroConfig* InConfig) { HeroConfig = InConfig; }

	UFUNCTION(BlueprintPure, Category = "Config")
	UHeroConfig* GetHeroConfig() const { return HeroConfig; }

	float GetConfigCameraDistance() const { return HeroConfig ? HeroConfig->CameraDistance : 10000.0f; }
	float GetConfigCameraPitch() const { return HeroConfig ? HeroConfig->CameraPitch : -45.0f; }
	float GetConfigCameraYaw() const { return HeroConfig ? HeroConfig->CameraYaw : 0.0f; }
	bool IsConfigTiltShiftEnabled() const { return HeroConfig ? HeroConfig->bEnableTiltShift : true; }
	float GetConfigTiltShiftFocusDistance() const { return HeroConfig ? HeroConfig->TiltShiftFocusDistance : 10000.0f; }
	float GetConfigRespawnDelay() const { return HeroConfig ? HeroConfig->RespawnDelay : 0.5f; }
	bool ShouldConfigAutoRespawn() const { return HeroConfig ? HeroConfig->bAutoRespawn : true; }

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	UHeroConfig* HeroConfig;
};
