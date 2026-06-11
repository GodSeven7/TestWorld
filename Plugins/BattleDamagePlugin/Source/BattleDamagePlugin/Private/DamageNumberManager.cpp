#include "DamageNumberManager.h"
#include "NiagaraComponent.h"
#include "Engine/World.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogDamageNumberManager, Log, All);

void UDamageNumberManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    CurrentSettings.MaxDamageNumberCount = DEFAULT_MAX_COUNT;

    LoadActorFromConfig();
}

void UDamageNumberManager::Deinitialize()
{
    ClearAllNumbers();

    if (DamageNumberActor)
    {
        DamageNumberActor->Destroy();
        DamageNumberActor = nullptr;
        NiagaraComponent = nullptr;
    }

    DamageNumberActorClass = nullptr;
    bIsInitialized = false;
    bIsEnabled = false;

    Super::Deinitialize();
}

bool UDamageNumberManager::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UDamageNumberManager::LoadActorFromConfig()
{
    FString ConfigPath = TEXT("/Script/BattleDamagePlugin.BattleDamageSettings");

    FString ActorPath;
    if (!GConfig || !GConfig->GetString(*ConfigPath, TEXT("DamageNumberActorPath"), ActorPath, GGameIni))
    {
        UE_LOG(LogDamageNumberManager, Log, TEXT("No INI config found for DamageNumberActorPath"));
        bIsEnabled = false;
        return;
    }

    DamageNumberActorClass = LoadClass<AActor>(nullptr, *ActorPath);
    if (!DamageNumberActorClass)
    {
        UE_LOG(LogDamageNumberManager, Warning, TEXT("Failed to load Blueprint: %s"), *ActorPath);
        bIsEnabled = false;
        return;
    }

    bIsEnabled = SpawnActorFromBlueprint();
}

bool UDamageNumberManager::SpawnActorFromBlueprint()
{
    UWorld* World = GetWorld();
    if (!World || !DamageNumberActorClass)
    {
        return false;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = FName(TEXT("DamageNumberActor"));
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    DamageNumberActor = World->SpawnActor<AActor>(DamageNumberActorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    if (!DamageNumberActor)
    {
        UE_LOG(LogDamageNumberManager, Warning, TEXT("Failed to spawn DamageNumberActor"));
        return false;
    }

    NiagaraComponent = DamageNumberActor->FindComponentByClass<UNiagaraComponent>();
    if (!NiagaraComponent)
    {
        UE_LOG(LogDamageNumberManager, Warning, TEXT("No NiagaraComponent found in Blueprint"));
        DamageNumberActor->Destroy();
        DamageNumberActor = nullptr;
        return false;
    }

    bIsInitialized = true;
    UE_LOG(LogDamageNumberManager, Log, TEXT("Initialized from Blueprint"));
    return true;
}

void UDamageNumberManager::EnqueueDamageNumber(const FVector& WorldLocation, float Damage, bool bIsCritical)
{
    if (!bIsEnabled || !bIsInitialized)
    {
        return;
    }

    FVector4 NewInfo;
    NewInfo.X = WorldLocation.X;
    NewInfo.Y = WorldLocation.Y;
    NewInfo.Z = WorldLocation.Z;

    float RawDamage = FMath::FloorToFloat(Damage);
    NewInfo.W = bIsCritical ? -RawDamage : RawDamage;

    FScopeLock Lock(&PendingLock);
    PendingDamageNumbers.Add(NewInfo);
}

void UDamageNumberManager::FlushToNiagara()
{
    if (!bIsEnabled || !bIsInitialized || !NiagaraComponent)
    {
        FScopeLock Lock(&PendingLock);
        PendingDamageNumbers.Empty();
        return;
    }

    TArray<FVector4> LocalPending;
    {
        FScopeLock Lock(&PendingLock);
        LocalPending = MoveTemp(PendingDamageNumbers);
        PendingDamageNumbers.Empty();
    }

    if (LocalPending.Num() == 0)
    {
        return;
    }

    TArray<FVector4> CurrentArray = UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayVector4(NiagaraComponent, FName("User.DamageInfo"));

    int32 MaxCount = CurrentSettings.MaxDamageNumberCount > 0 ? CurrentSettings.MaxDamageNumberCount : DEFAULT_MAX_COUNT;

    for (const FVector4& NewDamage : LocalPending)
    {
        if (CurrentArray.Num() >= MaxCount)
        {
            CurrentArray.RemoveAt(0);
        }
        CurrentArray.Add(NewDamage);
    }

    UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(NiagaraComponent, FName("User.DamageInfo"), CurrentArray);
}

void UDamageNumberManager::ClearAllNumbers()
{
    FScopeLock Lock(&PendingLock);
    PendingDamageNumbers.Empty();

    if (NiagaraComponent)
    {
        UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(NiagaraComponent, FName("User.DamageInfo"), TArray<FVector4>());
    }
}
