# CoreGlue

CoreGlue is the cross-plugin protocol layer. It defines shared interfaces, data
types, and event-bus contracts, but it does not implement gameplay logic.

## Core Responsibilities

| Type | File | Responsibility |
| --- | --- | --- |
| `IDamageService` | `Public/DamageService.h` | Damage application and damage notifications. |
| `ISpatialQueryService` | `Public/SpatialQueryService.h` | Spatial query protocol. |
| `IProjectileService` | `Public/ProjectileService.h` | Projectile spawn protocol. |
| `ICombatQueryService` | `Public/CombatQueryService.h` | Combat request protocol. |
| `ICrowdSurroundService` | `Public/CrowdSurroundService.h` | Crowd surround registration, position intent, and attack permission protocol. |
| `FCrowdSurroundAgentParams` | `Public/CrowdSurroundTypes.h` | Attack range and collision radius passed by AI when joining a surround group. |
| `FCrowdSurroundAssignment` | `Public/CrowdSurroundTypes.h` | Current manager-owned surround assignment: `AttackAnchor`, `WaitPoint`, desired position, movement state, and attack permission. |
| `UEventBus` | `Public/EventBus.h` | Shared event bus. |
| `UGlueBase` | `Public/GlueBase.h` | Base class for project glue. |
| `UActorUnifiedDataComponent` | `Public/ActorUnifiedDataComponent.h` | Runtime shared actor data. |
| `IGridObjectInfo` | `Public/GridObjectInfo.h` | Shared grid object information protocol. |

## CrowdSurround Protocol

`ICrowdSurroundService` exposes a stable RTS-style position assignment protocol:

- `RegisterSurroundAgent` / `RegisterSurroundAgentWithObject` / `RegisterSurroundAgentWithParams`: add an AI to a target surround group.
- `ReleaseSurroundAssignment`: release the AI's current owned point while keeping the AI eligible to request a new surround position.
- `UnregisterSurroundAgent`: remove an AI when it dies, loses target, or leaves combat.
- `Update`: tick the surround solver from the project layer.
- `GetSurroundAssignment`: query the AI's current `AttackAnchor` or `WaitPoint` assignment.
- `HasAttackPermission`: query whether the AI can actually attack.
- `CanAttackFromSurround`: combat-side attack gate; objects outside a surround group are allowed by default.
- `IsInSurroundGroup`: query whether an AI is registered in a surround group.

`FCrowdSurroundAssignment` intentionally exposes ownership-oriented fields:

- `Type`: `AttackAnchor`, `WaitPoint`, or `None`.
- `State`: `MovingToAttack`, `Attacking`, `MovingToWait`, `Waiting`, or `Reassigning`.
- `AnchorID` / `WaitIndex` / `WaitLayer` / `WaitBranchIndex`: the manager-owned position identity.
- `ParentAgentID`: the front-layer parent reference for a waiting AI.
- `bHasAttackPermission`: true only when the AI owns an attack anchor and is in position.
- `bLocksCrowdPosition`: true for an attacking AI that should resist ordinary crowd push.
- `bIsRefillCandidate`: true for a front-layer waiter that can be selected for refill.

## Boundaries

- CoreGlue defines protocols only.
- SparseGridPlugin implements `UCrowdSurroundManager`.
- GOAP consumes surround assignments and outputs `DesiredPosition` / `MoveSpeed`.
- CombatSystem consumes attack permission only.
- Plugins should collaborate through injected interfaces, not direct implementation dependencies.
