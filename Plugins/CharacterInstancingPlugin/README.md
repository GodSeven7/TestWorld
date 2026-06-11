# CharacterInstancingPlugin

## 一句话总结

这是一个角色动画管理插件，以 WorldSubsystem 为核心调度器，对大量角色的骨骼动画进行集中式状态解析、时间推进和并行求值，实现大规模角色的高性能动画驱动。

## 核心功能详解

### 1. 核心机制

插件以 `UCharacterAnimManager`（WorldSubsystem）为核心调度器，管理所有角色实例的注册、动画状态更新和骨骼变换应用。每个角色通过 `UCharacterSkelMeshComponent` 注册到 Manager，Manager 维护一个 `FCharacterInstanceData` 池，按帧执行动画状态解析、时间推进、并行骨骼动画求值和骨骼变换应用。所有角色使用独立骨骼网格体渲染，通过 `FCharacterAnimEvaluator` 进行CPU端动画采样。

### 2. 关键类与职责

| 类名 | 文件位置 | 职责 |
|------|---------|------|
| UCharacterAnimManager | Public/CharacterAnimManager.h | 核心管理器，负责实例注册/释放、动画Tick调度、并行动画求值 |
| UCharacterSkelMeshComponent | Public/CharacterSkelMeshComponent.h | 角色骨骼网格体组件，实现ICharacterDataSource，持有动画配置和状态，支持独立动画播放 |
| UCharacterAnimData | Public/CharacterAnimData.h | 动画配置DataAsset，定义骨骼网格体、动画状态列表、速度阈值等 |
| FCharacterAnimEvaluator | Public/CharacterAnimEvaluator.h | 动画求值器，提供CPU端动画采样和骨骼空间转换 |

### 3. 对外接口

**BlueprintCallable 方法：**
- `UCharacterSkelMeshComponent::InitFromAnimData(AnimData)` — 从DataAsset初始化动画配置
- `UCharacterSkelMeshComponent::SetAnimState(StateName)` — 切换动画状态
- `UCharacterSkelMeshComponent::SetPlayAnimRate(PlayRate)` — 设置播放速率
- `UCharacterSkelMeshComponent::SetLooping(bLooping)` — 设置循环播放
- `UCharacterSkelMeshComponent::PlayAnim()` / `PauseAnim()` / `StopAnim()` — 播放控制

**ICharacterDataSource 接口方法：**
- `GetPosition()` / `GetRotation()` — 获取位置/旋转
- `IsDead()` / `IsAttacking()` — 获取状态标记
- `GetMeshType()` — 获取网格体类型
- `SetMeshVisibility(bVisible)` — 控制可见性

### 4. 数据流

注册流程：UCharacterSkelMeshComponent.BeginPlay → RegisterComponent → Manager.AllocateSlot → 分配FCharacterInstanceData

每帧Tick：Manager.Tick → CollectActiveIndices → ResolveAnimStates（根据速度/死亡/攻击状态解析动画）→ AdvanceTimeAndSyncTransforms → EvaluateAnimationParallel（并行骨骼动画求值）→ ApplyBoneTransforms

### 5. 设计要点

- **集中式动画调度**：Manager 统一管理所有角色的动画状态解析和时间推进，避免各组件独立Tick带来的调度开销
- **并行动画求值**：`EvaluateAnimationParallel` 使用 ParallelFor 并行计算骨骼变换，利用 `FCharacterAnimEvaluator` 的 `SampleAnimation` 和 `LocalToComponentSpace` 进行CPU端动画采样
- **位图状态管理**：`FCharacterStateBits` 使用 TBitArray 高效跟踪 IsActive 状态
- **Slot池化**：预分配 INITIAL_CAPACITY(200) 个实例槽位，通过 FreeIndices 管理槽位回收，避免频繁内存分配
- **时间膨胀支持**：`GlobalTimeDilation` 和 `bGameplaySuspended` 控制动画推进
- **线程安全**：`SlotLock` 保护实例分配/释放操作

### 6. 不做什么

- 不负责角色的移动逻辑和AI行为，仅处理动画
- 不负责动画蓝图（AnimBP）的执行，使用原生UAnimSequence采样
- 不负责角色的碰撞和物理模拟
- 不提供网络同步
