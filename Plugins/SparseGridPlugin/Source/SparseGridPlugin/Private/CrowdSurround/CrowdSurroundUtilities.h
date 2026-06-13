#pragma once

#include "DrawDebugHelpers.h"

namespace CrowdSurroundUtilities
{
    inline FVector FlattenPosition(const FVector& Position)
    {
        return FVector(Position.X, Position.Y, 0.0f);
    }

    inline void DrawCircle(UWorld* World, const FVector& Center, float Radius, const FColor& Color, float LifeTime, float Thickness)
    {
        if (!World || Radius <= 0.0f)
        {
            return;
        }

        constexpr int32 Segments = 64;
        FVector Previous = Center + FVector(Radius, 0.0f, 0.0f);
        Previous.Z += 8.0f;

        for (int32 Index = 1; Index <= Segments; ++Index)
        {
            const float Angle = 2.0f * PI * static_cast<float>(Index) / static_cast<float>(Segments);
            FVector Current = Center + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
            Current.Z += 8.0f;
            DrawDebugLine(World, Previous, Current, Color, false, LifeTime, SDPG_World, Thickness);
            Previous = Current;
        }
    }
}
