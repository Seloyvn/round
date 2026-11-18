#include "Abilities/RoundDriveBladesAbility.h"

#include "Actors/RoundBladeActor.h"
#include "Actors/RoundCharacterBladecaller.h"
#include "Actors/RoundGridTile.h"
#include "Game/RoundGameState.h"
#include "Components/RoundAttributeSetComponent.h"

namespace
{
struct FDriveBladeCubeCoord
{
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;
};

FDriveBladeCubeCoord DriveBladeAxialToCube(const FVector2D& Axial)
{
    return { Axial.X, Axial.Y, -Axial.X - Axial.Y };
}

FDriveBladeCubeCoord DriveBladeCubeLerp(const FDriveBladeCubeCoord& A, const FDriveBladeCubeCoord& B, const double Alpha)
{
    return {
        FMath::Lerp(A.X, B.X, Alpha),
        FMath::Lerp(A.Y, B.Y, Alpha),
        FMath::Lerp(A.Z, B.Z, Alpha)
    };
}

FDriveBladeCubeCoord DriveBladeCubeRound(const FDriveBladeCubeCoord& Cube)
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

FVector2D DriveBladeCubeToAxial(const FDriveBladeCubeCoord& Cube)
{
    return FVector2D(static_cast<float>(Cube.X), static_cast<float>(Cube.Y));
}

TArray<ARoundGridTile*> BuildDriveBladePath(const ARoundBladeActor* Blade, const ARoundGridTile* ThroughTile, const int32 BehindDistance)
{
    TArray<ARoundGridTile*> Result;
    if (!Blade || !Blade->CurrentTile || !ThroughTile)
    {
        return Result;
    }

    const ARoundCharacter* OwningCharacter = Blade->OwningCharacter;
    const ARoundGameState* GameState = OwningCharacter ? OwningCharacter->GetRoundGameState() : nullptr;
    if (!GameState)
    {
        return Result;
    }

    const int32 SelectedDistance = Blade->CurrentTile->GetDistance(ThroughTile);
    if (SelectedDistance <= 0)
    {
        return Result;
    }

    const int32 EffectiveBehindDistance = FMath::Max(1, BehindDistance);
    const FDriveBladeCubeCoord StartCube = DriveBladeAxialToCube(Blade->CurrentTile->GridPosition);
    const FDriveBladeCubeCoord ThroughCube = DriveBladeAxialToCube(ThroughTile->GridPosition);
    const double ExtendedScale = static_cast<double>(SelectedDistance + EffectiveBehindDistance) / static_cast<double>(SelectedDistance);
    const FDriveBladeCubeCoord ExtendedCube = {
        StartCube.X + ((ThroughCube.X - StartCube.X) * ExtendedScale),
        StartCube.Y + ((ThroughCube.Y - StartCube.Y) * ExtendedScale),
        StartCube.Z + ((ThroughCube.Z - StartCube.Z) * ExtendedScale)
    };

    for (int32 StepIndex = 0; StepIndex < SelectedDistance + EffectiveBehindDistance; ++StepIndex)
    {
        const double Alpha = static_cast<double>(StepIndex + 1) / static_cast<double>(SelectedDistance + EffectiveBehindDistance);
        const FVector2D StepPosition = DriveBladeCubeToAxial(DriveBladeCubeRound(DriveBladeCubeLerp(StartCube, ExtendedCube, Alpha)));
        ARoundGridTile* StepTile = GameState->GetField(StepPosition);
        if (!StepTile)
        {
            break;
        }

        if (StepTile->OccupyingFieldObject && StepTile->OccupyingFieldObject != Blade)
        {
            break;
        }

        Result.AddUnique(StepTile);
    }

    return Result;
}
}

bool ARoundDriveBladesAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    const ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    return Bladecaller
        && TargetTile
        && TargetTile != Bladecaller->CurrentTile
        && IsTargetInRange(TargetTile)
        && GetAbilityVisibility() == ERoundButtonState::Enabled
        && Bladecaller->GetActiveBladeCount() > 0;
}

void ARoundDriveBladesAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    const ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    if (!Bladecaller || !TargetTile)
    {
        return;
    }

    const int32 BehindDistance = FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BehindDistance"))));
    for (ARoundBladeActor* Blade : Bladecaller->GetActiveBlades())
    {
        OutTiles.Append(BuildDriveBladePath(Blade, TargetTile, BehindDistance));
    }
}

FRoundActionResult ARoundDriveBladesAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    if (!Bladecaller || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    int32 TotalHits = 0;
    int32 MovedBlades = 0;
    const int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    const int32 BehindDistance = FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BehindDistance"))));
    const TArray<TObjectPtr<ARoundBladeActor>> Blades = Bladecaller->GetActiveBlades();
    for (ARoundBladeActor* Blade : Blades)
    {
        if (!Blade)
        {
            continue;
        }

        const TArray<ARoundGridTile*> Path = BuildDriveBladePath(Blade, TargetTile, BehindDistance);
        if (Path.Num() <= 0)
        {
            continue;
        }

        const FRoundFieldObjectMoveResult MoveResult = Blade->MoveAlongPath(Path);
        if (MoveResult.TraversedTiles.Num() > 0)
        {
            ++MovedBlades;
        }

        for (ARoundCharacter* HitCharacter : MoveResult.HitCharacters)
        {
            if (!HitCharacter || !Bladecaller->IsEnemyFor(HitCharacter))
            {
                continue;
            }

            bool bDidDie = false;
            const int32 Damage = GetAbilityDamage() + FMath::Max(0, Blades.Num() - 1) * FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BonusDamagePerAdditionalBlade"))));
            ApplyAbilityDamageToTarget(HitCharacter, Damage, DefenseIgnore, bDidDie);
            ++TotalHits;
        }
        if (GetAbilityValueFromKey(TEXT("RefreshMovedBladeLifetime")) > 0.0 && MoveResult.TraversedTiles.Num() > 0)
        {
            Blade->SetLifetimeInitiative(Bladecaller->GetBladeLifetimeInitiative());
        }
    }

    if (Bladecaller->Attributes)
    {
        const int32 Gain = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativePerHit"))));
        if (Gain > 0) Bladecaller->Attributes->GainInitiative(FMath::Min(35, Gain * TotalHits));
    }
    const int32 PerBladeReduction = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionPerMovedBlade"))));
    if (PerBladeReduction > 0) ReduceCooldown(FMath::Min(180, PerBladeReduction * MovedBlades));

    if (ARoundGameState* GameState = Bladecaller->GetRoundGameState())
    {
        GameState->OnBoardStateChanged.Broadcast();
    }

    Result.bSuccess = MovedBlades > 0;
    Result.Reason = FString::Printf(TEXT("Driven %d blade(s) through %d hit(s)."), MovedBlades, TotalHits);
    return Result;
}
