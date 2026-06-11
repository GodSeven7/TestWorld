#include "Subsystems/WorldGridSubsystem.h"
#include "Coordinator/PluginCoordinator.h"
#include "SparseGridManager.h"
#include "ActorUnifiedDataComponent.h"

void UWorldGridSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Coordinator = GetWorld()->GetSubsystem<UPluginCoordinator>();

	UE_LOG(LogTemp, Log, TEXT("WorldGridSubsystem initialized (thin proxy mode)"));
}

void UWorldGridSubsystem::Deinitialize()
{
	Coordinator = nullptr;
	Super::Deinitialize();
}

void UWorldGridSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

TStatId UWorldGridSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UWorldGridSubsystem, STATGROUP_Tickables);
}

TArray<ISparseGridObject*> UWorldGridSubsystem::GetAllSparseGridObjects() const
{
	return Coordinator ? Coordinator->GetAllSparseGridObjects() : TArray<ISparseGridObject*>();
}

void UWorldGridSubsystem::DestroyObject(const FSparseGridHandle& Handle)
{
	if (Coordinator) Coordinator->DestroyObject(Handle);
}

void UWorldGridSubsystem::UpdateObjectPosition(const FSparseGridHandle& Handle, const FVector& NewPosition)
{
	if (Coordinator) Coordinator->UpdateObjectPosition(Handle, NewPosition);
}

TArray<FSparseGridHandle> UWorldGridSubsystem::QuerySphere(const FVector& Center, float Radius) const
{
	return Coordinator ? Coordinator->QuerySphere(Center, Radius) : TArray<FSparseGridHandle>();
}

TArray<ISparseGridObject*> UWorldGridSubsystem::QuerySparseGridObjectsInSphere(const FVector& Center, float Radius) const
{
	return Coordinator ? Coordinator->QuerySparseGridObjectsInSphere(Center, Radius) : TArray<ISparseGridObject*>();
}

USparseGridManager* UWorldGridSubsystem::GetGridManager() const
{
	return Coordinator ? Coordinator->GetGridManager() : nullptr;
}

bool UWorldGridSubsystem::IsInitialized() const
{
	return Coordinator ? Coordinator->GetGridManager() != nullptr : false;
}

int32 UWorldGridSubsystem::GetTrackedActorCount() const
{
	return Coordinator ? Coordinator->GetTrackedActorCount() : 0;
}

void UWorldGridSubsystem::ClearAll()
{
	if (Coordinator) Coordinator->ClearAll();
}

void UWorldGridSubsystem::SetCollisionEnabled(bool bEnabled)
{
	if (Coordinator) Coordinator->SetCollisionEnabled(bEnabled);
}

bool UWorldGridSubsystem::IsCollisionEnabled() const
{
	return Coordinator ? Coordinator->IsCollisionEnabled() : false;
}

void UWorldGridSubsystem::SetCollisionMatrix(const FSparseGridCollisionMatrix& InMatrix)
{
	if (Coordinator) Coordinator->SetCollisionMatrix(InMatrix);
}

FSparseGridCollisionMatrix UWorldGridSubsystem::GetCollisionMatrix() const
{
	return Coordinator ? Coordinator->GetCollisionMatrix() : FSparseGridCollisionMatrix();
}

FSparseGridCollisionStats UWorldGridSubsystem::GetCollisionStats() const
{
	return Coordinator ? Coordinator->GetCollisionStats() : FSparseGridCollisionStats();
}

int32 UWorldGridSubsystem::GetActiveCollisionCount() const
{
	return Coordinator ? Coordinator->GetActiveCollisionCount() : 0;
}

AActor* UWorldGridSubsystem::FindNearestActor(const FVector& Position, float MaxRadius) const
{
	return Coordinator ? Coordinator->FindNearestActor(Position, MaxRadius) : nullptr;
}

UProjectileManager* UWorldGridSubsystem::GetProjectileManager() const
{
	return Coordinator ? Coordinator->GetProjectileManager() : nullptr;
}

void UWorldGridSubsystem::SetGameplaySuspended(bool bSuspended)
{
	if (Coordinator) Coordinator->SetGameplaySuspended(bSuspended);
}

bool UWorldGridSubsystem::IsGameplaySuspended() const
{
	return Coordinator ? Coordinator->IsGameplaySuspended() : false;
}

void UWorldGridSubsystem::RegisterUnifiedDataComponent(UActorUnifiedDataComponent* Component)
{
	if (Coordinator) Coordinator->RegisterUnifiedDataComponent(Component);
	if (Component && !UnifiedDataComponents.Contains(Component))
	{
		UnifiedDataComponents.Add(Component);
	}
}

void UWorldGridSubsystem::UnregisterUnifiedDataComponent(UActorUnifiedDataComponent* Component)
{
	if (Coordinator) Coordinator->UnregisterUnifiedDataComponent(Component);
	if (Component)
	{
		UnifiedDataComponents.RemoveSwap(Component);
	}
}

const TArray<TObjectPtr<UActorUnifiedDataComponent>>& UWorldGridSubsystem::GetUnifiedDataComponents() const
{
	return UnifiedDataComponents;
}
