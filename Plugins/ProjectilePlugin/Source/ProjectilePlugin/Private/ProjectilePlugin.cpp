#include "ProjectilePlugin.h"
#include "ProjectileDebug.h"

DEFINE_LOG_CATEGORY(LogProjectilePlugin);

void FProjectilePluginModule::StartupModule()
{
    UE_LOG(LogProjectilePlugin, Log, TEXT("ProjectilePlugin module started"));
    FProjectileDebug::Initialize();
}

void FProjectilePluginModule::ShutdownModule()
{
    FProjectileDebug::Deinitialize();
    UE_LOG(LogProjectilePlugin, Log, TEXT("ProjectilePlugin module shutdown"));
}

IMPLEMENT_MODULE(FProjectilePluginModule, ProjectilePlugin)
