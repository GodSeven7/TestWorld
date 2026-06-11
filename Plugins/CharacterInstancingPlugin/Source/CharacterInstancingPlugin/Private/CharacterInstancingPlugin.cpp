#include "CharacterInstancingPlugin.h"

DEFINE_LOG_CATEGORY(LogCharacterInstancingPlugin);

void FCharacterInstancingPluginModule::StartupModule()
{
    UE_LOG(LogCharacterInstancingPlugin, Log, TEXT("CharacterInstancingPlugin module started"));
}

void FCharacterInstancingPluginModule::ShutdownModule()
{
    UE_LOG(LogCharacterInstancingPlugin, Log, TEXT("CharacterInstancingPlugin module shutdown"));
}

IMPLEMENT_MODULE(FCharacterInstancingPluginModule, CharacterInstancingPlugin)
