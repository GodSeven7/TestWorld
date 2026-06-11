# SparseGridPlugin

SparseGridPlugin is the spatial infrastructure layer. It owns sparse-grid object
registration, collision/PBD push solving, FlowField navigation, steering, static
obstacles, and RTS-style crowd surround assignment.

## Modules

| Module | Core Type | Responsibility |
| --- | --- | --- |
| SparseGrid | `USparseGridManager` / `USparseGrid` | Object registration, spatial queries, ray tests, and collision detection entry points. |
| Collision/PBD | `FSparseGridCollisionDetector` / `FSparseGridPBDContactSolver` | Stable disk overlap correction and external-force push propagation. |
| FlowField | `UFlowFieldManager` / `UNavigationGrid` | Static navigation grid, static obstacle cells, integration fields, direction fields, and path debug. |
| Steering | `USparseGridManager::ApplySteeringBehaviors` | Converts `DesiredPosition` into final velocity with local avoidance and direct near-field approach. |
| CrowdSurround | `UCrowdSurroundManager` | Manager-owned attack anchors, balanced wait clusters, refill assignment, and attack permission. |
| StaticObstacle | `USparseGridStaticObstacleComponent` | Registers placed static objects into FlowField and optional local avoidance. |
| Debug | `SparseGridDebug` | Debug drawing switches for grid, collision, FlowField, steering, PBD, and CrowdSurround. |

## CrowdSurround Model

CrowdSurround no longer uses the old soft outer-band solver. The current
model is ownership based:

- `AttackAnchor`: a dynamic attack position around the target. One anchor can
  have at most one owner. Free anchors are generated near the current AI
  approach directions instead of starting from a fixed world-space angle.
- `WaitCluster`: a layered fan/tree-shaped waiting cluster behind one
  `AttackAnchor`.
- `WaitPoint`: a concrete waiting position in a cluster. One wait point can have
  at most one owner.
- `RefillCandidate`: a front-layer waiting AI behind an attack anchor. When that
  anchor opens, the manager picks the best candidate from the front layer.
- `Reserved`: an AI owns an attack anchor and is moving toward it.
- `Occupied`: an AI has reached an attack anchor and can attack.

The important rule is:

```text
AI moves only toward the position assigned by UCrowdSurroundManager.
AI never directly rushes the target center while registered in CrowdSurround.
```

For waiting AI, the manager first generates a balanced pool of usable
`WaitPoint`s across all usable clusters, then globally matches AI to those
points. The matching cost favors nearby same-sector movement, reachable points,
front layers, and existing stable ownership, so a clearly closer AI can replace
a farther owner without causing every waiter to churn.

For attacking AI, a requested attack point is also only a preference. The
manager checks the requested point against occupied and reserved attack points
using body radii plus contact padding. If the point is blocked by an occupied
attacker, the manager searches the nearest legal point on the attack ring. If
it is blocked by a not-yet-arrived reservation, a closer requester can steal
that reservation and the old owner re-enters assignment.

Natural attack capacity is based on the actual desired attack radius and
straight-line chord spacing between adjacent attack points. It is not computed
from outer attack range arc length, which would overestimate how many units can
stand without overlapping.

Capacity changes never prune existing `Occupied` attackers. They only limit new
attack reservations; at full capacity, a new requester can move into attack only
by stealing a not-yet-arrived `Reserved` point.

## Runtime Flow

1. GOAP Chase/Attack outputs `bWantsCrowdSurround=true`, target object, and move speed.
2. The agent registers through `ICrowdSurroundService::RegisterSurroundAgentWithParams`.
3. `UCrowdSurroundManager` refreshes target metrics and dynamic attack anchors.
4. Existing `Occupied` anchors are preserved while the owner stays registered; GOAP releases them explicitly when behavior state says they are no longer valid.
5. Existing `Reserved` anchors are preserved while the owner makes progress and the point remains spatially valid.
6. Invalid, stale, stuck, or overlapping reservations are released.
7. Unassigned AI request preferred attack points; the manager resolves conflicts, nearest legal alternatives, and reserved-point steals.
8. All extra AI are globally matched into balanced per-anchor layered `WaitPoint`s.
9. GOAP reads `FCrowdSurroundAssignment` and writes `DesiredPosition` / `MoveSpeed`.
10. CombatSystem calls `CanAttackFromSurround`; only permitted surround attackers can start attacks.
11. PBD prevents overlap; ordinary push cannot move an occupied attacker, but external force can propagate.

## CrowdSurround Files

| File | Responsibility |
| --- | --- |
| `Public/CrowdSurround/CrowdSurroundManager.h` | WorldSubsystem and `ICrowdSurroundService` declarations. |
| `Public/CrowdSurround/CrowdSurroundRuntimeTypes.h` | Participants, attack anchors, wait clusters, and target group runtime data. |
| `Private/CrowdSurround/CrowdSurroundManager.cpp` | Registration, unregistration, tick, query, and service implementation. |
| `Private/CrowdSurround/CrowdSurroundGeometry.cpp` | Attack radius, spacing, anchor direction, capacity, and walkability helpers. |
| `Private/CrowdSurround/CrowdSurroundSolver.cpp` | Assignment solving, occupied-anchor preservation, reservation timeout, refill, wait-cluster assignment, and validation. |
| `Private/CrowdSurround/CrowdSurroundDebug.cpp` | `SparseGrid.ShowCrowd` debug drawing. |
| `Private/CrowdSurround/CrowdSurroundUtilities.h` | Position flattening, assignment colors, and circle drawing helpers. |

## Static Obstacles

- `USparseGridStaticObstacleComponent` is for manually placed walls, pillars,
  rocks, and similar static objects.
- `bAffectsFlowField` controls whether it blocks FlowField cells.
- `bRegistersLocalAvoidance` controls whether it participates in SparseGrid
  collision/local avoidance.
- Static obstacles resist ordinary crowd push and external-force domino movement.

## Configuration

`USparseGridConfig` centralizes SparseGrid, navigation, steering, PBD, and
CrowdSurround parameters.

| Parameter / Getter | Meaning |
| --- | --- |
| `BaseRadius` | Project spatial scale baseline. |
| `PushConfig` | PBD push enablement, iterations, strength, and max correction. |
| `PushConfig.ExternalPushPropagationDuration` | How long a pushed object remains externally active. |
| `PushConfig.ExternalPushMinPropagationDistance` | Minimum correction needed to propagate external force. |
| `MaxFlowFieldGenerationsPerFrame` | Synchronous FlowField generation budget per frame. |
| `MaxAttackersPerTarget` | 0 means natural ring capacity; otherwise caps attackers per target. |
| `CrowdSurroundUpdateInterval` | Surround assignment refresh interval. |
| `bEnableCrowdWallCheck` | Whether CrowdSurround filters unwalkable points. |
| `GetMinCrowdSpacing()` | Minimum crowd spacing. |
| `GetCrowdReachTolerance()` | Arrival tolerance for surround positions. |
| `GetWaitClusterSpacing()` | Distance between wait-cluster layers. |
| `GetCrowdDirectMoveRadius()` | Near-field direct approach radius for surround assignments. |
| `GetCrowdContactPadding()` | Extra padding around collision disks. |
| `GetAttackBandInnerBuffer()` | Desired attack radius offset from max attack range. |
| `GetCrowdReservationTimeout()` | Time without progress before an attack reservation is released. |
| `GetCrowdProgressEpsilon()` | Minimum distance improvement considered progress. |

## Debug

| Console Variable | Effect |
| --- | --- |
| `SparseGrid.ShowCrowd 1` | Draws attack radius, attack anchors, wait clusters, assigned positions, parent links, and labels. |
| `SparseGrid.ShowPBD 1` | Draws PBD correction vectors. |
| `SparseGrid.ShowSteeringTargets 1` | Draws steering targets and final velocity direction. |
| `SparseGrid.FlowFieldPath 1` | Draws paths traced through the integration field. |
| `SparseGrid.FlowFieldDirection 1` | Draws FlowField directions. |
| `SparseGrid.CollisionDebug 1` | Draws collision radii. |

`SparseGrid.ShowCrowd` labels:

- `Crowd Anchors:x/y Wait:z R:min-max`: target group summary.
- `Hn FREE/RES/OCC/BLK owner`: attack anchor state.
- `Wn layer branch owner parent`: wait-cluster point state.
- `A ID H W L B MTA/ATK OK/I/M/L`: attack assignment.
- `Q ID H W L B MTW/WAIT R/I/M`: wait assignment, where `R` means front-layer refill candidate.

## Boundaries

- FlowField receives static obstacles only, not dynamic surround positions.
- Steering moves toward the manager-assigned point; it does not decide attack permission.
- PBD keeps collision disks separated and propagates external force.
- CrowdSurround does not move actors directly. It outputs position intent and attack permission.
