// Copyright Epic Games, Inc. All Rights Reserved.

#include "SparseGridDebug.h"
#include "HAL/IConsoleManager.h"
#include "SparseGridManager.h"
#include "SparseGrid.h"
#include "SparseGridObject.h"
#include "SparseGridRaycast.h"
#include "FlowFieldManager.h"
#include "NavigationGrid.h"
#include "PathfindingTypes.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"

static TAutoConsoleVariable<int32> CVarDebug(
	TEXT("SparseGrid.Debug"),
	0,
	TEXT("Enable SparseGrid debug visualization (0=Off, 1=On)"),
	ECVF_Default
);

static TAutoConsoleVariable<FString> CVarDebugColor(
	TEXT("SparseGrid.DebugColor"),
	TEXT("Green"),
	TEXT("Debug color: Green, Red, Blue, Yellow, Cyan, White"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarCollision(
	TEXT("SparseGrid.Collision"),
	1,
	TEXT("Enable SparseGrid collision detection (0=Off, 1=On)"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarCollisionDebug(
	TEXT("SparseGrid.CollisionDebug"),
	0,
	TEXT("Show collision debug visualization (0=Off, 1=Show collision radii)"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarRaycastTest(
	TEXT("SparseGrid.RaycastTest"),
	0,
	TEXT("Enable continuous raycast test from camera (0=Off, 1=On)"),
	ECVF_Default
);

static TAutoConsoleVariable<float> CVarRaycastTestDistance(
	TEXT("SparseGrid.RaycastTest.Distance"),
	10000.0f,
	TEXT("Raycast test distance"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarDebugForward(
	TEXT("SparseGrid.DebugForward"),
	0,
	TEXT("Show forward direction arrows on game objects (0=Off, 1=On)"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarFlowFieldPath(
	TEXT("SparseGrid.FlowFieldPath"),
	0,
	TEXT("Show FlowField path for each AI (0=Off, 1=On)"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarFlowFieldDirection(
	TEXT("SparseGrid.FlowFieldDirection"),
	0,
	TEXT("Show FlowField direction arrows for each cell (0=Off, 1=On)"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarDisableSteering(
	TEXT("SparseGrid.DisableSteering"),
	0,
	TEXT("Disable Steering behaviors (0=Enabled, 1=Disabled). When disabled, AI uses direct desired-position velocity."),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarShowCrowd(
	TEXT("SparseGrid.ShowCrowd"),
	0,
	TEXT("Show crowd surround bands, sectors, density, and assignments (0=Off, 1=On)"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarShowPBD(
	TEXT("SparseGrid.ShowPBD"),
	0,
	TEXT("Show PBD contact correction vectors (0=Off, 1=On)"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarShowSteeringTargets(
	TEXT("SparseGrid.ShowSteeringTargets"),
	0,
	TEXT("Show AI desired positions and computed steering velocity (0=Off, 1=On)"),
	ECVF_Default
);

namespace SparseGridDebug
{
	static FLinearColor ParseDebugColor(const FString& ColorName)
	{
		if (ColorName == TEXT("Red")) return FLinearColor::Red;
		if (ColorName == TEXT("Blue")) return FLinearColor::Blue;
		if (ColorName == TEXT("Yellow")) return FLinearColor::Yellow;
		if (ColorName == TEXT("Cyan")) return FLinearColor(0, 1, 1);
		if (ColorName == TEXT("White")) return FLinearColor::White;
		return FLinearColor::Green;
	}

	bool IsDebugEnabled()
	{
		return CVarDebug.GetValueOnGameThread() != 0;
	}

	bool IsCollisionEnabled()
	{
		return CVarCollision.GetValueOnGameThread() != 0;
	}

	bool IsCollisionDebugEnabled()
	{
		return CVarCollisionDebug.GetValueOnGameThread() != 0;
	}

	bool IsDebugForwardEnabled()
	{
		return CVarDebugForward.GetValueOnGameThread() != 0;
	}

	void SetCollisionEnabled(bool bEnabled)
	{
		CVarCollision->Set(bEnabled ? 1 : 0);
	}

	void DrawDebug(UWorld* World, USparseGridManager* GridManager)
	{
		if (!World || !GridManager)
			return;

		USparseGrid* Grid = GridManager->GetSparseGrid();
		if (!Grid)
			return;

		FLinearColor Color = ParseDebugColor(CVarDebugColor.GetValueOnGameThread());

		Grid->DrawDebugEx(World, Color, 0.0f);
	}

	bool IsRaycastTestEnabled()
	{
		return CVarRaycastTest.GetValueOnGameThread() != 0;
	}

	void SetRaycastTestEnabled(bool bEnabled)
	{
		CVarRaycastTest->Set(bEnabled ? 1 : 0);
	}

	float GetRaycastTestDistance()
	{
		return CVarRaycastTestDistance.GetValueOnGameThread();
	}

	void SetRaycastTestDistance(float Distance)
	{
		CVarRaycastTestDistance->Set(Distance);
	}

	void DrawRaycastTest(UWorld* World, USparseGridManager* GridManager)
	{
		if (!World || !GridManager)
			return;

		APlayerController* PC = World->GetFirstPlayerController();
		if (!PC)
			return;

		FVector ViewLocation;
		FRotator ViewRotation;
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

		FVector Direction = ViewRotation.Vector();
		float MaxDistance = GetRaycastTestDistance();

		FSparseGridRaycastHit Hit;
		bool bHit = GridManager->RaycastSingle(ViewLocation, Direction, MaxDistance, Hit);

		FVector RayEnd = ViewLocation + Direction * MaxDistance;

		if (bHit)
		{
			DrawDebugLine(World, ViewLocation, Hit.HitPoint, FColor::Green, false, 0.0f, 0, 2.0f);
			DrawDebugSphere(World, Hit.HitPoint, 10.0f, 12, FColor::Red, false, 0.0f, 0, 2.0f);

			if (Hit.HitActor)
			{
				FString HitInfo = FString::Printf(TEXT("Hit: %s\nDist: %.1f"),
					*Hit.HitActor->GetName(), Hit.HitDistance);
				DrawDebugString(World, Hit.HitPoint + FVector(0, 0, 20), HitInfo, nullptr, FColor::White, 0.0f);
			}
		}
		else
		{
			DrawDebugLine(World, ViewLocation, RayEnd, FColor::Red, false, 0.0f, 0, 0.2f);
		}
	}

	bool IsFlowFieldPathDebugEnabled()
	{
		return CVarFlowFieldPath.GetValueOnGameThread() != 0;
	}

	bool IsFlowFieldDirectionDebugEnabled()
	{
		return CVarFlowFieldDirection.GetValueOnGameThread() != 0;
	}

	void DrawFlowFieldDebug(UWorld* World, USparseGridManager* GridManager)
	{
		if (!World || !GridManager)
			return;

		UFlowFieldManager* FlowFieldMgr = World->GetSubsystem<UFlowFieldManager>();
		if (!FlowFieldMgr)
			return;

		UNavigationGrid* NavGrid = FlowFieldMgr->GetNavigationGrid();
		if (!NavGrid)
			return;

		// 绘制每个AI的寻路路径（直接从FlowField数据追踪，跳过bIsValid检查）
		if (IsFlowFieldPathDebugEnabled())
		{
			static const FIntVector PathNeighborOffsets[] = {
				FIntVector(1, 0, 0), FIntVector(-1, 0, 0),
				FIntVector(0, 1, 0), FIntVector(0, -1, 0),
				FIntVector(1, 1, 0), FIntVector(1, -1, 0),
				FIntVector(-1, 1, 0), FIntVector(-1, -1, 0)
			};

			TArray<ISparseGridObject*> Objects = GridManager->GetAllObjects();
			for (ISparseGridObject* Obj : Objects)
			{
				if (!Obj || !Obj->EnableSteering())
					continue;

				FVector DesiredPos = Obj->QueryDesiredPosition();
				if (DesiredPos.IsNearlyZero())
					continue;

				FVector StartPos = Obj->GetSparseGridPosition();
				FIntVector StartCell = NavGrid->WorldToGrid(StartPos);
				FIntVector TargetCell = NavGrid->WorldToGrid(DesiredPos);

				// 用量化后的CacheKey查找缓存（与游戏逻辑一致）
				FIntVector CacheKey = FlowFieldMgr->QuantizeToCacheLevel(TargetCell);
				FFlowFieldData* FlowField = NavGrid->GetFlowFieldCache(CacheKey);
				if (!FlowField || FlowField->IntegrationField.Num() == 0)
					continue;

				// 用 IntegrationField 追踪路径
				const FIntVector FieldTargetCell = FlowField->TargetCell;
				TArray<FVector> PathPoints;
				FIntVector Current = StartCell;
				int32 MaxSteps = 200;
				int32 Steps = 0;

				PathPoints.Add(NavGrid->GetCellCenter(Current));

				while (Current != FieldTargetCell && Steps < MaxSteps)
				{
					Steps++;
					float CurrentCost = FLT_MAX;
					const float* CostPtr = FlowField->IntegrationField.Find(Current);
					if (CostPtr)
						CurrentCost = *CostPtr;

					if (CurrentCost >= FLT_MAX || CurrentCost <= 0.0f)
						break;

					float BestCost = FLT_MAX;
					FIntVector BestNeighbor = Current;
					for (const FIntVector& Offset : PathNeighborOffsets)
					{
						FIntVector Neighbor(Current.X + Offset.X, Current.Y + Offset.Y, 0);
						const float* NeighborCost = FlowField->IntegrationField.Find(Neighbor);
						if (!NeighborCost || *NeighborCost >= FLT_MAX)
							continue;
						if (*NeighborCost < BestCost)
						{
							BestCost = *NeighborCost;
							BestNeighbor = Neighbor;
						}
					}

					if (BestNeighbor == Current || BestCost >= CurrentCost)
						break;

					PathPoints.Add(NavGrid->GetCellCenter(BestNeighbor));
					Current = BestNeighbor;
				}

				if (Current != FieldTargetCell)
				{
					PathPoints.Add(NavGrid->GetCellCenter(FieldTargetCell));
				}

				if (PathPoints.Num() >= 2)
				{
					for (int32 i = 0; i < PathPoints.Num() - 1; ++i)
					{
						FVector P1 = PathPoints[i]; P1.Z = 10.0f;
						FVector P2 = PathPoints[i + 1]; P2.Z = 10.0f;
						DrawDebugLine(World, P1, P2, FColor::Cyan, false, 0.02f, SDPG_World, 2.0f);
					}
					DrawDebugSphere(World, PathPoints[0] + FVector(0, 0, 10.0f), 8.0f, 8, FColor::Green, false, 0.02f, SDPG_World, 1.0f);
					DrawDebugSphere(World, PathPoints.Last() + FVector(0, 0, 10.0f), 8.0f, 8, FColor::Red, false, 0.02f, SDPG_World, 1.0f);
				}
			}
		}

		// 绘制动态障碍物标记（任一FlowField Debug开启时都绘制）
		// 绘制NavGrid中静态障碍物Cell
		{
			float CellSize = NavGrid->GetCellSize();
			float HalfSize = CellSize * 0.4f;

			const TMap<FIntVector, FNavCell>& AllNavCells = NavGrid->GetAllNavCells();
			for (const auto& Pair : AllNavCells)
			{
				if (!Pair.Value.IsStaticObstacle())
					continue;

				FVector CellCenter = NavGrid->GetCellCenter(Pair.Key);
				CellCenter.Z = 15.0f;
				DrawDebugBox(World, CellCenter, FVector(HalfSize, HalfSize, 5.0f), FColor::Red, false, 0.02f, SDPG_World, 2.0f);
			}
		}

		// 绘制每个Cell的FlowField方向
		if (IsFlowFieldDirectionDebugEnabled())
		{
			float CellSize = NavGrid->GetCellSize();

			// 获取玩家位置作为绘制中心
			FVector ViewCenter = FVector::ZeroVector;
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				if (APawn* Pawn = PC->GetPawn())
				{
					ViewCenter = Pawn->GetActorLocation();
				}
			}

			FIntVector ViewCenterCell = NavGrid->WorldToGrid(ViewCenter);
			// DrawRange基于世界空间半径(1500单位)换算为Cell数量，适配不同CellSize
			float DrawWorldRange = 1500.0f;
			int32 DrawRange = FMath::CeilToInt(DrawWorldRange / NavGrid->GetCellSize());

			// 收集所有AI正在使用的FlowField缓存，优先显示
			const TMap<FIntVector, FFlowFieldData>& AllCaches = NavGrid->GetAllFlowFieldCaches();
			if (AllCaches.Num() == 0)
				return;

			// 找到AI正在使用的缓存（通过AI的DesiredPosition量化后的CacheKey）
			const FFlowFieldData* BestCache = nullptr;
			TArray<ISparseGridObject*> AllObjs = GridManager->GetAllObjects();
			for (ISparseGridObject* Obj : AllObjs)
			{
				if (!Obj || !Obj->EnableSteering())
					continue;
				FVector DesiredPos = Obj->QueryDesiredPosition();
				if (DesiredPos.IsNearlyZero())
					continue;
				FIntVector TargetCell = NavGrid->WorldToGrid(DesiredPos);
				FIntVector CacheKey = FlowFieldMgr->QuantizeToCacheLevel(TargetCell);
				const FFlowFieldData* Found = AllCaches.Find(CacheKey);
				if (Found && Found->DirectionField.Num() > 0)
				{
					BestCache = Found;
					break;
				}
			}

			// 没找到AI使用的缓存，回退到离玩家最近的
			if (!BestCache)
			{
				float BestDistSq = FLT_MAX;
				for (const auto& CachePair : AllCaches)
				{
					if (CachePair.Value.DirectionField.Num() == 0)
						continue;
					FVector CacheTargetWorld = NavGrid->GetCellCenter(CachePair.Value.TargetCell);
					float DistSq = FVector::DistSquared2D(CacheTargetWorld, ViewCenter);
					if (DistSq < BestDistSq)
					{
						BestDistSq = DistSq;
						BestCache = &CachePair.Value;
					}
				}
			}

			if (!BestCache)
				return;

			for (const auto& DirPair : BestCache->DirectionField)
			{
				const FIntVector& Cell = DirPair.Key;
				const FVector& Dir = DirPair.Value;

				// 只绘制玩家视野范围内的cell
				if (FMath::Abs(Cell.X - ViewCenterCell.X) > DrawRange ||
					FMath::Abs(Cell.Y - ViewCenterCell.Y) > DrawRange)
					continue;

				// 跳过不可行走的cell（障碍物cell不画箭头）
				if (!NavGrid->IsWalkable(Cell))
					continue;

				if (Dir.SizeSquared() < KINDA_SMALL_NUMBER)
					continue;

				FVector CellCenter = NavGrid->GetCellCenter(Cell);
				CellCenter.Z = 5.0f;

				float ArrowLength = CellSize * 0.4f;
				FVector ArrowEnd = CellCenter + Dir.GetSafeNormal2D() * ArrowLength;

				DrawDebugDirectionalArrow(World, CellCenter, ArrowEnd, ArrowLength * 0.3f, FColor::Yellow, false, 0.02f, SDPG_World, 1.0f);
			}
		}
	}

	bool IsSteeringDisabled()
	{
		return CVarDisableSteering.GetValueOnGameThread() != 0;
	}

	bool IsCrowdDebugEnabled()
	{
		return CVarShowCrowd.GetValueOnGameThread() != 0;
	}

	bool IsPBDDebugEnabled()
	{
		return CVarShowPBD.GetValueOnGameThread() != 0;
	}

	bool IsSteeringTargetDebugEnabled()
	{
		return CVarShowSteeringTargets.GetValueOnGameThread() != 0;
	}
}
