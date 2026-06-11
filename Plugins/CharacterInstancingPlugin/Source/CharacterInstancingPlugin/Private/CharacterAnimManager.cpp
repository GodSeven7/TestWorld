#include "CharacterAnimManager.h"
#include "CharacterInstancingPlugin.h"
#include "CharacterSkelMeshComponent.h"
#include "CharacterAnimData.h"
#include "Engine/World.h"
#include "Async/ParallelFor.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

void UCharacterAnimManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    PreAllocateInstances(INITIAL_CAPACITY);
    UE_LOG(LogCharacterInstancingPlugin, Log, TEXT("CharacterAnimManager initialized (capacity: %d)"), INITIAL_CAPACITY);
}

void UCharacterAnimManager::Deinitialize()
{
    Instances.Empty();
    FreeIndices.Empty();
    CharacterIDToIndex.Empty();
    StateBits.ClearAll();
    IndexCache.Clear();
    Super::Deinitialize();
}

bool UCharacterAnimManager::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UCharacterAnimManager::PreAllocateInstances(int32 Count)
{
    int32 OldCount = Instances.Num();
    if (OldCount >= Count) return;

    Instances.SetNum(Count);
    StateBits.ResizeAll(OldCount, Count);
    for (int32 i = OldCount; i < Count; i++)
    {
        FreeIndices.Add(i);
    }
}

int32 UCharacterAnimManager::AllocateSlot()
{
    FScopeLock Lock(&SlotLock);
    if (FreeIndices.Num() == 0)
    {
        int32 OldCount = Instances.Num();
        int32 NewCount = OldCount + INITIAL_CAPACITY;
        PreAllocateInstances(NewCount);
    }
    int32 Index = FreeIndices.Pop();
    return Index;
}

void UCharacterAnimManager::ReleaseSlot(int32 Index)
{
    FScopeLock Lock(&SlotLock);
    FreeIndices.Add(Index);
}

int32 UCharacterAnimManager::RegisterComponent(UCharacterSkelMeshComponent* Comp)
{
    if (!Comp) return INDEX_NONE;

    int32 ID = NextCharacterID++;
    int32 Index = AllocateSlot();

    auto& Data = Instances[Index];
    Data.CharacterID = ID;
    Data.SkelMeshComp = Comp;
    Data.Transform = FTransform(Comp->GetComponentRotation(), Comp->GetComponentLocation());
    Data.PrevPosition = Comp->GetComponentLocation();

    StateBits.IsActive[Index] = true;
    CharacterIDToIndex.Add(ID, Index);

    return ID;
}

void UCharacterAnimManager::UnregisterComponent(int32 CharacterID)
{
    int32* IndexPtr = CharacterIDToIndex.Find(CharacterID);
    if (!IndexPtr) return;
    int32 Index = *IndexPtr;

    StateBits.IsActive[Index] = false;
    Instances[Index].Deactivate();
    CharacterIDToIndex.Remove(CharacterID);
    ReleaseSlot(Index);
}

int32 UCharacterAnimManager::GetActiveCharacterCount() const
{
    return CharacterIDToIndex.Num();
}

void UCharacterAnimManager::Tick(float DeltaTime)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UCharacterAnimManager_Tick);

    CollectActiveIndices();
    if (IndexCache.ActiveIndices.Num() == 0) return;

    if (bGameplaySuspended) return;

    ResolveAnimStates();
    AdvanceTimeAndSyncTransforms(DeltaTime);
    EvaluateAnimationParallel();
    ApplyBoneTransforms();
}

void UCharacterAnimManager::CollectActiveIndices()
{
    IndexCache.Clear();
    int32 Count = Instances.Num();
    for (int32 i = 0; i < Count; i++)
    {
        if (StateBits.IsActive[i])
        {
            IndexCache.ActiveIndices.Add(i);
        }
    }
}

void UCharacterAnimManager::ResolveAnimStates()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ResolveAnimStates);

    for (int32 Index : IndexCache.ActiveIndices)
    {
        auto& Data = Instances[Index];
        UCharacterSkelMeshComponent* Comp = Data.GetComponent();
        if (!Comp) continue;

        ICharacterDataSource* Source = Data.GetDataSource();
        if (!Source) continue;

        UCharacterAnimData* AnimConfig = Comp->GetAnimData();
        if (!AnimConfig) continue;

        FName TargetState;

        if (Source->IsDead())
        {
            TargetState = AnimConfig->DeathStateName;
        }
        else if (Source->IsAttacking())
        {
            TargetState = AnimConfig->AttackStateName;
        }
        else if (Source->IsMoving())
        {
            if (Data.MovementSpeed >= AnimConfig->RunSpeedThreshold)
            {
                TargetState = AnimConfig->RunStateName;
            }
            else
            {
                TargetState = AnimConfig->WalkStateName;
            }
        }
        else
        {
            TargetState = AnimConfig->DefaultStateName;
        }

        if (Comp->GetCurrentState() != TargetState)
        {
            Comp->SetAnimState(TargetState);
        }
    }
}

void UCharacterAnimManager::AdvanceTimeAndSyncTransforms(float DeltaTime)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(AdvanceTimeAndSyncTransforms);

    const float ScaledDelta = DeltaTime * GlobalTimeDilation;

    for (int32 Index : IndexCache.ActiveIndices)
    {
        auto& Data = Instances[Index];
        UCharacterSkelMeshComponent* Comp = Data.GetComponent();
        if (!Comp) continue;

        if (Comp->IsPlayingAnim())
        {
            Comp->AdvanceTime(ScaledDelta);
        }

        ICharacterDataSource* Source = Data.GetDataSource();
        if (Source)
        {
            FVector CurrentPos = Source->GetPosition();
            Data.Transform = FTransform(Source->GetRotation(), CurrentPos);
            float Distance = (CurrentPos - Data.PrevPosition).Size();
            Data.MovementSpeed = (ScaledDelta > 0.f) ? (Distance / ScaledDelta) : 0.f;
            Data.PrevPosition = CurrentPos;
        }
    }
}

void UCharacterAnimManager::EvaluateAnimationParallel()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(EvaluateAnimationParallel);

    ParallelFor(IndexCache.ActiveIndices.Num(), [this](int32 i)
    {
        int32 Index = IndexCache.ActiveIndices[i];
        auto& Data = Instances[Index];
        UCharacterSkelMeshComponent* Comp = Data.GetComponent();
        if (!Comp) return;

        if (Comp->IsPlayingAnim())
        {
            Comp->EvaluateAnimation();
        }
    });
}

void UCharacterAnimManager::ApplyBoneTransforms()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ApplyBoneTransforms);

    for (int32 Index : IndexCache.ActiveIndices)
    {
        auto& Data = Instances[Index];
        UCharacterSkelMeshComponent* Comp = Data.GetComponent();
        if (!Comp) continue;

        Comp->ApplyBoneTransforms();
    }
}
