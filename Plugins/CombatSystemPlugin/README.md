# CombatSystemPlugin

## 一句话总结

CombatSystemPlugin 是数据驱动的战斗执行层，负责技能队列、攻击阶段、挥击检测、投射物发射、伤害委派和攻击许可门控。

## 核心职责

| 类型 | 文件 | 职责 |
| --- | --- | --- |
| `UCombatManager` | `Public/CombatManager.h` | 战斗中央管理器，持有配置表和外部服务接口，负责技能执行、范围查询、伤害委派和攻击许可门控 |
| `UCombatComponent` | `Public/CombatComponent.h` | Actor 战斗组件，加载角色/武器技能，维护自动技能队列和攻击阶段状态机 |
| `UCombatSkill` | `Public/CombatSkill.h` | 技能基类，提供配置缓存、前置条件和执行生命周期 |
| `UCombatWeapon` | `Public/CombatWeapon.h` | 武器技能基类，负责目标选择和伤害计算入口 |
| `UMeleeWeapon` | `Public/MeleeWeapon.h` | 近战武器实现 |
| `URangeWeapon` | `Public/RangeWeapon.h` | 远程武器实现，通过 `IProjectileService` 发射投射物 |
| `UAuraManager` | `Public/AuraManager.h` | Aura 效果实例管理 |

## CrowdSurround 攻击门控

CombatSystem 不参与围攻站位，但会尊重 `ICrowdSurroundService` 的攻击许可：

- `UCombatManager::SetCrowdSurroundService` 注入围攻服务。
- `UCombatManager::CanActorAttackFromSurround` 查询当前 Actor 是否允许攻击。
- 未注册到围攻组的 Actor 默认允许攻击，保证玩家、非围攻单位和特殊技能不受影响。
- 已注册到围攻组的 AI 必须处于攻击带并获得 CrowdSurround 许可，才允许自动技能队列或主动攻击真正启动。

## 技能队列流程

1. `BeginPlay` 从 CharacterTable / WeaponTable / SkillTable 加载技能配置。
2. 自动技能进入 `FScheduledSkill` 队列。
3. 每帧检查冷却、静态可行性、CrowdSurround 攻击许可和技能前置条件。
4. 通过 Windup → Active → FollowThrough → Idle 推进技能状态。
5. Active 阶段执行范围查询、阵营过滤、命中去重和伤害委派。

## 与其他系统的边界

- 伤害数值和击杀处理交给 BattleDamagePlugin 的 `IDamageService`。
- 空间查询交给 SparseGridPlugin 的 `ISpatialQueryService`。
- 投射物生成交给 ProjectilePlugin 的 `IProjectileService`。
- AI 决策由 GOAPPlugin 输出。
- 围攻站位由 SparseGridPlugin 的 `UCrowdSurroundManager` 解算。
