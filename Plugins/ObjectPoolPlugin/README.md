# ObjectPoolPlugin

## 一句话总结

这是一个对象池插件，用于通过池化复用UObject和AActor实例来避免频繁的创建/销毁开销，提供泛型对象池和Actor池的统一管理能力。

## 核心功能详解

### 1. 核心机制

插件以 `UObjectPoolManager`（GameInstanceSubsystem）为统一入口，按类型（UClass）管理多个对象池实例。`TObjectPool<T>` 为UObject提供池化能力，`TActorPool<T>` 为AActor提供池化能力。Actor池化采用"隐藏+禁用Tick+移至隐藏位置"的方式模拟回收，而非Destroy；重新取出时恢复可见性和Tick。两个池均支持预预热（Prewarm）、自动扩容和容量上限。池化对象通过 `IPoolableObject`/`IPoolableActor` 接口接收生命周期回调。

### 2. 关键类与职责

| 类名 | 文件位置 | 职责 |
|------|---------|------|
| UObjectPoolManager | Public/ObjectPoolManager.h | 对象池管理器，按UClass管理TObjectPool和TActorPool实例，提供模板化的Spawn/Return接口 |
| TObjectPool<T> | Public/ObjectPool.h | UObject泛型对象池，管理FreeList和AllObjects，支持线程安全操作 |
| TActorPool<T> | Public/ActorPool.h | AActor泛型对象池，通过隐藏/禁用/移位模拟回收，支持自定义Spawn/Return委托 |
| IPoolableObject | Public/ObjectPool.h | UObject池化接口，定义OnSpawnFromPool/OnReturnToPool/ResetPoolState/CanReturnToPool |
| IPoolableActor | Public/ActorPool.h | Actor池化接口，定义OnActorPoolSpawn/OnActorPoolReturn/ResetActorPoolState/RecycleSelf |

### 3. 对外接口

**UObjectPoolManager 模板方法：**
- `SpawnActor<T>(World, Location, Rotation)` — 从池中取出Actor
- `SpawnActor<T>(World, Transform)` — 从池中取出Actor（带Transform）
- `ReturnActor<T>(Actor)` / `ReturnActor(Actor)` — 归还Actor到池
- `SpawnObject<T>()` — 从池中取出UObject
- `ReturnObject<T>(Object)` — 归还UObject到池
- `PrewarmActorPool<T>(Count)` — 预热Actor池
- `PrewarmObjectPool<T>(Count)` — 预热UObject池
- `GetOrCreateActorPool<T>(Config)` — 获取或创建Actor池
- `GetOrCreateObjectPool<T>(Config)` — 获取或创建UObject池

**UObjectPoolManager BlueprintCallable 方法：**
- `ClearAllPools()` — 清空所有池
- `GetAllPoolStats(OutActorStats, OutObjectStats)` — 获取所有池统计信息
- `DumpPoolStats()` — 输出池统计到日志

**IPoolableActor 方法：**
- `RecycleSelf()` — 自行归还到池

**委托事件：**
- `UObjectPoolManager::OnActorReturnedToPool` — Actor归还到池时全局通知
- `TActorPool::OnSpawnDelegate` / `OnReturnDelegate` — 单池级别的自定义回调

### 4. 数据流

Actor取出流程：SpawnActor<T> → GetOrCreateActorPool → Pool.Spawn(Transform) → FreeList有可用对象 → ActivateActor（SetTransform/SetHiddenInGame(false)/SetTickEnabled(true)/OnActorPoolSpawn）→ 返回Actor指针

Actor归还流程：ReturnActor → Pool.Return → DeactivateActor（OnActorPoolReturn/ResetActorPoolState/SetHiddenInGame(true)/SetTickEnabled(false)/SetActorLocation(PoolHiddenLocation)）→ 加入FreeList

UObject取出/归还流程类似，但无Transform/Hidden/Tick操作。

### 5. 设计要点

- **SpawnActorDeferred模式**：Actor首次创建使用 `SpawnActorDeferred`，取出时才调用 `FinishSpawning`，确保BeginPlay在正确位置触发
- **隐藏位置策略**：池中Actor被移动到 `PoolHiddenLocation`（默认Z=-100000），而非Destroy
- **线程安全**：`TObjectPool` 和 `TActorPool` 均通过 `FCriticalSection`（PoolLock）保护，`UObjectPoolManager` 通过 `PoolsLock` 保护池注册表
- **自动扩容**：当FreeList为空且 `bAutoExpand=true` 时，按 `ExpandStep` 数量自动创建新对象
- **容量限制**：`MaxCapacity=0` 表示无限制，否则限制总创建数量
- **统计信息**：`FObjectPoolStats`/`FActorPoolStats` 跟踪 TotalCreated/TotalSpawned/TotalReturned/CurrentActive/PeakUsage
- **IPoolableActor.PendingReturn**：`SetActorPoolPendingReturn` 标记防止重复归还
- **GameInstance级别**：`UObjectPoolManager` 为GameInstanceSubsystem，池跨关卡持久化

### 6. 不做什么

- 不负责池中Actor的碰撞处理，回收后碰撞体仍存在（需在OnActorPoolReturn中手动禁用）
- 不提供基于使用频率的自动缩容机制
- 不负责网络同步的池化对象状态管理
- 不提供对象池的序列化和持久化
