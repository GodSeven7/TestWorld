#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCharacterInstancingPlugin, Log, All);

class CHARACTERINSTANCINGPLUGIN_API FCharacterInstancingPluginModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
