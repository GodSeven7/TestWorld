# QuestPlugin

## 一句话总结

这是一个任务系统插件，用于管理游戏任务的生命周期（创建/启动/完成/失败），通过可组合的目标类型（击杀计数/倒计时）驱动任务进度和结果判定。

## 核心功能详解

### 1. 核心机制

插件以 `UQuestManager`（WorldSubsystem）为任务管理中枢，维护活跃任务列表。每个任务由 `UQuestInstance` 表示，包含一组 `UQuestObjective` 目标。目标采用策略模式，每种目标类型定义自己的触发条件（如击杀指定类型Actor、倒计时结束）。任务每帧Tick驱动目标更新，当任一目标被触发时，其 `ResultOnTrigger` 决定任务结果（Victory/Defeat）。Manager提供快捷工厂方法（CreateBossQuest/CreateSurvivalQuest）创建常见任务模板。

### 2. 关键类与职责

| 类名 | 文件位置 | 职责 |
|------|---------|------|
| UQuestManager | Public/QuestManager.h | 任务管理器，负责任务创建/启动/停止/ ticking和Actor死亡通知分发 |
| UQuestInstance | Public/QuestInstance.h | 任务实例，持有目标列表，管理任务状态（Inactive/Active/Completed/Failed） |
| UQuestObjective | Public/QuestObjective.h | 目标基类，定义触发机制和进度查询接口 |
| UQuestObjective_ActorDeath | Public/QuestObjective_ActorDeath.h | 击杀目标，追踪指定类型Actor的死亡计数 |
| UQuestObjective_Timer | Public/QuestObjective_Timer.h | 计时目标，倒计时结束触发（可设为Victory或Defeat） |

### 3. 对外接口

**UQuestManager BlueprintCallable 方法：**
- `CreateQuest(QuestID)` — 创建任务实例
- `StartQuest(Quest)` — 启动任务
- `StopQuest(QuestID)` — 停止任务
- `NotifyActorDeath(DeadActor)` — 通知Actor死亡（分发给所有活跃任务）
- `GetQuest(QuestID)` — 查询任务实例

**UQuestManager 快捷工厂：**
- `CreateBossQuest(QuestID, TimeLimit, BossClass, PlayerClass)` — 创建Boss战任务（击杀Boss=Victory，超时=Defeat）
- `CreateSurvivalQuest(QuestID, SurvivalTime, PlayerClass)` — 创建生存任务（存活到时间=Victory，玩家死亡=Defeat）

**UQuestInstance BlueprintCallable 方法：**
- `GetState()` — 获取任务状态
- `GetQuestID()` — 获取任务ID
- `GetResult()` — 获取任务结果
- `GetObjectives()` — 获取目标列表

**UQuestObjective BlueprintCallable 方法：**
- `IsTriggered()` — 目标是否已触发
- `GetResultOnTrigger()` — 触发时的结果
- `GetProgress()` — 目标进度（0.0~1.0）

**UQuestObjective_ActorDeath 方法：**
- `GetKillCount()` / `GetTargetCount()` — 击杀数/目标数

**UQuestObjective_Timer 方法：**
- `GetRemainingTime()` / `GetTimeLimit()` — 剩余时间/时间限制

**委托事件：**
- `UQuestInstance::OnQuestFinished` — 任务结束时触发（Result + Reason）
- `UQuestManager::OnQuestCompleted` — 任务完成时全局触发（QuestID + Result + Reason）

### 4. 数据流

任务创建流程：CreateQuest → new UQuestInstance → Setup(QuestID) → 添加Objectives → StartQuest → State=Active → 加入ActiveQuests

每帧更新：TickQuests → 遍历ActiveQuests → QuestInstance.Tick → 遍历Objectives → Objective.Tick（如Timer倒计时）→ EvaluateObjectives → 检查是否有目标被触发 → 触发时设置Quest结果 → 广播OnQuestFinished/OnQuestCompleted

Actor死亡通知：NotifyActorDeath → 遍历ActiveQuests → QuestInstance.NotifyActorDeath → 遍历Objectives → Objective.NotifyActorDeath（如ActorDeath目标检查死亡Actor是否匹配TargetActorClass）→ 更新击杀计数 → 达到TargetCount时触发

### 5. 设计要点

- **可组合目标**：任务可包含多个目标，任一目标触发即决定任务结果，支持Victory和Defeat两种触发结果
- **策略模式**：`UQuestObjective` 为抽象基类，子类实现特定触发逻辑，易于扩展新目标类型
- **快捷工厂**：`CreateBossQuest` 和 `CreateSurvivalQuest` 封装常见任务模板，减少重复代码
- **安全停止**：`PendingStopQuestIDs` 延迟停止机制，避免在Tick过程中修改ActiveQuests数组
- **进度查询**：`GetProgress()` 返回0.0~1.0的标准化进度值，Timer目标返回剩余时间比例，ActorDeath目标返回击杀比例

### 6. 不做什么

- 不负责任务的UI显示和进度展示
- 不提供任务链/分支任务等复杂任务结构
- 不负责任务数据的持久化存储
- 不提供网络同步的任务状态管理
- 不支持动态添加/移除目标（任务启动后目标固定）
