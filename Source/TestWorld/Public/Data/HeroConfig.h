#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HeroConfig.generated.h"

UCLASS(BlueprintType)
class TESTWORLD_API UHeroConfig : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraDistance = 10000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraPitch = -45.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraYaw = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|TiltShift")
	bool bEnableTiltShift = true;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|TiltShift")
	float TiltShiftFocusDistance = 10000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|TiltShift")
	float TiltShiftFocusRegion = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|TiltShift")
	float TiltShiftBlurAmount = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	float RespawnDelay = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	bool bAutoRespawn = true;
};
