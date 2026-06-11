#pragma once

#include "CoreMinimal.h"

class UWorld;
class USparseGridManager;

namespace SparseGridDebug
{
	SPARSEGRIDPLUGIN_API bool IsDebugEnabled();
	
	SPARSEGRIDPLUGIN_API bool IsCollisionEnabled();
	
	SPARSEGRIDPLUGIN_API bool IsCollisionDebugEnabled();
	
	SPARSEGRIDPLUGIN_API bool IsDebugForwardEnabled();
	
	SPARSEGRIDPLUGIN_API void SetCollisionEnabled(bool bEnabled);
	
	SPARSEGRIDPLUGIN_API void DrawDebug(UWorld* World, USparseGridManager* GridManager);
	
	SPARSEGRIDPLUGIN_API bool IsRaycastTestEnabled();
	
	SPARSEGRIDPLUGIN_API void SetRaycastTestEnabled(bool bEnabled);
	
	SPARSEGRIDPLUGIN_API float GetRaycastTestDistance();
	
	SPARSEGRIDPLUGIN_API void SetRaycastTestDistance(float Distance);
	
	SPARSEGRIDPLUGIN_API void DrawRaycastTest(UWorld* World, USparseGridManager* GridManager);

	SPARSEGRIDPLUGIN_API bool IsFlowFieldPathDebugEnabled();
	
	SPARSEGRIDPLUGIN_API bool IsFlowFieldDirectionDebugEnabled();

	SPARSEGRIDPLUGIN_API bool IsSteeringDisabled();

	SPARSEGRIDPLUGIN_API bool IsCrowdDebugEnabled();

	SPARSEGRIDPLUGIN_API bool IsPBDDebugEnabled();

	SPARSEGRIDPLUGIN_API bool IsSteeringTargetDebugEnabled();

	SPARSEGRIDPLUGIN_API void DrawFlowFieldDebug(UWorld* World, USparseGridManager* GridManager);
}
