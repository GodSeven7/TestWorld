# GOAPPlugin

GOAPPlugin provides goal-driven AI behavior planning. It decides whether an AI
should patrol, chase, or attack, but it does not compute path directions,
local avoidance, or combat hit results.

## Core Responsibilities

| Type | File | Responsibility |
| --- | --- | --- |
| `UGOAPManager` | `Public/GOAPManager.h` | Agent registration, world-state updates, target selection, planning, and action scheduling. |
| `FGOAPPlanner` | `Public/GOAPPlanner.h` | A* action sequence search. |
| `UGOAPAgentComponent` | `Public/GOAPAgentComponent.h` | Default AI agent implementation. |
| `IGOAPAgentInterface` | `Public/GOAPAgentInterface.h` | Agent query/apply/callback protocol. |
| `FGOAPActionExecutor` | `Public/GOAPActionExecutor.h` | Base action executor with OnEnter/OnExit lifecycle. |
| `FChaseActionExecutor` | `Private/Executors/ChaseActionExecutor.cpp` | Chase logic — move toward target. |
| `FAttackActionExecutor` | `Private/Executors/AttackActionExecutor.cpp` | Attack logic — lock Crowd position, request combat. |
| `FPatrolActionExecutor` | `Private/Executors/PatrolActionExecutor.cpp` | Patrol waypoint logic. |
| `FReturnActionExecutor` | `Private/Executors/ReturnActionExecutor.cpp` | Return-to-spawn logic. |

## Executor Lifecycle

Each action executor follows a three-phase lifecycle:

```
OnEnter(Agent)           ← Called once when the action starts executing
       ↓
Execute() / Apply()      ← Called every tick while the action is active
       ↓
OnExit(Agent, Reason)    ← Called once when the action ends
```

`EGOAPActionExitReason` indicates why the action ended:

| Reason | Meaning |
| --- | --- |
| `Completed` | `CheckCompletion` returned true |
| `Aborted` | `ShouldAbort` returned true (precondition violated) |
| `Failed` | `bActionFailed` was set (unrecoverable error) |
| `PlanInvalid` | Plan was invalidated externally |

`OnEnter` is the place to acquire resources (e.g. lock a Crowd position).
`OnExit` is the place to release resources (e.g. unlock a Crowd position).
This ensures every acquire has a paired release regardless of exit path.

## Data Output

`FGOAPAgentContext` is GOAP's only behavior-intent output channel:

- `DesiredPosition`: the position the AI wants to move to.
- `MoveSpeed`: current move speed; 0 means the AI intentionally stops.
- `bIsAttacking`: whether the AI wants to execute an attack.
- `LookDirection` / `TargetLocation`: facing and target data.

## CrowdSurround Integration

GOAP interacts with `ICrowdSurroundService` through two lifecycle calls only:

| Call | When | Purpose |
| --- | --- | --- |
| `LockSurroundAssignment` | `FAttackActionExecutor::OnEnter` | Lock current position so other agents don't take it |
| `UnlockSurroundAssignment` | `FAttackActionExecutor::OnExit` | Release the locked position (any exit reason) |

GOAP does not register agents with Crowd, query Crowd assignments, or read
Crowd internal data. Agent registration and position assignment are handled
outside GOAP.

## Boundaries

- GOAP does not compute path directions; FlowField provides navigation direction.
- GOAP does not perform local avoidance; Steering handles it.
- GOAP does not solve crowd push; SparseGrid PBD handles it.
- GOAP does not apply damage; combat requests go through `ICombatQueryService`.
- GOAP does not distribute attack anchors or wait clusters; `ICrowdSurroundService` owns that logic.
- GOAP does not register agents to Crowd or query Crowd positions; other layers handle that.
