// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SparseGridObject.h"
#include "SparseGridHandle.h"
#include "SparseGridComponent.generated.h"

UCLASS(ClassGroup = "SparseGrid", meta = (BlueprintSpawnableComponent))
class SPARSEGRIDPLUGIN_API USparseGridComponent : public UActorComponent, public ISparseGridObject
{
    GENERATED_BODY()

public:
    USparseGridComponent();

	virtual AActor* GetSparseGridOwnerActor() const { return GetOwner(); }

	virtual UObject* QueryUObject() const override { return const_cast<USparseGridComponent*>(this); }

    virtual FVector GetSparseGridPosition() const override;
    virtual FVector GetSparseGridForwardVector() const override;
    virtual UObject* GetSparseGridParent() const override;
    virtual bool IsCrowdPushLocked() const override;

	virtual FSparseGridHandle GetGridHandle() const { return GridHandle; }

	virtual int32 GetObjectID() const override;

    void SetAssociatedComponent(USceneComponent* Component) { AssociatedComponent = Component; }
    USceneComponent* GetAssociatedComponent() const { return AssociatedComponent; }

    void SetSparseGridActive(bool bActive)
    {
        SetSparseGridObjectActive(bActive);
        SetEnableSparseGridCollision(bActive);
    }

    bool IsSparseGridActive() const { return IsSparseGridObjectActive(); }

    UFUNCTION(BlueprintCallable, Category = "SparseGrid")
    void RegisterToGrid();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    FSparseGridHandle GridHandle;
    
    UPROPERTY()
    USceneComponent* AssociatedComponent = nullptr;
};
