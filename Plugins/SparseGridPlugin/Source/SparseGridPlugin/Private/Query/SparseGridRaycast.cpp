// Copyright Epic Games, Inc. All Rights Reserved.

#include "SparseGridRaycast.h"
#include "SparseGridObject.h"

void FSparseGridDDAState::Initialize(const FVector& RayOrigin, const FVector& RayDir, float CellSize)
{
	// Calculate initial cell coordinates
	CurrentCell = FIntVector(
		FMath::FloorToInt(RayOrigin.X / CellSize),
		FMath::FloorToInt(RayOrigin.Y / CellSize),
		FMath::FloorToInt(RayOrigin.Z / CellSize)
	);

	// Calculate step direction for each axis
	Step = FIntVector(
		RayDir.X >= 0 ? 1 : -1,
		RayDir.Y >= 0 ? 1 : -1,
		RayDir.Z >= 0 ? 1 : -1
	);

	// Calculate tMax (distance to next cell boundary)
	tMax = FVector(
		RayDir.X != 0.0f 
			? ((CurrentCell.X + (Step.X > 0 ? 1 : 0)) * CellSize - RayOrigin.X) / RayDir.X 
			: FLT_MAX,
		RayDir.Y != 0.0f 
			? ((CurrentCell.Y + (Step.Y > 0 ? 1 : 0)) * CellSize - RayOrigin.Y) / RayDir.Y 
			: FLT_MAX,
		RayDir.Z != 0.0f 
			? ((CurrentCell.Z + (Step.Z > 0 ? 1 : 0)) * CellSize - RayOrigin.Z) / RayDir.Z 
			: FLT_MAX
	);

	// Calculate tDelta (distance to move one cell)
	tDelta = FVector(
		RayDir.X != 0.0f ? CellSize / FMath::Abs(RayDir.X) : FLT_MAX,
		RayDir.Y != 0.0f ? CellSize / FMath::Abs(RayDir.Y) : FLT_MAX,
		RayDir.Z != 0.0f ? CellSize / FMath::Abs(RayDir.Z) : FLT_MAX
	);

	CurrentT = 0.0f;
}

int32 FSparseGridDDAState::StepToNextCell()
{
	// Find the axis with minimum tMax
	int32 MinAxis = 0;
	if (tMax.Y < tMax.X) MinAxis = 1;
	if (tMax.Z < tMax[MinAxis]) MinAxis = 2;

	// Step to next cell
	switch (MinAxis)
	{
	case 0:
		CurrentCell.X += Step.X;
		CurrentT = tMax.X;
		tMax.X += tDelta.X;
		break;
	case 1:
		CurrentCell.Y += Step.Y;
		CurrentT = tMax.Y;
		tMax.Y += tDelta.Y;
		break;
	case 2:
		CurrentCell.Z += Step.Z;
		CurrentT = tMax.Z;
		tMax.Z += tDelta.Z;
		break;
	}

	return MinAxis;
}
