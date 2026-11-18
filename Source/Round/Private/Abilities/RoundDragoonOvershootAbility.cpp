#include "Abilities/RoundDragoonOvershootAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacterDragoon.h"
#include "Actors/RoundGridTile.h"
#include "Game/RoundGameState.h"
#include "Components/RoundAttributeSetComponent.h"

namespace
{
struct FOvershootCubeCoord
{
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;
};

FOvershootCubeCoord OvershootAxialToCube(const FVector2D& Axial)
{
    return { Axial.X, Axial.Y, -Axial.X - Axial.Y };
}

FOvershootCubeCoord OvershootCubeLerp(const FOvershootCubeCoord& A, const FOvershootCubeCoord& B, const double Alpha)
{
    return { FMath::Lerp(A.X, B.X, Alpha), FMath::Lerp(A.Y, B.Y, Alpha), FMath::Lerp(A.Z, B.Z, Alpha) };
}

FOvershootCubeCoord OvershootCubeRound(const FOvershootCubeCoord& Cube)
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

    return { static_cast<double>(RoundedX), static_cast<double>(RoundedY), static_cast<double>(RoundedZ) };
}

FVector2D OvershootCubeToAxial(const FOvershootCubeCoord& Cube)
{
    return FVector2D(static_cast<float>(Cube.X), static_cast<float>(Cube.Y));
}

TArray<ARoundGridTile*> BuildOvershootLine(const ARoundCharacter* Character, const ARoundGridTile* TargetTile, const int32 BehindDistance)
{
    TArray<ARoundGridTile*> Result;
    if (!Character || !Character->CurrentTile || !TargetTile)
    {
        return Result;
    }

    const ARoundGameState* GameState = Character->GetRoundGameState();
    if (!GameState)
    {
        return Result;
    }

    const int32 SelectedDistance = Character->CurrentTile->GetDistance(TargetTile);
    if (SelectedDistance <= 0)
    {
        return Result;
    }

    const int32 TotalDistance = SelectedDistance + FMath::Max(0, BehindDistance);
    const FOvershootCubeCoord StartCube = OvershootAxialToCube(Character->CurrentTile->GridPosition);
    const FOvershootCubeCoord TargetCube = OvershootAxialToCube(TargetTile->GridPosition);
    const double RangeScale = static_cast<double>(FMath::Max(1, TotalDistance)) / static_cast<double>(SelectedDistance);
    const FOvershootCubeCoord ExtendedTargetCube = {
        StartCube.X + ((TargetCube.X - StartCube.X) * RangeScale),
        StartCube.Y + ((TargetCube.Y - StartCube.Y) * RangeScale),
        StartCube.Z + ((TargetCube.Z - StartCube.Z) * RangeScale)
    };

    for (int32 StepIndex = 0; StepIndex < TotalDistance; ++StepIndex)
    {
        const double Alpha = static_cast<double>(StepIndex + 1) / static_cast<double>(TotalDistance);
        ARoundGridTile* StepTile = GameState->GetField(OvershootCubeToAxial(OvershootCubeRound(OvershootCubeLerp(StartCube, ExtendedTargetCube, Alpha))));
        if (!StepTile)
        {
            break;
        }

        Result.AddUnique(StepTile);
    }

    return Result;
}
}

bool ARoundDragoonOvershootAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    return OwningCharacter
        && OwningCharacter->CurrentTile
        && TargetTile
        && TargetTile != OwningCharacter->CurrentTile
        && IsTargetInRange(TargetTile)
        && GetAbilityVisibility() == ERoundButtonState::Enabled;
}

void ARoundDragoonOvershootAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    OutTiles.Append(BuildOvershootLine(OwningCharacter, TargetTile, FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BehindDistance"))))));
}

FRoundActionResult ARoundDragoonOvershootAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterDragoon* Dragoon = Cast<ARoundCharacterDragoon>(OwningCharacter);
    if (!Dragoon || !Dragoon->CurrentTile || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    const int32 BehindDistance = FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BehindDistance"))));
    const TArray<ARoundGridTile*> LineTiles = BuildOvershootLine(Dragoon, TargetTile, BehindDistance);
    if (LineTiles.Num() <= 0)
    {
        Result.Reason = TEXT("No valid overshoot path.");
        return Result;
    }

    ARoundGridTile* DestinationTile = nullptr;
    const int32 TargetIndex = LineTiles.IndexOfByKey(TargetTile);
    for (int32 Index = LineTiles.Num() - 1; Index > TargetIndex; --Index)
    {
        ARoundGridTile* Candidate = LineTiles[Index];
        if (Candidate && Candidate->IsMoveable() && !Candidate->HasBlockingFieldObject() && !Candidate->OccupyingCharacter)
        {
            DestinationTile = Candidate;
            break;
        }
    }

    if (!DestinationTile)
    {
        Result.Reason = TEXT("No free tile behind the target.");
        return Result;
    }

    Dragoon->SetField(DestinationTile, true);

    bool bAnyDestroyed = false;
    const int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    const double DistanceMultiplier = 1.0 + FMath::Max(0.0, GetAbilityValueFromKey(TEXT("DamageMultiplierAddPerBehindTile"))) * BehindDistance;
    const int32 AppliedDamage = ApplyAbilityDamageToTile(TargetTile, FMath::RoundToInt(GetAbilityDamage() * DistanceMultiplier), DefenseIgnore, bAnyDestroyed, true);
    if (ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile))
    {
        Dragoon->RecordEnemyHit(TargetCharacter, AppliedDamage, GetAbilityDamageType());
        if (TargetCharacter->Attributes) TargetCharacter->Attributes->PassInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnHit")))));
    }
    if (Dragoon->Attributes) Dragoon->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainOnLanding")))));
    Dragoon->GainMomentumStacks(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BonusMomentumOnHit")))));

    Result.bSuccess = true;
    Result.Reason = TEXT("Overshoot executed.");
    return Result;
}
