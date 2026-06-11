#include "CombatDebug.h"
#include "DrawDebugHelpers.h"
#include "CombatComponent.h"
#include "CombatManager.h"
#include "CombatWeapon.h"
#include "CombatSkill.h"
#include "ICombatObject.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarDebug(
    TEXT("CombatSystem.Debug"),
    0,
    TEXT("Enable Combat debug visualization (0=Off, 1=On)"),
    ECVF_Default
);

namespace CombatDebug
{
    bool IsDebugEnabled()
    {
        return CVarDebug.GetValueOnGameThread() != 0;
    }

    EWeaponDebugState GetWeaponDebugState(UCombatComponent* Comp, UCombatManager* Manager)
    {
        if (!Comp || !Manager)
            return EWeaponDebugState::Idle;

        EAttackPhase Phase = Comp->GetAttackPhase();

        switch (Phase)
        {
        case EAttackPhase::Active:
            return EWeaponDebugState::Active;
        case EAttackPhase::Windup:
        case EAttackPhase::FollowThrough:
            return EWeaponDebugState::FollowThrough;
        case EAttackPhase::Idle:
        default:
            return EWeaponDebugState::Idle;
        }
    }

    static void DrawDebugCircle2D(UWorld* World, const FVector& Center, float Radius, const FColor& Color, bool bPersistent = false, float LifeTime = -1.0f, uint8 DepthPriority = 0, float Thickness = 0.0f)
    {
        const int32 Segments = 32;
        float AngleStep = 2.0f * PI / Segments;

        FVector PrevPoint = FVector(Center.X + Radius, Center.Y, Center.Z);
        for (int32 i = 1; i <= Segments; ++i)
        {
            float Angle = AngleStep * i;
            FVector NextPoint = FVector(Center.X + Radius * FMath::Cos(Angle), Center.Y + Radius * FMath::Sin(Angle), Center.Z);
            DrawDebugLine(World, PrevPoint, NextPoint, Color, bPersistent, LifeTime, DepthPriority, Thickness);
            PrevPoint = NextPoint;
        }
    }

    static void DrawDebugArc2D(UWorld* World, const FVector& Center, const FVector& Direction, float Length, float AngleDegrees, const FColor& Color, bool bPersistent = false, float LifeTime = -1.0f, uint8 DepthPriority = 0, float Thickness = 0.0f)
    {
        float HalfAngleRad = FMath::DegreesToRadians(AngleDegrees * 0.5f);
        int32 Segments = FMath::Max(8, FMath::RoundToInt(AngleDegrees / 5.0f));
        float AngleStep = FMath::DegreesToRadians(AngleDegrees) / Segments;

        FVector LeftDir = Direction.RotateAngleAxis(-AngleDegrees * 0.5f, FVector::UpVector);
        FVector RightDir = Direction.RotateAngleAxis(AngleDegrees * 0.5f, FVector::UpVector);

        FVector PrevPoint = Center + LeftDir * Length;
        DrawDebugLine(World, Center, PrevPoint, Color, bPersistent, LifeTime, DepthPriority, Thickness);

        for (int32 i = 1; i <= Segments; ++i)
        {
            float AngleRad = -HalfAngleRad + AngleStep * i;
            FVector NextPoint = Center + Direction.RotateAngleAxis(FMath::RadiansToDegrees(AngleRad), FVector::UpVector) * Length;
            DrawDebugLine(World, PrevPoint, NextPoint, Color, bPersistent, LifeTime, DepthPriority, Thickness);
            PrevPoint = NextPoint;
        }

        DrawDebugLine(World, Center, Center + RightDir * Length, Color, bPersistent, LifeTime, DepthPriority, Thickness);
    }

    static void DrawDebugRectangle2D(UWorld* World, const FVector& Center, const FVector& Direction, float Length, float Width, const FColor& Color, bool bPersistent = false, float LifeTime = -1.0f, uint8 DepthPriority = 0, float Thickness = 0.0f)
    {
        float HalfWidth = Width * 0.5f;
        FVector RightVec = Direction.RotateAngleAxis(90.0f, FVector::UpVector);

        FVector Left = Center + RightVec * HalfWidth;
        FVector Right = Center - RightVec * HalfWidth;
        FVector LeftEnd = Left + Direction * Length;
        FVector RightEnd = Right + Direction * Length;

        DrawDebugLine(World, Left, LeftEnd, Color, bPersistent, LifeTime, DepthPriority, Thickness);
        DrawDebugLine(World, Right, RightEnd, Color, bPersistent, LifeTime, DepthPriority, Thickness);
        DrawDebugLine(World, Left, Right, Color, bPersistent, LifeTime, DepthPriority, Thickness);
        DrawDebugLine(World, LeftEnd, RightEnd, Color, bPersistent, LifeTime, DepthPriority, Thickness);

        DrawDebugLine(World, Center, Center + Direction * Length, FColor::Green, bPersistent, LifeTime, DepthPriority, Thickness);
    }

    void DrawDebug(UWorld* World, UCombatComponent* Comp)
    {
        if (!IsDebugEnabled() || !World || !Comp)
            return;

        UCombatWeapon* Weapon = Comp->GetWeapon();
        if (!Weapon)
            return;

        const FCombatSkillConfig* Config = Weapon->GetSkillConfig();
        if (!Config)
            return;

        UCombatManager* Manager = Comp->GetManager();
        EWeaponDebugState State = GetWeaponDebugState(Comp, Manager);

        FColor Color;
        switch (State)
        {
        case EWeaponDebugState::Active:
            Color = FColor::Red;
            break;
        case EWeaponDebugState::FollowThrough:
            Color = FColor::Blue;
            break;
        default:
            Color = FColor::Yellow;
            break;
        }

        FVector Location = Comp->GetCombatWorldPosition();
        Location.Z = 50.0f;
        FVector Direction = Comp->GetCombatForwardVector();
        Direction.Z = 0.0f;
        Direction.Normalize();

        float Range = Config->Range;

        switch (Config->AttackShape)
        {
        case EAttackShape::Circle:
            {
                float Radius = Range;
                if (Config->MaxTargets > 1)
                {
                    Radius += Config->AreaRadius;
                }
                DrawDebugCircle2D(World, Location, Radius, Color, false, -1.0f, 0, 2.0f);
            }
            break;

        case EAttackShape::Cone:
            {
                DrawDebugArc2D(World, Location, Direction, Range, Config->ConeAngle, Color, false, -1.0f, 0, 2.0f);
                DrawDebugLine(World, Location, Location + Direction * Range, FColor::Green, false, -1.0f, 0, 2.0f);
            }
            break;

        case EAttackShape::Rectangle:
            {
                DrawDebugRectangle2D(World, Location, Direction, Range, Config->RectangleWidth, Color, false, -1.0f, 0, 2.0f);
            }
            break;

        default:
            DrawDebugCircle2D(World, Location, Range, Color, false, -1.0f, 0, 2.0f);
            break;
        }
    }
}
