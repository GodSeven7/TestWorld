# GameUIPlugin

## 一句话总结

这是一个游戏UI插件，用于管理游戏界面的生命周期和交互，提供HUD、开始菜单、设置菜单和结算界面的统一创建/显示/隐藏控制及游戏流程请求转发。

## 核心功能详解

### 1. 核心机制

插件以 `UGameUISubsystem`（GameInstanceSubsystem）为UI管理中枢，根据 `UGameUIConfig`（DataAsset）中配置的Widget类创建和管理四个界面实例（HUD/StartMenu/SettingMenu/GameOverMenu）。`UGameDataSubsystem`（GameInstanceSubsystem）作为游戏数据仓库，持有游戏时间、敌人数量、玩家生命值和游戏结果等数据，数据变更时通过委托广播通知所有Widget。`UGameWidgetBase` 作为所有界面Widget的基类，自动订阅GameDataSubsystem的事件并在NativeEvent中派发到子类。

### 2. 关键类与职责

| 类名 | 文件位置 | 职责 |
|------|---------|------|
| UGameUISubsystem | Public/GameUISubsystem.h | UI管理子系统，负责Widget创建/销毁/显示/隐藏和游戏流程请求转发 |
| UGameDataSubsystem | Public/GameDataSubsystem.h | 游戏数据子系统，持有游戏运行数据并广播变更事件 |
| UGameWidgetBase | Public/GameWidgetBase.h | Widget基类，自动订阅GameDataSubsystem事件并提供BlueprintNativeEvent扩展点 |
| UGameUIConfig | Public/GameUIConfig.h | UI配置DataAsset，定义四种Widget的类引用 |
| UGameHUDWidget | Public/GameHUDWidget.h | HUD界面，显示游戏时间/敌人数量/生命值/游戏结果 |
| UGameStartMenuWidget | Public/GameStartMenuWidget.h | 开始菜单，提供开始游戏和设置入口按钮 |

### 3. 对外接口

**UGameUISubsystem BlueprintCallable 方法：**
- `SetUIConfig(Config)` — 设置UI配置
- `ShowHUD()` / `HideHUD()` — 显示/隐藏HUD
- `ShowStartMenu()` / `HideStartMenu()` — 显示/隐藏开始菜单
- `ShowSettingMenu()` / `HideSettingMenu()` — 显示/隐藏设置菜单
- `ShowGameOverMenu()` / `HideGameOverMenu()` — 显示/隐藏结算界面
- `HideAll()` — 隐藏所有界面
- `RequestStartGame()` / `RequestRestartGame()` / `RequestQuitGame()` — 游戏流程请求

**UGameUISubsystem 委托事件：**
- `OnRequestStartGame` — 请求开始游戏
- `OnRequestRestartGame` — 请求重新开始
- `OnRequestQuitGame` — 请求退出游戏

**UGameDataSubsystem BlueprintCallable 方法：**
- `SetGameTime(Time)` / `GetGameTime()` — 设置/获取游戏时间
- `SetEnemyCount(Count)` / `GetEnemyCount()` — 设置/获取敌人数量
- `SetPlayerHealth(Current, Max)` / `GetHealthPercent()` — 设置/获取玩家生命值
- `SetGameResult(Result)` / `GetGameResult()` — 设置/获取游戏结果
- `ResetGameData()` — 重置所有游戏数据

**UGameDataSubsystem 委托事件：**
- `OnGameTimeChanged` — 游戏时间变更
- `OnEnemyCountChanged` — 敌人数量变更
- `OnPlayerHealthChanged` — 玩家生命值变更
- `OnGameResultChanged` — 游戏结果变更

**UGameWidgetBase BlueprintNativeEvent：**
- `OnGameTimeChanged(NewTime)` — 游戏时间变更通知
- `OnEnemyCountChanged(NewCount)` — 敌人数量变更通知
- `OnPlayerHealthChanged(CurrentHealth, MaxHealth)` — 生命值变更通知
- `OnGameResultChanged(NewResult)` — 游戏结果变更通知

### 4. 数据流

数据驱动流程：业务层调用GameDataSubsystem.SetXxx → 数据变更 → 委托广播 → UGameWidgetBase.NativeConstruct时绑定监听 → OnXxxChanged_Implementation → 子类Override更新UI元素

UI交互流程：用户点击按钮（如StartButton）→ Widget内部OnStartClicked → UGameUISubsystem.RequestStartGame → OnRequestStartGame委托 → 业务层监听并执行游戏开始逻辑

界面切换流程：业务层调用ShowStartMenu → Subsystem创建Widget实例（CreateWidgetInstance）→ AddToViewport → 业务层调用HideStartMenu → RemoveFromParent

### 5. 设计要点

- **数据与UI分离**：`UGameDataSubsystem` 纯粹管理数据，`UGameUISubsystem` 纯粹管理界面，通过委托连接
- **GameInstance级别生命周期**：两个Subsystem均为GameInstanceSubsystem，数据跨关卡持久化
- **DataAsset配置**：Widget类通过 `UGameUIConfig` DataAsset配置，支持蓝图子类化自定义UI样式
- **BlueprintNativeEvent模式**：Widget基类使用BlueprintNativeEvent，C++子类Override _Implementation，蓝图子类可覆盖事件
- **BindWidget元数据**：HUD/Menu Widget使用 `meta=(BindWidget)` 绑定UMG控件，编译期检查控件存在性
- **世界切换处理**：`OnWorldChanged` 回调处理世界切换时的Widget失效

### 6. 不做什么

- 不负责游戏逻辑的执行（如敌人生成、关卡管理），仅转发请求
- 不提供UI动画和过渡效果
- 不负责输入模式切换（如显示/隐藏鼠标光标）
- 不提供多语言/本地化支持
