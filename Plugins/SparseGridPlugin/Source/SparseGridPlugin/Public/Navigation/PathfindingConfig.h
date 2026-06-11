#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PathfindingConfig.generated.h"

UCLASS()
class SPARSEGRIDPLUGIN_API UPathfindingConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    // FlowField搜索的世界空间半径（世界单位），运行时自动换算为Cell数量
    UPROPERTY(EditDefaultsOnly, Category = "FlowField")
    float FlowFieldWorldRange = 5000.0f;

    UPROPERTY(EditDefaultsOnly, Category = "FlowField")
    int32 MaxFlowFieldCacheCount = 8;

    UPROPERTY(EditDefaultsOnly, Category = "FlowField")
    float CacheInvalidateInterval = 2.0f;

    UPROPERTY(EditDefaultsOnly, Category = "PathSmoother")
    bool bEnablePathSmoothing = true;

    UPROPERTY(EditDefaultsOnly, Category = "Threading")
    int32 SyncMaxNodes = 5000;
};
