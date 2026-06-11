#include "CameraAdvancePlugin.h"

DEFINE_LOG_CATEGORY(LogCameraAdvancePlugin);

void FCameraAdvancePluginModule::StartupModule()
{
    UE_LOG(LogCameraAdvancePlugin, Log, TEXT("CameraAdvancePlugin module started"));
}

void FCameraAdvancePluginModule::ShutdownModule()
{
    UE_LOG(LogCameraAdvancePlugin, Log, TEXT("CameraAdvancePlugin module shutdown"));
}

IMPLEMENT_MODULE(FCameraAdvancePluginModule, CameraAdvancePlugin)
