# CameraAdvancePlugin

## 一句话总结

这是一个相机抖动插件，以 WorldSubsystem 为核心管理器，通过正弦衰减算法计算相机偏移量，为攻击命中、受击、击杀等战斗事件提供打击感反馈。

## 核心功能详解

### 1. 核心机制

插件以 `UCameraShakeManager`（WorldSubsystem）为核心管理器，维护活跃抖动实例列表，每帧通过正弦衰减公式计算叠加偏移量。外部通过 `IGameCameraShakeService` 接口请求抖动（传入持续时间和放大倍率），Manager 创建 `FCameraShakeInstance` 运行时实例，在 Tick 中更新并输出 `FGameCameraShakeOffset`（位置+旋转偏移）。

抖动公式：`Offset = Amplitude * sin(Frequency * Time) * exp(-DecayRate * Time) * Scale`

### 2. 关键类与职责

| 类名 | 文件位置 | 职责 |
|------|---------|------|
| UCameraShakeManager | Public/CameraShakeManager.h | 核心管理器，管理抖动实例、每帧计算偏移、实现IGameCameraShakeService |
| IGameCameraShakeService | CoreGlue/Public/GameCameraShakeService.h | 外部调用接口，定义RequestShake/StopAllShakes/GetCurrentOffset |
| FGameCameraShakeOffset | CoreGlue/Public/GameCameraShakeService.h | 抖动偏移结果结构体，包含LocationOffset和RotationOffset |
| FCameraShakeInstance | Public/CameraShakeManager.h | 运行时抖动实例，跟踪剩余时间、放大倍率、已过时间 |
| UCameraAdvanceGlue | TestWorld/Public/Glue/CameraAdvanceGlue.h | 胶水层，订阅EventBus战斗事件触发抖动，将偏移量应用到玩家相机 |

### 3. 对外接口

**IGameCameraShakeService 接口方法：**
- `RequestShake(Duration, Scale)` — 请求抖动，Duration为持续时间(秒)，Scale为放大倍率
- `StopAllShakes()` — 立即停止所有活跃抖动
- `GetCurrentOffset()` — 获取当前叠加偏移量（FGameCameraShakeOffset）

**控制台变量：**
- `Disable.CameraShake` — 运行时开关，设为1禁用相机抖动

### 4. 数据流

事件触发：CombatGlue → EventBus.OnDamageApplied / OnCombatantDeath → CameraAdvanceGlue → ShakeManager.RequestShake

每帧Tick：PluginCoordinator(Phase 9.5) → CameraAdvanceGlue.Tick → ShakeManager.Tick(更新实例+计算偏移) → ApplyShakeOffsetToCamera(写入SpringArm)

抖动实例生命周期：RequestShake → 创建FCameraShakeInstance → 每帧更新ElapsedTime/RemainingTime → RemainingTime<=0时移除

### 5. 设计要点

- **简单正弦衰减**：使用 sin * exp 衰减公式，参数硬编码（位置振幅5、旋转振幅0.5度、频率25Hz、衰减率8），无需外部配置
- **多实例叠加**：多个抖动实例同时活跃时偏移量线性叠加，支持连续命中时的打击感累积
- **Glue模式**：遵循项目Glue模式，CameraAdvanceGlue作为Manager与EventBus之间的桥梁，由PluginCoordinator统一调度
- **接口隔离**：IGameCameraShakeService定义在CoreGlue插件中，CameraAdvancePlugin实现该接口，调用方仅依赖接口不依赖实现
- **Suspend支持**：挂起时不接受新抖动请求，已有抖动继续衰减至自然结束，避免视觉冻结

### 6. 不做什么

- 不负责相机的基础跟随和旋转逻辑，仅叠加抖动偏移
- 不提供多种抖动模式（如方向性抖动、冲击抖动等），仅提供统一的正弦衰减抖动
- 不负责抖动参数的DataAsset配置，参数硬编码在Manager中
- 不提供网络同步
