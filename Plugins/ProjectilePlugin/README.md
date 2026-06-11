# ProjectilePlugin

## 一句话总结

这是一个投射物插件，用于管理大量投射物的抛物线弹道计算、并行运动更新、碰撞检测和伤害施加，通过HISM实例化渲染实现高性能投射物可视化。

## 核心功能详解

### 1. 核心机制

插件以 `UProjectileManager`（WorldSubsystem）为核心，管理投射物的完整生命周期。投射物发射时通过 `FProjectileTrajectoryUtils` 计算抛物线轨迹点序列，存入 `FProjectileData`。每帧Tick按阶段处理：Flying阶段沿轨迹点插值移动，当高度低于碰撞阈值时转入Collision阶段；Collision阶段通过 ISpatialQueryService 查询范围内目标并施加伤害（通过 IDamageService），然后进入Destroyed阶段等待延迟清理。运动计算和碰撞检测均支持并行处理，渲染通过 HISM 批量更新实例变换。

### 2. 关键类与职责

| 类名 | 文件位置 | 职责 |
|------|---------|------|
| UProjectileManager | Public/ProjectileManager.h | 投射物管理器，负责发射/运动/碰撞/伤害/销毁的完整生命周期管理 |
| FProjectileData | Public/ProjectileData.h | 投射物运行时数据，包含轨迹点、位置、速度、阶段状态和碰撞信息 |
| FProjectileTrajectoryUtils | Public/ProjectileData.h | 抛物线轨迹计算工具，生成轨迹点序列和时间序列 |
| FProjectileStateBits | Public/ProjectileData.h | 位图状态管理，跟踪IsActive/IsFlying/IsInCollisionPhase/NeedsTransformUpdate等 |
| FProjectileDebug | Public/ProjectileDebug.h | 调试工具，提供控制台命令发射测试投射物 |

### 3. 对外接口

**UProjectileManager 方法：**
- `SpawnProjectile(Params)` — 发射投射物（FProjectileSpawnParams版本）
- `SpawnProjectile(Params)` — 发射投射物（FProjectileLaunchParams版本，IProjectileService接口实现）
- `DestroyProjectile(ProjectileID)` — 销毁指定投射物
- `ClearAllProjectiles()` — 清除所有投射物
- `SetGroundHeight(Height)` — 设置地面高度
- `SetDebugEnabled(bEnabled)` — 开关调试模式
- `SetDrawTrajectoryDebug(bEnabled)` — 开关轨迹调试绘制

**UProjectileManager BlueprintCallable 方法：**
- `GetActiveProjectileCount()` — 获取活跃投射物数量
- `GetTotalProjectileCount()` — 获取总投射物数量

**IProjectileService 接口实现：**
- `SpawnProjectile(FProjectileLaunchParams)` — IProjectileService接口

**委托事件：**
- `OnProjectileHit` — 投射物命中时触发（FProjectileHitResult）
- `OnProjectileSpawned` — 投射物生成时触发（ProjectileID）
- `OnProjectileDestroyed` — 投射物销毁时触发（ProjectileID）

### 4. 数据流

发射流程：SpawnProjectile → AllocateProjectileSlot → FProjectileTrajectoryUtils.CalculateParabolicTrajectory计算轨迹 → FProjectileData.InitializeWithTrajectory → Phase=Flying → 添加HISM实例

每帧Tick：CollectActiveIndices → CollectPhaseIndices → ParallelComputeMovement（Flying阶段沿轨迹插值移动）→ ParallelSweepCollisionDetection（Flying阶段扫掠检测）→ ParallelCollisionDetection（Collision阶段范围查询检测）→ ProcessHitResults（命中处理+伤害施加）→ FlushTransformUpdates（批量更新HISM变换）→ ProcessPendingDeactivations（清理已销毁投射物）

伤害流程：碰撞检测命中 → ApplyDamageToTarget → IDamageService.ApplyDamageHit → 触发OnProjectileHit

### 5. 设计要点

- **抛物线弹道**：`FProjectileTrajectoryUtils::CalculateParabolicTrajectory` 根据起点/终点/高度/速度预计算完整轨迹点序列，运行时沿轨迹插值移动
- **三阶段生命周期**：Flying（沿轨迹飞行）→ Collision（接近地面时进入碰撞检测）→ Destroyed（延迟清理）
- **并行计算**：`ParallelComputeMovement` 和 `ParallelSweepCollisionDetection` 使用并行任务处理大量投射物
- **HISM渲染**：通过 `UHierarchicalInstancedStaticMeshComponent` 批量渲染投射物，无效实例移至Z=-100000
- **Slot池化**：预分配 INITIAL_CAPACITY(500) 个投射物槽位，通过 FreeIndices 管理回收
- **位图状态**：`FProjectileStateBits` 使用 TBitArray 高效跟踪状态
- **碰撞高度阈值**：`CollisionHeightThreshold` 控制投射物何时从Flying转入Collision阶段
- **扫掠子步**：`MAX_SWEEP_SUBSTEPS=8` 防止高速投射物穿透目标
- **线程安全**：`SlotLock` 保护槽位分配/释放
- **调试支持**：`FProjectileDebug` 提供控制台命令发射测试投射物和轨迹可视化

### 6. 不做什么

- 不负责投射物的视觉特效（如拖尾、爆炸），仅提供HISM网格渲染
- 不负责投射物的音效
- 不提供投射物的网络同步
- 不支持非抛物线弹道（如追踪弹、直线弹道）[待确认]
