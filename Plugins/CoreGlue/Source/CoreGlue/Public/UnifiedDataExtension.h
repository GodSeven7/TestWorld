#pragma once

#include "CoreMinimal.h"

class UActorUnifiedDataComponent;

/**
 * 统一数据扩展接口
 * 由 CoreGlue 的 UActorUnifiedDataComponent 统一触发 Snapshot 和 Apply
 * 项目层的扩展组件实现此接口，存储插件专属数据
 */
class COREGLUE_API IUnifiedDataExtension
{
public:
	virtual ~IUnifiedDataExtension() = default;

	virtual void CreateExtensionSnapshot() = 0;
	virtual void ApplyExtensionToActor(float DeltaTime) = 0;
};
