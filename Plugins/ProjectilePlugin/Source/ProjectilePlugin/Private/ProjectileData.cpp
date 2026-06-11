#include "ProjectileData.h"

FParabolicTrajectoryResult FProjectileTrajectoryUtils::CalculateParabolicTrajectory(
    const FVector& StartPos,
    const FVector& TargetPos,
    float Height,
    float Speed,
    float SimFrequency
)
{
    FParabolicTrajectoryResult Result;

    FVector StartToEnd = TargetPos - StartPos;
    float HorizontalDist = StartToEnd.Size2D();

    if (HorizontalDist < UE_KINDA_SMALL_NUMBER && FMath::Abs(StartToEnd.Z) < UE_KINDA_SMALL_NUMBER)
    {
        return Result;
    }

    FVector HorizontalDir = StartToEnd.GetSafeNormal2D();

    float PeakZ = FMath::Max(StartPos.Z, TargetPos.Z) + Height;

    float TotalTime = (Speed > UE_KINDA_SMALL_NUMBER) ? (HorizontalDist / Speed) : 1.0f;
    if (TotalTime < UE_KINDA_SMALL_NUMBER)
    {
        TotalTime = 1.0f;
    }

    float a = 0.0f;
    float b = 0.0f;
    float c = StartPos.Z;

    if (HorizontalDist > UE_KINDA_SMALL_NUMBER)
    {
        a = -4.0f * (PeakZ - (StartPos.Z + TargetPos.Z) * 0.5f) / (HorizontalDist * HorizontalDist);
        b = (TargetPos.Z - StartPos.Z - a * HorizontalDist * HorizontalDist) / HorizontalDist;
    }

    int32 NumSteps = FMath::CeilToInt(TotalTime * SimFrequency);
    NumSteps = FMath::Max(NumSteps, 2);

    float StepDeltaTime = TotalTime / NumSteps;

    for (int32 i = 0; i <= NumSteps; i++)
    {
        float t = i * StepDeltaTime;
        float s = Speed * t;

        FVector Point = StartPos + HorizontalDir * s;
        Point.Z = a * s * s + b * s + c;

        Result.PathPoints.Add(Point);
        Result.PathTimes.Add(t);
    }

    Result.TotalTime = TotalTime;
    Result.bSuccess = true;

    return Result;
}
