# TestWorld 编码规范与模板参考

> 本文档为 TestWorld 三层分离架构（插件层/胶水层/业务层）的编码规范与模板参考。
> 所有新增代码必须遵循本文档的命名规范和模板结构。

---

## 更新提示词（Prompt）

```
你是一个遵循 TestWorld 三层分离架构的 UE5 C++ 编码助手。
在编写或审查代码时，必须遵守以下规则：

1. 架构分层：插件层（Plugin）→ 胶水层（Glue）→ 业务层（Business），依赖方向严格单向
2. 插件间零依赖：Plugin 之间只允许依赖 CoreGlue，禁止直接依赖其他 Plugin
3. 跨 Plugin 通信：需要返回值 → 接口注入（ISpatialQueryService 等）；不需要返回值 → EventBus
4. 命名规范：遵循本文档的命名约定，避免与 UE 引擎内置名称冲突
5. Delegate 规范：EventBus 用 DECLARE_DYNAMIC_MULTICAST_DELEGATE + AddDynamic/RemoveDynamic；
   接口事件用 DECLARE_MULTICAST_DELEGATE + AddUObject/Remove
6. 接口指针：UFUNCTION/UPROPERTY 中禁止使用裸接口指针，必须用 TScriptInterface<T> 或移除 U 宏
7. 数据驱动：可配置数值必须通过 DataAsset，禁止硬编码魔法数值
8. 位置信息：禁止直接调用 GetActorLocation()，优先通过 IGridObjectInfo 获取
9. 执行流程保护：严禁变更核心逻辑和核心数据的执行流程，重构时必须保持原有调用顺序不变

参考文档：TestWorld_Coding_Conventions.md
框架文档：TestWorld_Framework_Architecture.md
```

---

## 1. 命名规范

### 1.1 类命名

| 层级 | 前缀 | 格式 | 示例 | 说明 |
|------|------|------|------|------|
| CoreGlue 接口 | I | `I{Domain}{Capability}` | `ISpatialQueryService` | 域+能力 |
| CoreGlue 接口UObject包装 | U | `U{Domain}{Capability}` | `USpatialQueryService` | 与I前缀对应 |
| CoreGlue 结构体 | F | `F{Domain}{Name}` | `FSpatialQueryHandle` | 域+名称 |
| CoreGlue 枚举 | E | `E{Domain}{Type}` | `EDamageType` | 域+类型 |
| Plugin Manager | U | `U{PluginName}Manager` | `UCombatManager` | 插件名+Manager |
| Plugin Component | U | `U{Feature}Component` | `UCombatComponent` | 功能+Component |
| Glue 类 | U | `U{Domain}Glue` | `UCombatGlue` | 域+Glue |
| 业务 Actor | A | `AGame{Role}` | `AGameHero`, `AGameAI` | Game前缀+角色 |
| 业务 DataAsset | U | `U{Feature}Config` | `UCombatConfig` | 功能+Config |

### 1.2 避免与 UE 冲突的命名

| 禁止使用 | 原因 | 替代方案 |
|----------|------|----------|
| `IInterface` | UE 宏保留 | `I{Domain}Service` / `I{Domain}Provider` |
| `FDelegate` | UE 保留 | `FOn{EventDescription}` |
| `GetActorLocation()` | 绕过架构 | `IGridObjectInfo::GetGridPosition()` |
| `GetObjectInterface()` | 与 UE `GetInterface` 冲突 | `GetObjectFromHandle()` |
| `OnDestroyed` | 与 UE Actor 事件冲突 | `OnActorRemovedFromGrid` |
| `OnHit` | 与 UE 碰撞事件冲突 | `OnCombatHitProcessed` |
| `IsValid()` | 与 UE `IsValid()` 冲突 | 对 Handle 用 `IsValid()`，对 Object 用 `IsGridObjectActive()` |
| `EDamageType` | 已移至 CoreGlue | 统一 include `DamageTypes.h` |

### 1.3 文件命名

| 类型 | 格式 | 示例 |
|------|------|------|
| 接口文件 | `{InterfaceName}.h` | `SpatialQueryService.h` |
| Manager 头文件 | `{ManagerName}.h` | `CombatManager.h` |
| Glue 头文件 | `{Domain}Glue.h` | `CombatGlue.h` |
| 类型文件 | `{Domain}Types.h` | `DamageTypes.h` |
| Config 文件 | `{Feature}Config.h` | `CombatConfig.h` |

### 1.4 成员变量命名

| 类型 | 前缀 | 示例 | 说明 |
|------|------|------|------|
| 接口指针（非UPROPERTY） | 无 | `SpatialQuery` | `ISpatialQueryService* SpatialQuery` |
| UObject 指针（UPROPERTY） | 无 | `CombatManager` | `TObjectPtr<UCombatManager> CombatManager` |
| 布尔值 | b | `bSuspended` | `bool bSuspended` |
| 委托成员（私有） | 无 | `OnActorRemovedFromGridDelegate` | 私有委托成员加 Delegate 后缀 |
| DelegateHandle | 无 | `ActorRemovedHandle` | `FDelegateHandle ActorRemovedHandle` |
| 常量 | 无 | `INVALID_GROUP_ID` | `static constexpr int32 INVALID_GROUP_ID` |

---

## 2. CoreGlue 接口模板

### 2.1 服务接口（Service Interface）

```cpp
// CoreGlue/Public/{Domain}Service.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "{Domain}Service.generated.h"

// [可选] 传入参数结构体
USTRUCT(BlueprintType)
struct F{Domain}RequestParams
{
    GENERATED_BODY()

    // UPROPERTY() 成员...
};

// [可选] 返回结果结构体
USTRUCT(BlueprintType)
struct F{Domain}Result
{
    GENERATED_BODY()

    // UPROPERTY() 成员...
};

UINTERFACE(MinimalAPI, Blueprintable)
class U{Domain}Service : public UInterface
{
    GENERATED_BODY()
};

class COREGLUE_API I{Domain}Service
{
    GENERATED_BODY()

public:
    // 纯虚函数，必须由 Plugin 实现
    virtual F{Domain}Result ExecuteRequest(
        const F{Domain}RequestParams& Params) = 0;

    // [可选] 带默认实现的便捷方法
    virtual bool IsAvailable() const { return true; }
};
```

### 2.2 事件提供者接口（Event Provider Interface）

```cpp
// CoreGlue/Public/{Domain}EventProvider.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "{Domain}EventProvider.generated.h"

// 委托声明：FOn + 事件描述
DECLARE_MULTICAST_DELEGATE_OneParam(FOn{EventDescription}, AActor*);
// [可选] 多参数
DECLARE_MULTICAST_DELEGATE_TwoParams(FOn{EventDescription}WithInfo, int32, AActor*);

UINTERFACE(MinimalAPI, Blueprintable)
class U{Domain}EventProvider : public UInterface
{
    GENERATED_BODY()
};

class COREGLUE_API I{Domain}EventProvider
{
    GENERATED_BODY()

public:
    // 返回委托引用，供外部订阅
    virtual FOn{EventDescription}& On{EventDescription}() = 0;
};
```

### 2.3 数据对象接口（Object Info Interface）

```cpp
// CoreGlue/Public/{Domain}ObjectInfo.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "{Domain}ObjectInfo.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class U{Domain}ObjectInfo : public UInterface
{
    GENERATED_BODY()
};

class COREGLUE_API I{Domain}ObjectInfo
{
    GENERATED_BODY()

public:
    // 常量：INVALID_{NAME}_ID 命名
    static constexpr int32 INVALID_{NAME}_ID = -1;

    // 只读查询（virtual + 默认实现）
    virtual AActor* GetOwnerActor() const { return nullptr; }
    virtual FVector GetGridPosition() const { return FVector::ZeroVector; }
    virtual bool IsGridObjectActive() const { return true; }

    // [可选] 可写方法（虚函数 + 空实现）
    virtual void SetGroupID(int32 InGroupID) {}
    virtual void OnGroupJoined(int32 InGroupID, uint8 InRole) {}
};
```

### 2.4 共享枚举/类型

```cpp
// CoreGlue/Public/{Domain}Types.h
#pragma once

#include "CoreMinimal.h"
#include "{Domain}Types.generated.h"

UENUM(BlueprintType)
enum class E{Domain}{Category} : uint8
{
    Default,
    // ...
};
```

---

## 3. Plugin 实现模板

### 3.1 Manager 实现接口

```cpp
// {Plugin}/Public/{Name}Manager.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "{Domain}Service.h"          // [必须] 实现的 CoreGlue 接口
#include "{Domain}EventProvider.h"     // [可选] 实现的事件提供者
#include "{Name}Manager.generated.h"

class U{Name}Manager : public UWorldSubsystem
    , public I{Domain}Service        // [必须] 实现的服务接口
    // [可选] , public I{Domain}EventProvider
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize(FSubsystemCollectionBase& Collection) override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    // [必须] 接口注入方法（由 Glue 调用）
    void Set{Domain}Service(I{Domain}Service* InService) { {Domain}Service = InService; }
    I{Domain}Service* Get{Domain}Service() const { return {Domain}Service; }

    // [必须] 实现 CoreGlue 接口
    virtual F{Domain}Result ExecuteRequest(
        const F{Domain}RequestParams& Params) override;

    // [可选] 实现事件提供者接口
    virtual FOn{EventDescription}& On{EventDescription}() override
    {
        return On{EventDescription}Delegate;
    }

    // Plugin 自有公开方法
    // ...

private:
    // [必须] 注入的接口指针（非 UPROPERTY）
    I{Domain}Service* {Domain}Service = nullptr;

    // [可选] 事件委托成员（加 Delegate 后缀避免与访问函数冲突）
    FOn{EventDescription} On{EventDescription}Delegate;

    // Plugin 内部状态
    // ...
};
```

### 3.2 接口注入模式

```cpp
// Manager 内部使用注入的接口
void U{Name}Manager::DoSomething()
{
    if (!{Domain}Service)
        return;

    // 通过接口调用，零延迟
    F{Domain}Result Result = {Domain}Service->ExecuteRequest(Params);
    // 处理结果...
}
```

### 3.3 Handle 转换（如需桥接 SparseGrid）

```cpp
// 在 Manager 中提供 Handle 转换
static FSpatialQueryHandle ToSpatialQueryHandle(const F{Internal}Handle& Handle)
{
    FSpatialQueryHandle Out;
    Out.Index = Handle.Index;
    Out.Version = Handle.Version;
    return Out;
}

static F{Internal}Handle To{Internal}Handle(const FSpatialQueryHandle& Handle)
{
    F{Internal}Handle Out;
    Out.Index = Handle.Index;
    Out.Version = Handle.Version;
    return Out;
}
```

---

## 4. Glue 类模板

### 4.1 Glue 头文件

```cpp
// Source/TestWorld/Public/Glue/{Domain}Glue.h
#pragma once

#include "CoreMinimal.h"
#include "GlueBase.h"
#include "EventBus.h"              // [可选] 如需发布/订阅事件
#include "{Domain}Glue.generated.h"

class U{Plugin}Manager;
class UOtherManager;
class UEventBus;

UCLASS()
class TESTWORLD_API U{Domain}Glue : public UGlueBase
{
    GENERATED_BODY()

public:
    virtual void Initialize(UWorld* World) override;
    virtual void Shutdown() override;
    virtual void Tick(float DeltaTime) override;

    // [必须] Bind 方法：注入 Plugin 引用 + 接口绑定
    void Bind(
        U{Plugin}Manager* In{Plugin}Manager,
        UOtherManager* InOtherManager /* [可选] 更多依赖 */
    );

    // [可选] 暴露查询接口给其他 Glue
    U{Plugin}Manager* Get{Plugin}Manager() const { return {Plugin}Manager; }

private:
    // [必须] 持有的 Plugin 引用（UPROPERTY + TObjectPtr）
    UPROPERTY()
    TObjectPtr<U{Plugin}Manager> {Plugin}Manager;

    UPROPERTY()
    TObjectPtr<UOtherManager> OtherManager;

    // [可选] EventBus 引用
    UPROPERTY()
    TObjectPtr<UEventBus> EventBus;

    // [可选] 事件回调（UFUNCTION 用于 AddDynamic）
    UFUNCTION()
    void On{EventName}(/* 参数与源委托一致 */);
};
```

### 4.2 Glue 实现文件

```cpp
// Source/TestWorld/Private/Glue/{Domain}Glue.cpp
#include "Glue/{Domain}Glue.h"
#include "{Plugin}Manager.h"
#include "OtherManager.h"

void U{Domain}Glue::Initialize(UWorld* World)
{
}

void U{Domain}Glue::Shutdown()
{
    // [必须] 移除所有 AddDynamic 订阅
    if (OtherManager)
    {
        OtherManager->On{EventName}.RemoveDynamic(this, &U{Domain}Glue::On{EventName});
    }

    // [可选] 移除 EventBus 订阅
    if (EventBus)
    {
        EventBus->On{EventName}.RemoveDynamic(this, &U{Domain}Glue::Handle{EventName});
    }
}

void U{Domain}Glue::Bind(
    U{Plugin}Manager* In{Plugin}Manager,
    UOtherManager* InOtherManager)
{
    {Plugin}Manager = In{Plugin}Manager;
    OtherManager = InOtherManager;

    // [必须] 接口注入：将实现者 Cast 为接口指针
    if ({Plugin}Manager && OtherManager)
    {
        I{Domain}Service* Service = Cast<I{Domain}Service>(OtherManager);
        {Plugin}Manager->Set{Domain}Service(Service);
    }

    // [可选] 订阅事件（DYNAMIC_MULTICAST 用 AddDynamic）
    if (OtherManager)
    {
        OtherManager->On{EventName}.AddDynamic(this, &U{Domain}Glue::On{EventName});
    }
}

void U{Domain}Glue::Tick(float DeltaTime)
{
    if (bSuspended || !{Plugin}Manager)
        return;

    // 驱动 Plugin 逻辑
}

void U{Domain}Glue::On{EventName}(/* 参数 */)
{
    // [可选] 转发事件到 EventBus
    if (EventBus)
    {
        F{EventName}Event Event;
        // 填充 Event 字段...
        EventBus->On{EventName}.Broadcast(Event);
    }
}
```

---

## 5. Delegate 规范

### 5.1 两种委托体系

| 体系 | 声明宏 | 订阅方式 | 取消订阅 | 适用场景 |
|------|--------|----------|----------|----------|
| **EventBus 事件** | `DECLARE_DYNAMIC_MULTICAST_DELEGATE` | `AddDynamic` | `RemoveDynamic` | Glue 间异步通知 |
| **接口事件** | `DECLARE_MULTICAST_DELEGATE` | `AddUObject` | `Remove(Handle)` | Plugin 内事件暴露 |

### 5.2 EventBus 事件模板

```cpp
// ── 步骤1: 在 EventBus.h 中声明事件结构体和委托 ──

USTRUCT(BlueprintType)
struct F{EventName}Event
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<AActor> TargetActor;  // [可选] 弱引用

    UPROPERTY()
    float Value = 0.0f;                  // [可选] 数值

    UPROPERTY()
    FVector Location = FVector::ZeroVector; // [可选] 位置
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOn{EventName}, const F{EventName}Event&, Event);

// ── 步骤2: 在 UEventBus 类中声明委托成员 ──

UCLASS(BlueprintType)
class COREGLUE_API UEventBus : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "EventBus")
    FOn{EventName} On{EventName};
};

// ── 步骤3: 在 Glue 中发布事件 ──

void U{Domain}Glue::OnSomethingHappened(/* ... */)
{
    if (EventBus)
    {
        F{EventName}Event Event;
        Event.TargetActor = SomeActor;
        Event.Value = SomeValue;
        EventBus->On{EventName}.Broadcast(Event);
    }
}

// ── 步骤4: 在 Glue 中订阅事件（Bind 中） ──

EventBus->On{EventName}.AddDynamic(this, &U{Domain}Glue::Handle{EventName});

// ── 步骤5: 在 Glue 中取消订阅（Shutdown 中） ──

EventBus->On{EventName}.RemoveDynamic(this, &U{Domain}Glue::Handle{EventName});

// ── 步骤6: 回调函数签名 ──

UFUNCTION()
void Handle{EventName}(const F{EventName}Event& Event);
```

### 5.3 接口事件模板

```cpp
// ── 步骤1: 在接口头文件中声明委托 ──

DECLARE_MULTICAST_DELEGATE_OneParam(FOn{EventName}, AActor*);

// ── 步骤2: 在接口中声明访问函数 ──

class I{Domain}EventProvider
{
public:
    virtual FOn{EventName}& On{EventName}() = 0;
};

// ── 步骤3: 在实现类中声明委托成员（加 Delegate 后缀） ──

class U{Plugin}Manager : public UWorldSubsystem, public I{Domain}EventProvider
{
    // ...
    FOn{EventName} On{EventName}Delegate;

    virtual FOn{EventName}& On{EventName}() override
    {
        return On{EventName}Delegate;
    }
};

// ── 步骤4: 在消费者中订阅（AddUObject + 保存 Handle） ──

FDelegateHandle {EventName}Handle;

// 订阅：
{Provider}->On{EventName}().AddUObject(this, &U{Consumer}::Handle{EventName});

// 取消订阅：
{Provider}->On{EventName}().Remove({EventName}Handle);

// ── 步骤5: 触发事件 ──

On{EventName}Delegate.Broadcast(/* params... */);
```

### 5.4 委托命名约定

| 场景 | 委托类型名 | 成员变量名 | 访问函数名 |
|------|-----------|-----------|-----------|
| EventBus 事件 | `FOn{EventName}` | `On{EventName}` | N/A（直接访问成员） |
| 接口事件 | `FOn{EventName}` | `On{EventName}Delegate` | `On{EventName}` |
| Plugin 内部事件 | `FOn{EventName}` | `On{EventName}` | N/A |

---

## 6. 接口指针使用规范

### 6.1 禁止在 UFUNCTION/UPROPERTY 中使用裸接口指针

```cpp
// ❌ 错误：
UPROPERTY()
ISpatialQueryService* SpatialQuery;  // 编译错误

UFUNCTION()
void SetService(ISpatialQueryService* Service);  // 编译错误

// ✅ 正确方案1：不用 U 宏（接口指针不需要反射，推荐）
ISpatialQueryService* SpatialQuery = nullptr;

void SetSpatialQueryService(ISpatialQueryService* InService) { SpatialQuery = InService; }

// ✅ 正确方案2：如需 UPROPERTY，使用 TScriptInterface
UPROPERTY()
TScriptInterface<ISpatialQueryService> SpatialQuery;
```

### 6.2 接口注入的标准模式

```cpp
// Glue::Bind() 中注入接口
void UCombatGlue::Bind(UCombatManager* Manager, USparseGridManager* GridManager)
{
    CombatManager = Manager;

    // Cast UObject 到接口指针
    ISpatialQueryService* QueryService = Cast<ISpatialQueryService>(GridManager);
    if (QueryService)
    {
        Manager->SetSpatialQueryService(QueryService);
    }
}
```

---

## 7. Build.cs 依赖规范

### 7.1 Plugin Build.cs 模板

```csharp
// {Plugin}/Source/{Module}/{Module}.Build.cs
using UnrealBuildTool;

public class {Module} : ModuleRules
{
    public {Module}(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "CoreGlue"    // [必须] 唯一允许的跨 Plugin 依赖
            }
        );

        // [禁止] 不要在这里添加其他 Plugin 模块名
        // ❌ "SparseGridPlugin"
        // ❌ "CombatSystemPlugin"
        // ❌ "BattleDamagePlugin"
    }
}
```

### 7.2 业务层 Build.cs

```csharp
// 业务层可以依赖所有 Plugin
PublicDependencyModuleNames.AddRange(
    new string[]
    {
        "Core",
        "CoreUObject",
        "Engine",
        "CoreGlue",
        "SparseGridPlugin",
        "CombatSystemPlugin",
        "BattleDamagePlugin",
        "ProjectilePlugin",
        "GOAPPlugin",
        // ... 其他 Plugin
    }
);
```

---

## 8. 执行流程保护规范

### 8.1 核心原则

**严禁变更核心逻辑和核心数据的执行流程。** PluginCoordinator 作为纯调度器，其 Tick 中的调用顺序是系统正确性的基础，任何重构不得改变原有执行顺序。

### 8.2 具体规则

| 规则 | 说明 |
|------|------|
| 调用顺序不可变 | PluginCoordinator::Tick 中各 Stage 的调用顺序必须与重构前完全一致 |
| 新增逻辑不可插入 | 不得在现有调用之间插入新的逻辑步骤，只能追加到对应 Stage 末尾 |
| Glue 多 Tick 接口 | Glue 类可提供多个 Tick 接口（如 `TickSpatialUpdate`、`TickCollisionDetection`），由 Coordinator 在不同位置调用 |
| 条件守卫不可移除 | `bGameplaySuspended`、`CVarDisableGOAP` 等条件守卫必须保留在原位置 |
| 性能分析标记保留 | `SCOPED_NAMED_EVENT`、`TRACE_CPUPROFILER_EVENT_SCOPE` 等性能标记必须随逻辑迁移到 Glue |

### 8.3 迁移检查清单

将业务逻辑从 Coordinator 迁移到 Glue 时，必须逐项确认：

- [ ] 调用位置（在哪个 Stage、哪个条件分支内）与迁移前一致
- [ ] 调用顺序（相对于其他调用的先后关系）与迁移前一致
- [ ] 条件守卫（bGameplaySuspended 等）仍由 Coordinator 控制
- [ ] 性能分析标记已迁移到 Glue 对应方法中
- [ ] 功能行为与迁移前完全一致（无逻辑变更）

---

## 9. 常见陷阱

| 陷阱 | 症状 | 解决方案 |
|------|------|----------|
| 接口指针加 UPROPERTY | 编译错误：`Property and function argument pointers cannot be interfaces` | 接口指针不加 U 宏，或用 `TScriptInterface` |
| EventBus 订阅未取消 | 内存泄漏/重复回调 | 在 `Shutdown()` 中调用 `RemoveDynamic` |
| Handle 未保存导致无法取消 | 无法取消订阅 | 保存 `FDelegateHandle` 成员变量 |
| 委托类型名不一致 | 链接错误 | CoreGlue 中统一声明，Plugin 通过 include 引用 |
| `FOnXxx` 与 `FOnXxxDelegate` 混淆 | 编译错误 | 接口事件：成员加 `Delegate` 后缀，访问函数不加 |
| `Cast<IInterface>(this)` 在 const 方法中 | 编译错误 | 使用 `Cast<IInterface>(const_cast<ThisType*>(this))` |
| 不同结构体直接赋值 | 编译错误 | 逐字段映射（如 `FProjectileLaunchConfig` → `FProjectileConfig`） |
| Plugin A include Plugin B 的头文件 | 编译/链接错误 | 通过 CoreGlue 接口解耦 |
