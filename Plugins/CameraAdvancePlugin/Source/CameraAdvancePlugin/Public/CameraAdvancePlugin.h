#pragma once

#include "CoreMinimal.h"
#include "CameraAdvancePlugin.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCameraAdvancePlugin, Log, All);

class CAMERAADVANCEPLUGIN_API FCameraAdvancePluginModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
