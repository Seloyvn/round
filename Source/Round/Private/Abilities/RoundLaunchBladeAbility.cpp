#include "Abilities/RoundLaunchBladeAbility.h"

#include "Actors/RoundBladeActor.h"
#include "Actors/RoundCharacterBladecaller.h"
#include "Actors/RoundGridTile.h"
#include "Game/RoundGameState.h"
#include "Components/RoundAttributeSetComponent.h"

namespace
{
struct FBladeCubeCoord
{
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;
};

FBladeCubeCoord BladeAxialToCube(const FVector2D& Axial)
{
    return { Axial.X, Axial.Y, -Axial.X - Axial.Y };
}

FBladeCubeCoord BladeCubeLerp(const FBladeCubeCoord& A, const FBladeCubeCoord& B, const double Alpha)
{
    return {
        FMath::Lerp(A.X, B.X, Alpha),
        FMath::Lerp(A.Y, B.Y, Alpha),
        FMath::Lerp(A.Z, B.Z, Alpha)
    };
}

FBladeCubeCoord BladeCubeRound(const FBladeCubeCoord& Cube)
{
    int32 RoundedX = FMath::RoundToInt(Cube.X);
    int32 RoundedY = FMath::RoundToInt(Cube.Y);
    int32 RoundedZ = FMath::RoundToInt(Cube.Z);

    const double XDiff = FMath::Abs(static_cast<double>(RoundedX) - Cube.X);
    const double YDiff = FMath::Abs(static_cast<double>(RoundedY) - Cube.Y);
    const double ZDiff = FMath::Abs(static_cast<double>(RoundedZ) - Cube.Z);

    if (XDiff > YDiff && XDiff > ZDiff)
    {
        RoundedX = -RoundedY - RoundedZ;
    }
    else if (YDiff > ZDiff)
    {
        RoundedY = -RoundedX - RoundedZ;
    }
    else
    {
        RoundedZ = -RoundedX - RoundedY;
    }

    return {
        static_cast<double>(RoundedX),
        static_cast<double>(RoundedY),
        static_cast<double>(RoundedZ)
    };
}

FVector2D BladeCubeToAxial(const FBladeCubeCoord& Cube)
{
    return FVector2D(static_cast<float>(Cube.X), static_cast<float>(Cube.Y));
}

TArray<ARoundGridTile*> BuildLaunchBladePath(const ARoundCharacterBladecaller* Bladecaller, const ARoundGridTile* TargetTile, const int32 MaxRange)
{
    TArray<ARoundGridTile*> Result;
    if (!Bladecaller || !Bladecaller->CurrentTile || !TargetTile)
    {
        return Result;
    }

    const ARoundGameState* GameState = Bladecaller->GetRoundGameState();
    if (!GameState)
    {
        return Result;
    }

    const int32 SelectedDistance = Bladecaller->CurrentTile->GetDistance(TargetTile);
    if (SelectedDistance <= 0)
    {
        return Result;
    }

    const FBladeCubeCoord StartCube = BladeAxialToCube(Bladecaller->CurrentTile->GridPosition);
    const FBladeCubeCoord TargetCube = BladeAxialToCube(TargetTile->GridPosition);
    const double RangeScale = static_cast<double>(FMath::Max(1, MaxRange)) / static_cast<double>(SelectedDistance);
    const FBladeCubeCoord ExtendedTargetCube = {
        StartCube.X + ((TargetCube.X - StartCube.X) * RangeScale),
        StartCube.Y + ((TargetCube.Y - StartCube.Y) * RangeScale),
        StartCube.Z + ((TargetCube.Z - StartCube.Z) * RangeScale)
    };

    for (int32 StepIndex = 0; StepIndex < MaxRange; ++StepIndex)
    {
        const double Alpha = static_cast<double>(StepIndex + 1) / static_cast<double>(MaxRange);
        const FVector2D StepPosition = BladeCubeToAxial(BladeCubeRound(BladeCubeLerp(StartCube, ExtendedTargetCube, Alpha)));
        ARoundGridTile* StepTile = GameState->GetField(StepPosition);
        if (!StepTile)
        {
            break;
        }

        if (StepTile->OccupyingFieldObject)
        {
            break;
        }

        Result.AddUnique(StepTile);
    }

    return Result;
}
}

bool ARoundLaunchBladeAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    const ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    return Bladecaller
        && Bladecaller->CurrentTile
        && TargetTile
        && TargetTile != Bladecaller->CurrentTile
        && IsTargetInRange(TargetTile)
        && GetAbilityVisibility() == ERoundButtonState::Enabled
        && !Bladecaller->CurrentTile->OccupyingFieldObject;
}

void ARoundLaunchBladeAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    const ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    if (!Bladecaller || !TargetTile)
    {
        return;
    }

    OutTiles.Append(BuildLaunchBladePath(Bladecaller, TargetTile, GetAbilityRange()));
}

FRoundActionResult ARoundLaunchBladeAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    if (!Bladecaller || !Bladecaller->CurrentTile || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    const TArray<ARoundGridTile*> LaunchPath = BuildLaunchBladePath(Bladecaller, TargetTile, GetAbilityRange());
    if (LaunchPath.Num() <= 0)
    {
        Result.Reason = TEXT("No valid launch path.");
        return Result;
    }

    ARoundBladeActor* Blade = Bladecaller->SummonBladeAtTile(Bladecaller->CurrentTile);
    if (!Blade)
    {
        Result.Reason = TEXT("Failed to summon blade.");
        return Result;
    }

    const FRoundFieldObjectMoveResult MoveResult = Blade->MoveAlongPath(LaunchPath);
    const int32 BonusLifetime = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BladeLifetime"))));
    if (BonusLifetime > 0) Blade->SetLifetimeInitiative(Bladecaller->GetBladeLifetimeInitiative() + BonusLifetime);
    const int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    int32 HitCount = 0;
    for (ARoundCharacter* HitCharacter : MoveResult.HitCharacters)
    {
        if (!HitCharacter || !Bladecaller->IsEnemyFor(HitCharacter))
        {
            continue;
        }

        bool bDidDie = false;
        const double PerTile = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("DamageMultiplierAddPerTraversedTile")));
        const int32 Damage = FMath::RoundToInt(GetAbilityDamage() * (1.0 + PerTile * MoveResult.TraversedTiles.Num()));
        ApplyAbilityDamageToTarget(HitCharacter, Damage, DefenseIgnore, bDidDie);
        ++HitCount;
    }
    const int32 InitiativePerHit = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativePerHit"))));
    if (Bladecaller->Attributes && InitiativePerHit > 0) Bladecaller->Attributes->GainInitiative(FMath::Min(25, InitiativePerHit * HitCount));
    if (HitCount >= 2) ReduceCooldown(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionOnMultiHit")))));

    if (ARoundGameState* GameState = Bladecaller->GetRoundGameState())
    {
        GameState->OnBoardStateChanged.Broadcast();
    }
    Result.bSuccess = MoveResult.bReachedDestination || MoveResult.TraversedTiles.Num() > 0;
    Result.Reason = FString::Printf(TEXT("Blade launched through %d target(s)."), MoveResult.HitCharacters.Num());
    return Result;
}
