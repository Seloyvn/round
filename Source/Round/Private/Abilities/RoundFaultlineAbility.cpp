#include "Abilities/RoundFaultlineAbility.h"

#include "Actors/RoundCharacterGeomancer.h"
#include "Actors/RoundGridTile.h"
#include "Data/RoundTerrainHelpers.h"
#include "Game/RoundGameState.h"
#include "Components/RoundAttributeSetComponent.h"

namespace
{
struct FFaultCubeCoord
{
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;
};

FFaultCubeCoord FaultAxialToCube(const FVector2D& Axial)
{
    return { Axial.X, Axial.Y, -Axial.X - Axial.Y };
}

FFaultCubeCoord FaultCubeLerp(const FFaultCubeCoord& A, const FFaultCubeCoord& B, const double Alpha)
{
    return { FMath::Lerp(A.X, B.X, Alpha), FMath::Lerp(A.Y, B.Y, Alpha), FMath::Lerp(A.Z, B.Z, Alpha) };
}

FFaultCubeCoord FaultCubeRound(const FFaultCubeCoord& Cube)
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

FVector2D FaultCubeToAxial(const FFaultCubeCoord& Cube)
{
    return FVector2D(static_cast<float>(Cube.X), static_cast<float>(Cube.Y));
}

TArray<ARoundGridTile*> BuildFaultlineTiles(const ARoundCharacter* Character, const ARoundGridTile* TargetTile, const int32 RangeBudget)
{
    TArray<ARoundGridTile*> Result;
    if (!Character || !Character->CurrentTile || !TargetTile || RangeBudget <= 0)
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

    const int32 MaxSteps = RangeBudget + 8;
    const FFaultCubeCoord StartCube = FaultAxialToCube(Character->CurrentTile->GridPosition);
    const FFaultCubeCoord TargetCube = FaultAxialToCube(TargetTile->GridPosition);
    const double RangeScale = static_cast<double>(MaxSteps) / static_cast<double>(SelectedDistance);
    const FFaultCubeCoord ExtendedTargetCube = {
        StartCube.X + ((TargetCube.X - StartCube.X) * RangeScale),
        StartCube.Y + ((TargetCube.Y - StartCube.Y) * RangeScale),
        StartCube.Z + ((TargetCube.Z - StartCube.Z) * RangeScale)
    };

    int32 ConsumedRange = 0;
    for (int32 StepIndex = 0; StepIndex < MaxSteps; ++StepIndex)
    {
        const double Alpha = static_cast<double>(StepIndex + 1) / static_cast<double>(MaxSteps);
        ARoundGridTile* StepTile = GameState->GetField(FaultCubeToAxial(FaultCubeRound(FaultCubeLerp(StartCube, ExtendedTargetCube, Alpha))));
        if (!StepTile)
        {
            break;
        }

        Result.AddUnique(StepTile);
        if (!RoundTerrain::IsRockLikeTerrain(StepTile->TerrainType))
        {
            ++ConsumedRange;
            if (ConsumedRange >= RangeBudget)
            {
                break;
            }
        }
    }

    return Result;
}
}

bool ARoundFaultlineAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    return OwningCharacter && OwningCharacter->CurrentTile && TargetTile && TargetTile != OwningCharacter->CurrentTile && IsTargetInRange(TargetTile);
}

void ARoundFaultlineAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    OutTiles.Append(BuildFaultlineTiles(OwningCharacter, TargetTile, GetAbilityRange()));
}

FRoundActionResult ARoundFaultlineAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterGeomancer* Geomancer = Cast<ARoundCharacterGeomancer>(OwningCharacter);
    if (!Geomancer || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    const TArray<ARoundGridTile*> FaultTiles = BuildFaultlineTiles(Geomancer, TargetTile, GetAbilityRange());
    bool bTriggeredRockSection = false;
    int32 CrossedRocks = 0;
    int32 HitCount = 0;

    for (ARoundGridTile* Tile : FaultTiles)
    {
        if (!Tile)
        {
            continue;
        }

        const bool bRockTile = RoundTerrain::IsRockLikeTerrain(Tile->TerrainType);
        bTriggeredRockSection = bTriggeredRockSection || bRockTile;
        if (bRockTile) ++CrossedRocks;
        ARoundCharacter* TargetCharacter = Tile->OccupyingCharacter;
        if (!TargetCharacter || !Geomancer->IsEnemyFor(TargetCharacter))
        {
            continue;
        }

        int32 Damage = GetAbilityDamage();
        if (bTriggeredRockSection)
        {
            Damage += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RockBonusDamage"))));
        }

        bool bDidDie = false;
        const int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
        const int32 AppliedDamage = ApplyAbilityDamageToTarget(TargetCharacter, Damage, DefenseIgnore, bDidDie);
        if (AppliedDamage > 0)
        {
            if (!bDidDie && TargetCharacter->Attributes)
            {
                TargetCharacter->Attributes->PassInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnHit")))));
            }
            if (!bDidDie)
            {
                Geomancer->ApplyGeomancerMark(TargetCharacter);
            }
            ++HitCount;
        }
    }

    const int32 ReductionPerRock = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionPerRock"))));
    if (ReductionPerRock > 0) ReduceCooldown(FMath::Min(120, CrossedRocks * ReductionPerRock));

    Result.bSuccess = FaultTiles.Num() > 0;
    Result.Reason = FString::Printf(TEXT("Faultline hit %d target(s)."), HitCount);
    return Result;
}
