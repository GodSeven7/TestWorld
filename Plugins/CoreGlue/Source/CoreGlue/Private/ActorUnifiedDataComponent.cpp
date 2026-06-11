#include "ActorUnifiedDataComponent.h"
#include "GameFramework/Actor.h"
#include "UnifiedDataRegistry.h"
#include "Subsystems/WorldSubsystem.h"

UActorUnifiedDataComponent::UActorUnifiedDataComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	static int32 NextObjectID = 1;
	CurrentData.ObjectID = NextObjectID++;
}

void UActorUnifiedDataComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UActorUnifiedDataComponent::InitializeUnifiedData()
{
	SyncFromActor();

	if (UWorld* World = GetWorld())
	{
		World->ForEachSubsystem<UWorldSubsystem>([this](UWorldSubsystem* WorldSubsystem)
			{
				if (IUnifiedDataRegistry* Registry = Cast<IUnifiedDataRegistry>(WorldSubsystem))
				{
					Registry->RegisterUnifiedDataComponent(this);
					CachedRegistry = Registry;
				}
			});
	}
}

void UActorUnifiedDataComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->ForEachSubsystem<UWorldSubsystem>([this](UWorldSubsystem* WorldSubsystem)
			{
				if (IUnifiedDataRegistry* Registry = Cast<IUnifiedDataRegistry>(WorldSubsystem))
				{
					Registry->UnregisterUnifiedDataComponent(this);
				}
			});
	}

	Super::EndPlay(EndPlayReason);
}

void UActorUnifiedDataComponent::CreateSnapshot()
{
	SnapshotData = CurrentData;

	for (IUnifiedDataExtension* Extension : Extensions)
	{
		Extension->CreateExtensionSnapshot();
	}
}

void UActorUnifiedDataComponent::ApplyToActor(float DeltaTime)
{
	bool bHasMovement = CurrentData.MovementVelocity.SizeSquared() > 0.0f;

	if (bHasMovement)
	{
		CurrentData.Position += CurrentData.MovementVelocity * DeltaTime;
		// Velocity is not cleared here; preserved for next frame's Steering prediction
		CurrentData.MarkDirty(EUnifiedDataDirtyFlag::Position);
	}

	// Update bIsMoving state for animation system
	bool bCurrentlyMoving = CurrentData.MovementVelocity.SizeSquared() > 1.0f;
	if (CurrentData.bIsMoving != bCurrentlyMoving)
	{
		CurrentData.bIsMoving = bCurrentlyMoving;
		CurrentData.MarkDirty(EUnifiedDataDirtyFlag::IsMoving);
	}

	if (CurrentData.IsDirty(EUnifiedDataDirtyFlag::Position) ||
		CurrentData.IsDirty(EUnifiedDataDirtyFlag::Rotation))
	{
		if (AssociatedComponent)
		{
			if (CurrentData.IsDirty(EUnifiedDataDirtyFlag::Position))
				AssociatedComponent->SetWorldLocation(CurrentData.Position);
			if (CurrentData.IsDirty(EUnifiedDataDirtyFlag::Rotation))
				AssociatedComponent->SetWorldRotation(CurrentData.Rotation);
		}
		else if (GetOwner())
		{
			GetOwner()->SetActorLocationAndRotation(CurrentData.Position, CurrentData.Rotation);
		}
	}

	// 处理 Faction 变更通知
	if (CurrentData.IsDirty(EUnifiedDataDirtyFlag::Faction))
	{
		int32 OldFaction = SnapshotData.Faction;
		int32 NewFaction = CurrentData.Faction;
		if (OldFaction != NewFaction && CachedRegistry)
		{
			CachedRegistry->NotifyFactionChanged(this, OldFaction, NewFaction);
		}
	}

	CurrentData.ClearDirty();

	for (IUnifiedDataExtension* Extension : Extensions)
	{
		Extension->ApplyExtensionToActor(DeltaTime);
	}
}

void UActorUnifiedDataComponent::MarkDirty(EUnifiedDataDirtyFlag Flag)
{
	CurrentData.MarkDirty(Flag);
}

void UActorUnifiedDataComponent::SetPosition(const FVector& InPosition)
{
	CurrentData.Position = InPosition;
	CurrentData.MarkDirty(EUnifiedDataDirtyFlag::Position);
}

void UActorUnifiedDataComponent::SetRotation(const FRotator& InRotation)
{
	CurrentData.Rotation = InRotation;
	CurrentData.MarkDirty(EUnifiedDataDirtyFlag::Rotation);
}

void UActorUnifiedDataComponent::SetForwardVector(const FVector& InForward)
{
	CurrentData.ForwardVector = InForward;
	CurrentData.MarkDirty(EUnifiedDataDirtyFlag::ForwardVector);
}

void UActorUnifiedDataComponent::SetFaction(int32 InFaction)
{
	CurrentData.Faction = InFaction;
	CurrentData.MarkDirty(EUnifiedDataDirtyFlag::Faction);
}

void UActorUnifiedDataComponent::SetAlive(bool bAlive)
{
	CurrentData.bIsAlive = bAlive;
	CurrentData.MarkDirty(EUnifiedDataDirtyFlag::IsAlive);
}

void UActorUnifiedDataComponent::SetObjectType(int32 InType)
{
	CurrentData.ObjectType = InType;
	CurrentData.MarkDirty(EUnifiedDataDirtyFlag::ObjectType);
}

bool UActorUnifiedDataComponent::HasTarget() const
{
	return CurrentData.TargetInfo.HasTarget();
}

int32 UActorUnifiedDataComponent::GetTargetID() const
{
	return CurrentData.TargetInfo.TargetID;
}

FVector UActorUnifiedDataComponent::GetTargetPosition() const
{
	return CurrentData.TargetInfo.TargetPosition;
}

float UActorUnifiedDataComponent::GetTargetDistance() const
{
	return CurrentData.TargetInfo.TargetDistance;
}

void UActorUnifiedDataComponent::SetTarget(int32 InTargetID, const FVector& InTargetPosition, float InDistance)
{
	CurrentData.TargetInfo.TargetID = InTargetID;
	CurrentData.TargetInfo.TargetPosition = InTargetPosition;
	CurrentData.TargetInfo.TargetDistance = InDistance;
	CurrentData.MarkDirty(EUnifiedDataDirtyFlag::TargetInfo);
}

void UActorUnifiedDataComponent::ClearTarget()
{
	CurrentData.TargetInfo.Clear();
	CurrentData.MarkDirty(EUnifiedDataDirtyFlag::TargetInfo);
}

FVector UActorUnifiedDataComponent::GetMovementVelocity() const
{
	return CurrentData.MovementVelocity;
}

void UActorUnifiedDataComponent::SetMovementVelocity(const FVector& InVelocity)
{
	CurrentData.MovementVelocity = InVelocity;
}

void UActorUnifiedDataComponent::ClearMovementVelocity()
{
	CurrentData.MovementVelocity = FVector::ZeroVector;
}

float UActorUnifiedDataComponent::GetMoveSpeed() const
{
	return CurrentData.MoveSpeed;
}

void UActorUnifiedDataComponent::SetMoveSpeed(float InMoveSpeed)
{
	CurrentData.MoveSpeed = InMoveSpeed;
}

FVector UActorUnifiedDataComponent::GetDesiredPosition() const
{
	return CurrentData.DesiredPosition;
}

void UActorUnifiedDataComponent::SetDesiredPosition(const FVector& InPosition)
{
	CurrentData.DesiredPosition = InPosition;
	CurrentData.bHasDesiredPosition = !InPosition.IsNearlyZero();
}

bool UActorUnifiedDataComponent::HasDesiredPosition() const
{
	return CurrentData.bHasDesiredPosition;
}

FVector UActorUnifiedDataComponent::GetCorrectedDesiredPosition() const
{
	return CurrentData.CorrectedDesiredPosition;
}

void UActorUnifiedDataComponent::SetCorrectedDesiredPosition(const FVector& InPosition)
{
	CurrentData.CorrectedDesiredPosition = InPosition;
	CurrentData.bHasCorrectedDesiredPosition = !InPosition.IsNearlyZero();
}

bool UActorUnifiedDataComponent::HasCorrectedDesiredPosition() const
{
	return CurrentData.bHasCorrectedDesiredPosition;
}

void UActorUnifiedDataComponent::ClearCorrectedDesiredPosition()
{
	CurrentData.CorrectedDesiredPosition = FVector::ZeroVector;
	CurrentData.bHasCorrectedDesiredPosition = false;
}

bool UActorUnifiedDataComponent::IsMoving() const
{
	return CurrentData.bIsMoving;
}

void UActorUnifiedDataComponent::SetIsMoving(bool bInMoving)
{
	CurrentData.bIsMoving = bInMoving;
	CurrentData.MarkDirty(EUnifiedDataDirtyFlag::IsMoving);
}

bool UActorUnifiedDataComponent::IsNavigationObstacle() const
{
	return CurrentData.bIsNavigationObstacle;
}

void UActorUnifiedDataComponent::SetNavigationObstacle(bool bObstacle)
{
	CurrentData.bIsNavigationObstacle = bObstacle;
}

void UActorUnifiedDataComponent::SyncFromActor()
{
	if (AssociatedComponent)
	{
		CurrentData.Position = AssociatedComponent->GetComponentLocation();
		CurrentData.Rotation = AssociatedComponent->GetComponentRotation();
		CurrentData.ForwardVector = AssociatedComponent->GetForwardVector();
	}
	else if (GetOwner())
	{
		CurrentData.Position = GetOwner()->GetActorLocation();
		CurrentData.Rotation = GetOwner()->GetActorRotation();
		CurrentData.ForwardVector = GetOwner()->GetActorForwardVector();
	}
	CurrentData.ClearDirty();
}

void UActorUnifiedDataComponent::RegisterExtension(IUnifiedDataExtension* Extension)
{
	if (Extension && !Extensions.Contains(Extension))
	{
		Extensions.Add(Extension);
	}
}

void UActorUnifiedDataComponent::UnregisterExtension(IUnifiedDataExtension* Extension)
{
	Extensions.Remove(Extension);
}
