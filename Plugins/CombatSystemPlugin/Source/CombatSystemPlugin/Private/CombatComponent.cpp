#include "CombatComponent.h"
#include "CombatSkill.h"
#include "CombatWeapon.h"
#include "CombatManager.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"


#if WITH_EDITOR
TArray<FString> UCombatComponent::GetCharacterIDOptions() const
{
    TArray<FString> Options;
    if (CharacterTableRef)
    {
        for (const FName& RowName : CharacterTableRef->GetRowNames())
        {
            Options.Add(RowName.ToString());
        }
    }
    return Options;
}

TArray<FString> UCombatComponent::GetWeaponIDOptionsFromComp() const
{
    TArray<FString> Options;
    if (WeaponTableRef)
    {
        for (const FName& RowName : WeaponTableRef->GetRowNames())
        {
            Options.Add(RowName.ToString());
        }
    }
    return Options;
}
#endif

UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    LoadSkillsFromTables();

    if (bAutoRegister)
    {
        if (UCombatManager* Manager = GetManager())
        {
            Manager->RegisterCombatComponent(this);
        }
    }
}

void UCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bAutoRegister)
    {
        if (UCombatManager* Manager = GetManager())
        {
            Manager->UnregisterCombatComponent(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void UCombatComponent::LoadSkillsFromTables()
{
    UCombatManager* Manager = GetManager();
    if (!Manager)
        return;

    Skills.Empty();
    Weapon = nullptr;

    // 方案B：从 CharacterTable 加载角色技能列表
    const FCharacterCombatConfig* CharConfig = Manager->FindCharacterConfig(CharacterID);
    if (CharConfig)
    {
        // 加载角色默认武器
        FName CharWeaponID = CharConfig->DefaultWeaponHandle.RowName;
        if (!CharWeaponID.IsNone() && !Weapon)
        {
            const FWeaponStats* WeaponStats = Manager->FindWeaponStats(CharWeaponID);
            if (WeaponStats)
            {
                Weapon = NewObject<UCombatWeapon>(this);
                Weapon->WeaponID = CharWeaponID;
                // 从 WeaponStats 的 DefaultSkillHandle 获取技能ID
                Weapon->SkillID = WeaponStats->DefaultSkillHandle.RowName;
                Weapon->InitWeapon(Manager);

                if (Weapon->GetSkillConfig())
                {
                    Skills.Add(Weapon);
                }
            }
        }

        // 加载角色技能列表
        for (const FDataTableRowHandle& SkillHandle : CharConfig->SkillHandles)
        {
            FName SkillIDFromHandle = SkillHandle.RowName;
            if (SkillIDFromHandle.IsNone())
                continue;

            const FCombatSkillConfig* Config = Manager->FindSkillConfig(SkillIDFromHandle);
            if (!Config)
                continue;

            // 如果是武器技能且还没有武器，创建武器
            if (Config->SkillType == ESkillType::Attack && !Weapon)
            {
                Weapon = NewObject<UCombatWeapon>(this);
                Weapon->WeaponID = WeaponID;
                Weapon->SkillID = SkillIDFromHandle;
                Weapon->InitWeapon(Manager);
                Skills.Add(Weapon);
                continue;
            }

            // 普通技能
            UCombatSkill* Skill = NewObject<UCombatSkill>(this);
            Skill->SkillID = SkillIDFromHandle;
            Skill->InitSkill(Manager);
            Skills.Add(Skill);
        }
    }

    // 方案A：从 WeaponTable 加载武器和默认技能
    if (!WeaponID.IsNone() && !Weapon)
    {
        const FWeaponStats* WeaponStats = Manager->FindWeaponStats(WeaponID);
        if (WeaponStats)
        {
            Weapon = NewObject<UCombatWeapon>(this);
            Weapon->WeaponID = WeaponID;
            // 从 WeaponStats 的 DefaultSkillHandle 获取技能ID
            Weapon->SkillID = WeaponStats->DefaultSkillHandle.RowName;
            Weapon->InitWeapon(Manager);

            if (Weapon->GetSkillConfig() && !Skills.Contains(Weapon))
            {
                Skills.Add(Weapon);
            }
        }
    }

    InitAllSkills(Manager);
    InitSkillQueue();
}

void UCombatComponent::InitAllSkills(UCombatManager* Manager)
{
    for (UCombatSkill* Skill : Skills)
    {
        if (Skill)
        {
            if (UCombatWeapon* WeaponSkill = Cast<UCombatWeapon>(Skill))
            {
                WeaponSkill->InitWeapon(Manager);
            }
            else
            {
                Skill->InitSkill(Manager);
            }
        }
    }
}

float UCombatComponent::GetWeaponRange() const
{
    if (Weapon)
    {
        if (const FCombatSkillConfig* Config = Weapon->GetSkillConfig())
        {
            return Config->Range;
        }
    }
    return 0.0f;
}

bool UCombatComponent::IsCombatActive() const
{
    return GetOwner() != nullptr && Weapon != nullptr;
}

void UCombatComponent::AddPendingEvent(const FCombatEventData& Event)
{
    FScopeLock Lock(&EventLock);
    PendingEvents.Add(Event);
}

void UCombatComponent::FlushPendingEvents()
{
    TArray<FCombatEventData> EventsToDispatch;
    
    {
        FScopeLock Lock(&EventLock);
        EventsToDispatch = MoveTemp(PendingEvents);
    }

    for (const FCombatEventData& Event : EventsToDispatch)
    {
        OnAttack.Broadcast(Event.Attacker, Event);
        if (Event.Target)
        {
            OnHit.Broadcast(Event.Target, Event);
        }
    }
}

UCombatManager* UCombatComponent::GetManager() const
{
    if (UWorld* World = GetWorld())
    {
        return World->GetSubsystem<UCombatManager>();
    }
    return nullptr;
}

bool UCombatComponent::IsInSwing(AActor* Target) const
{
    if (!Target)
        return false;

    FScopeLock Lock(&SwingLock);
    for (const FActiveSwing& Swing : ActiveSwings)
    {
        if (Swing.HitActors.Contains(Target))
        {
            return true;
        }
    }
    return false;
}

bool UCombatComponent::HasActiveSwing() const
{
    FScopeLock Lock(&SwingLock);
    return !ActiveSwings.IsEmpty();
}

void UCombatComponent::AddSwing(double EndTime)
{
    FScopeLock Lock(&SwingLock);
    FActiveSwing NewSwing;
    NewSwing.EndTime = EndTime;
    ActiveSwings.Add(NewSwing);
}

void UCombatComponent::AddSwingHit(AActor* Target)
{
    if (!Target)
        return;

    FScopeLock Lock(&SwingLock);
    if (ActiveSwings.IsEmpty())
        return;

    FActiveSwing& CurrentSwing = ActiveSwings.Last();
    CurrentSwing.HitActors.Add(Target);
}

void UCombatComponent::CleanupExpiredSwings(double CurrentTime)
{
    FScopeLock Lock(&SwingLock);
    ActiveSwings.RemoveAll([CurrentTime](const FActiveSwing& Swing)
    {
        return Swing.EndTime <= CurrentTime;
    });
}

void UCombatComponent::SetAttackPhase(EAttackPhase Phase)
{
    CurrentPhase = Phase;
}

void UCombatComponent::OnCombatAttackExecuted(float Damage, bool bIsCritical)
{
}

void UCombatComponent::OnCombatHitReceived(float Damage)
{
}

FVector UCombatComponent::GetCombatWorldPosition() const
{
    if (AActor* Owner = GetOwner())
        return Owner->GetActorLocation();
    return FVector::ZeroVector;
}

FVector UCombatComponent::GetCombatForwardVector() const
{
    if (AActor* Owner = GetOwner())
        return Owner->GetActorForwardVector();
    return FVector::ForwardVector;
}

int32 UCombatComponent::GetCombatFaction() const
{
    return Faction;
}

UObject* UCombatComponent::GetCombatParent() const
{
    return GetOwner();
}

void UCombatComponent::RequestAttack(AActor* Target)
{
    if (!HasWeapon())
    {
        return;
    }

    int32 WeaponSkillIndex = Skills.IndexOfByKey(Weapon);
    if (WeaponSkillIndex != INDEX_NONE)
    {
        RequestSkill(WeaponSkillIndex);
    }
}

bool UCombatComponent::CanAttack() const
{
    return HasWeapon() && CurrentPhase == EAttackPhase::Idle;
}

bool UCombatComponent::IsAttacking() const
{
    return CurrentPhase != EAttackPhase::Idle;
}

// ── 技能系统实现 ──

int32 UCombatComponent::AddSkill(UCombatSkill* Skill)
{
    if (!Skill)
        return INDEX_NONE;

    Skills.Add(Skill);
    return Skills.Num() - 1;
}

void UCombatComponent::RemoveSkill(int32 SkillIndex)
{
    if (!Skills.IsValidIndex(SkillIndex))
        return;

    if (ActiveSkillIndex == SkillIndex)
    {
        AbortActiveSkill();
    }

    Skills.RemoveAt(SkillIndex);

    SkillQueue.RemoveAll([SkillIndex](const FScheduledSkill& Entry)
    {
        return Entry.SkillIndex == SkillIndex;
    });

    for (FScheduledSkill& Entry : SkillQueue)
    {
        if (Entry.SkillIndex > SkillIndex)
        {
            Entry.SkillIndex--;
        }
    }
}

bool UCombatComponent::RequestSkill(int32 SkillIndex)
{
    if (!CanCastSkill(SkillIndex))
        return false;

    StartSkill(SkillIndex);
    return true;
}

bool UCombatComponent::CanCastSkill(int32 SkillIndex) const
{
    if (!Skills.IsValidIndex(SkillIndex))
        return false;

    if (ActiveSkillIndex != INDEX_NONE)
        return false;

    double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    for (const FScheduledSkill& Entry : SkillQueue)
    {
        if (Entry.SkillIndex == SkillIndex)
        {
            if (Entry.AvailableTime > CurrentTime)
                return false;
            break;
        }
    }

    UCombatSkill* Skill = Skills[SkillIndex];

    return Skill->CanExecute(GetOwner()) &&
           Skill->CheckPreCondition(GetOwner());
}

bool UCombatComponent::IsSkillOnCooldown(int32 SkillIndex) const
{
    double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    for (const FScheduledSkill& Entry : SkillQueue)
    {
        if (Entry.SkillIndex == SkillIndex)
        {
            return Entry.AvailableTime > CurrentTime;
        }
    }
    return false;
}

float UCombatComponent::GetSkillCooldownRemaining(int32 SkillIndex) const
{
    double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    for (const FScheduledSkill& Entry : SkillQueue)
    {
        if (Entry.SkillIndex == SkillIndex)
        {
            return FMath::Max(0.0f, static_cast<float>(Entry.AvailableTime - CurrentTime));
        }
    }
    return 0.0f;
}

UCombatSkill* UCombatComponent::GetActiveSkill() const
{
    if (Skills.IsValidIndex(ActiveSkillIndex))
    {
        return Skills[ActiveSkillIndex];
    }
    return nullptr;
}

int32 UCombatComponent::FindSkillIndex(FName InSkillID) const
{
    for (int32 i = 0; i < Skills.Num(); i++)
    {
        if (Skills[i] && Skills[i]->GetSkillID() == InSkillID)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

void UCombatComponent::InitSkillQueue()
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    double CurrentTime = World->GetTimeSeconds();
    SkillQueue.Empty();

    for (int32 i = 0; i < Skills.Num(); i++)
    {
        UCombatSkill* Skill = Skills[i];
        if (!Skill)
            continue;

        const FCombatSkillConfig* Config = Skill->GetSkillConfig();
        if (!Config || !Config->bAutoCast)
            continue;

        if (!Skill->CanExecute(GetOwner()))
            continue;

        FScheduledSkill Entry;
        Entry.AvailableTime = CurrentTime;
        Entry.SkillIndex = i;
        Entry.Priority = Config->Priority;
        SkillQueue.Add(Entry);
    }

    SkillQueue.Sort([](const FScheduledSkill& A, const FScheduledSkill& B)
    {
        return A < B;
    });

    bSkillQueueDirty = false;
}

void UCombatComponent::ProcessSkillQueue(float DeltaTime)
{
    if (bSkillQueueDirty)
    {
        SkillQueue.Sort([](const FScheduledSkill& A, const FScheduledSkill& B)
        {
            return A < B;
        });
        bSkillQueueDirty = false;
    }

    if (ActiveSkillIndex != INDEX_NONE)
    {
        if (!Skills.IsValidIndex(ActiveSkillIndex))
        {
            ActiveSkillIndex = INDEX_NONE;
            CurrentSkillPhase = ESkillPhase::Idle;
            SetAttackPhase(EAttackPhase::Idle);
            SkillPhaseTimer = 0.0f;
            return;
        }

        UCombatSkill* Skill = Skills[ActiveSkillIndex];

        SkillPhaseTimer += DeltaTime;

        switch (CurrentSkillPhase)
        {
        case ESkillPhase::Windup:
            if (SkillPhaseTimer >= Skill->GetWindupTime())
            {
                CurrentSkillPhase = ESkillPhase::Active;
                SetAttackPhase(EAttackPhase::Windup);
                SkillPhaseTimer = 0.0f;
                Skill->OnSkillStarted(GetOwner());
            }
            break;

        case ESkillPhase::Active:
            Skill->ExecuteInternal(GetOwner(), DeltaTime);
            if (SkillPhaseTimer >= Skill->GetDuration())
            {
                CurrentSkillPhase = ESkillPhase::FollowThrough;
                SetAttackPhase(EAttackPhase::Active);
                CleanupExpiredSwings(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);
                SkillPhaseTimer = 0.0f;
            }
            break;

        case ESkillPhase::FollowThrough:
            if (SkillPhaseTimer >= Skill->GetFollowThroughTime())
            {
                Skill->OnSkillCompleted(GetOwner());

                double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
                UpdateSkillAvailableTime(ActiveSkillIndex, CurrentTime + Skill->GetCooldown());

                ActiveSkillIndex = INDEX_NONE;
                CurrentSkillPhase = ESkillPhase::Idle;
                SetAttackPhase(EAttackPhase::Idle);
                SkillPhaseTimer = 0.0f;
            }
            break;

        default:
            break;
        }

        return;
    }

    if (SkillQueue.Num() > 0)
    {
        UWorld* World = GetWorld();
        if (World)
        {
            double CurrentTime = World->GetTimeSeconds();
            for (const FScheduledSkill& Entry : SkillQueue)
            {
                if (Entry.AvailableTime > CurrentTime)
                    break;

                if (!Skills.IsValidIndex(Entry.SkillIndex))
                    continue;

                UCombatSkill* Skill = Skills[Entry.SkillIndex];
                if (!Skill || !Skill->CanExecute(GetOwner()))
                    continue;

                if (!Skill->CheckPreCondition(GetOwner()))
                    continue;

                StartSkill(Entry.SkillIndex);
                break;
            }
        }
    }
}

void UCombatComponent::StartSkill(int32 SkillIndex)
{
    if (!Skills.IsValidIndex(SkillIndex))
        return;

    UCombatSkill* Skill = Skills[SkillIndex];

    ActiveSkillIndex = SkillIndex;
    CurrentSkillPhase = ESkillPhase::Windup;
    SetAttackPhase(EAttackPhase::Windup);
    SkillPhaseTimer = 0.0f;

    if (Skill->GetWindupTime() <= 0.0f)
    {
        CurrentSkillPhase = ESkillPhase::Active;
        SetAttackPhase(EAttackPhase::Active);
        SkillPhaseTimer = 0.0f;
        Skill->OnSkillStarted(GetOwner());
    }
}

void UCombatComponent::AbortActiveSkill()
{
    if (ActiveSkillIndex == INDEX_NONE)
        return;

    if (Skills.IsValidIndex(ActiveSkillIndex))
    {
        Skills[ActiveSkillIndex]->OnSkillCompleted(GetOwner());
    }

    ActiveSkillIndex = INDEX_NONE;
    CurrentSkillPhase = ESkillPhase::Idle;
    SetAttackPhase(EAttackPhase::Idle);
    SkillPhaseTimer = 0.0f;
}

void UCombatComponent::SetSkillCooldown(int32 SkillIndex, float Cooldown)
{
    if (!Skills.IsValidIndex(SkillIndex))
        return;

    double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    double NewAvailableTime = (Cooldown > 0.0f) ? (CurrentTime + Cooldown) : CurrentTime;
    UpdateSkillAvailableTime(SkillIndex, NewAvailableTime);
}

void UCombatComponent::UpdateSkillAvailableTime(int32 InSkillIndex, double NewAvailableTime)
{
    for (FScheduledSkill& Entry : SkillQueue)
    {
        if (Entry.SkillIndex == InSkillIndex)
        {
            Entry.AvailableTime = NewAvailableTime;
            bSkillQueueDirty = true;
            return;
        }
    }
}
