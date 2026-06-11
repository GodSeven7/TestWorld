#include "SparseGridPlugin.h"

IMPLEMENT_MODULE(FSparseGridPluginModule, SparseGridPlugin)

void FSparseGridPluginModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("SparseGridPlugin module started"));
}

void FSparseGridPluginModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("SparseGridPlugin module shutdown"));
}