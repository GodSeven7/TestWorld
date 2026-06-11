// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace FSparseGridCollisionUtils
{
    static bool RaycastSphere(
        const FVector& SphereCenter,
        float SphereRadius,
        const FVector& RayOrigin,
        const FVector& RayDirection,
        FVector& OutHitPoint,
        float& OutHitDistance)
    {
        FVector L = SphereCenter - RayOrigin;
        float Tca = FVector::DotProduct(L, RayDirection);
        
        if (Tca < 0.0f)
            return false;
        
        float d2 = L.SizeSquared() - Tca * Tca;
        float r2 = SphereRadius * SphereRadius;
        
        if (d2 > r2)
            return false;
        
        float Thc = FMath::Sqrt(r2 - d2);
        float t = Tca - Thc;
        
        if (t < 0.0f)
            t = Tca + Thc;
        
        OutHitDistance = t;
        OutHitPoint = RayOrigin + RayDirection * t;
        return true;
    }

}
