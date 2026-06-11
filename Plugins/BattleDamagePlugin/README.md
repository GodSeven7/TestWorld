# BattleDamagePlugin

## 一句话总结

这是一个战斗伤害计算插件，用于提供伤害计算、治疗、死亡判定和伤害数字显示的完整战斗数值体系。

## 核心功能详解

### 1. 核心机制

插件通过 `UBattleDamageManager`（WorldSubsystem）作为中央调度器，接收伤害/治疗请求，经过护甲减免计算和暴击判定后修改目标属性，并通过延迟通知机制在游戏线程上安全地触发事件回调和伤害数字显示。伤害处理支持即时模式（`ApplyDamage`）和队列模式（`QueueDamage`），队列模式允许从任意线程提交伤害请求，随后在游戏线程统一处理。

### 2. 关键类与职责

| 类名 | 文件位置 | 职责 |
|------|---------|------|
| UBattleDamageManager | Public/BattleDamageManager.h | 伤害系统中央管理器，负责伤害计算、护甲减免、暴击判定、延迟通知和击杀处理 |
| UBattleDamageComponent | Public/BattleDamageComponent.h | Actor组件，持有战斗属性数据，实现IBattleDamageInterface，自动注册到Manager |
| IBattleDamageInterface | Public/BattleDamageInterface.h | 战斗对象接口，定义属性获取、伤害应用、死亡/治疗回调等契约 |
| UDamageNumberManager | Public/DamageNumberManager.h | 伤害数字显示管理器，通过Niagara粒子系统批量渲染伤害数字 |
| FBattleAttributeData | Public/BattleDamageTypes.h | 战斗属性数据结构，包含生命值、攻击力、防御、魔抗、暴击等 |

### 3. 对外接口

**BlueprintCallable 方法：**
- `UBattleDamageManager::QueueDamage(Target, Info)` — 队列化伤害请求
- `UBattleDamageManager::QueueHeal(Target, Amount, Healer)` — 队列化治疗请求
- `UBattleDamageManager::QueueKill(Target, ...)` — 队列化击杀请求
- `UBattleDamageManager::ApplyDamage(Target, Info)` — 即时伤害计算
- `UBattleDamageManager::ApplyHeal(Target, Amount, Healer)` — 即时治疗
- `UBattleDamageManager::ProcessCombatHit(Target, RawDamage, DamageType, Instigator, ImpactLocation, bCanCritical)` — 完整战斗命中处理（线程安全读取属性）
- `UBattleDamageManager::ProcessPendingDamages()` — 处理待处理伤害队列
- `UBattleDamageManager::ProcessPendingKillsOnGameThread()` — 处理待处理击杀
- `UBattleDamageManager::ProcessPendingDamageNotifications()` — 处理待处理伤害通知
- `UBattleDamageManager::SetDamageNumberEnabled(bEnabled)` — 开关伤害数字显示
- `UBattleDamageComponent::SetHealth(NewHealth)` — 设置当前生命值
- `UBattleDamageComponent::SetMaxHealth(NewMaxHealth)` — 设置最大生命值
- `UBattleDamageComponent::Revive(HealthPercent)` — 复活

**委托事件：**
- `UBattleDamageComponent::OnDamageReceivedDelegate` — 受到伤害时触发
- `UBattleDamageComponent::OnDeathDelegate` — 死亡时触发
- `UBattleDamageComponent::OnHealReceivedDelegate` — 受到治疗时触发
- `UBattleDamageComponent::OnRevivedDelegate` — 复活时触发
- `UBattleDamageManager::OnCombatantRegistered` — 战斗者注册时触发
- `UBattleDamageManager::OnCombatantUnregistered` — 战斗者注销时触发
- `UBattleDamageManager::OnCombatantDeath` — 战斗者死亡时触发
- `UBattleDamageManager::OnCombatHitProcessed` — 战斗命中处理完成时触发

### 4. 数据流

外部调用 → QueueDamage/ProcessCombatHit → Manager加锁入队 → ProcessPendingDamages取出并调用CalculateAndApplyDamage → 护甲减免计算 + 暴击判定 → 修改目标Attributes.CurrentHealth → ApplyDamageResult → 生成PendingDamageNotification → ProcessPendingDamageNotifications在GameThread触发OnDamageReceived回调 + EnqueueDamageNumber → FlushToNiagara渲染伤害数字

击杀流程：伤害致死 → QueueKill入队 → ProcessPendingKillsOnGameThread → 触发OnDeath + OnCombatantDeath

### 5. 设计要点

- **线程安全**：属性读写使用 `FCriticalSection` 保护（`AttributeLock`、`CombatantsLock`、`PendingDamagesLock`、`PendingKillsLock`、`PendingDamageNotificationsLock`）；`ProcessCombatHit` 使用 `GetAttributeDataThreadSafe()` 读取属性，适合从非GameThread调用
- **延迟通知模式**：伤害计算可以立即执行，但事件回调和伤害数字显示通过 PendingNotification 延迟到 GameThread 处理，避免跨线程委托广播
- **伤害减免公式**：物理伤害 = max(0, BaseDamage - Defense)；魔法伤害 = max(0, BaseDamage - MagicResistance)；真实伤害 = BaseDamage；治疗不走伤害减免，由ApplyHeal独立处理
- **暴击机制**：通过 `RollCritical(CriticalChance)` 随机判定，暴击伤害 = FinalDamage × CriticalMultiplier
- **伤害数字**：基于 Niagara 系统批量渲染，通过 `UDamageNumberManager` 管理生命周期，支持普通/暴击两种类型
- **自动注册**：`UBattleDamageComponent` 的 `bAutoRegister` 默认为 true，在 BeginPlay 时自动注册到 Manager

### 6. 不做什么

- 不负责伤害来源的判定逻辑（如攻击范围检测、碰撞检测），由 CombatSystemPlugin 负责
- 不负责伤害数字的UI布局和样式，仅通过 Niagara 粒子系统渲染
- 不负责角色死亡后的行为逻辑（如播放死亡动画、掉落物品），仅触发事件
- 不提供网络同步机制，伤害计算在本地执行
