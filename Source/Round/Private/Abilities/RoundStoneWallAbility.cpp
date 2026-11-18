#include "Abilities/RoundStoneWallAbility.h"

#include "Actors/RoundGridTile.h"
#include "Actors/RoundCharacterGeomancer.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Game/RoundGameState.h"

namespace
{
struct FGeomancerCubeCoord
{
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;
};

FGeomancerCubeCoord AxialToCube(const FVector2D& Axial)
{
    return { Axial.X, Axial.Y, -Axial.X - Axial.Y };
}

FGeomancerCubeCoord CubeLerp(const FGeomancerCubeCoord& A, const FGeomancerCubeCoord& B, const double Alpha)
{
    return { FMath::Lerp(A.X, B.X, Alpha), FMath::Lerp(A.Y, B.Y, Alpha), FMath::Lerp(A.Z, B.Z, Alpha) };
}

FGeomancerCubeCoord CubeRound(const FGeomancerCubeCoord& Cube)
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

FVector2D CubeToAxial(const FGeomancerCubeCoord& Cube)
{
    return FVector2D(static_cast<float>(Cube.X), static_cast<float>(Cube.Y));
}

TArray<ARoundGridTile*> BuildLineTiles(const ARoundCharacter* Character, const ARoundGridTile* TargetTile, const int32 Length)
{
    TArray<ARoundGridTile*> Result;
    if (!Character || !Character->CurrentTile || !TargetTile || Length <= 0)
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

    const FGeomancerCubeCoord StartCube = AxialToCube(Character->CurrentTile->GridPosition);
    const FGeomancerCubeCoord TargetCube = AxialToCube(TargetTile->GridPosition);
    const double RangeScale = static_cast<double>(Length) / static_cast<double>(SelectedDistance);
    const FGeomancerCubeCoord ExtendedTargetCube = {
        StartCube.X + ((TargetCube.X - StartCube.X) * RangeScale),
        StartCube.Y + ((TargetCube.Y - StartCube.Y) * RangeScale),
        StartCube.Z + ((TargetCube.Z - StartCube.Z) * RangeScale)
    };

    for (int32 StepIndex = 0; StepIndex < Length; ++StepIndex)
    {
        const double Alpha = static_cast<double>(StepIndex + 1) / static_cast<double>(Length);
        if (ARoundGridTile* StepTile = GameState->GetField(CubeToAxial(CubeRound(CubeLerp(StartCube, ExtendedTargetCube, Alpha)))))
        {
            Result.AddUnique(StepTile);
        }
    }

    return Result;
}
}

bool ARoundStoneWallAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    return OwningCharacter && OwningCharacter->CurrentTile && TargetTile && TargetTile != OwningCharacter->CurrentTile && IsTargetInRange(TargetTile);
}

void ARoundStoneWallAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    OutTiles.Append(BuildLineTiles(OwningCharacter, TargetTile, FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("WallLength"))))));
}

FRoundActionResult ARoundStoneWallAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    const TArray<ARoundGridTile*> WallTiles = BuildLineTiles(OwningCharacter, TargetTile, FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("WallLength")))));
    if (WallTiles.Num() <= 0)
    {
        Result.Reason = TEXT("No valid wall path.");
        return Result;
    }

    ARoundCharacterGeomancer* Geomancer = Cast<ARoundCharacterGeomancer>(OwningCharacter);
    ARoundGameState* GameState = OwningCharacter ? OwningCharacter->GetRoundGameState() : nullptr;
    int32 CreatedRocks = 0;
    TArray<ARoundCharacter*> DamagedTargets;
    for (ARoundGridTile* Tile : WallTiles)
    {
        if (!Tile || Tile->OccupyingCharacter || Tile->GetResolvedOccupyingFieldObject())
        {
            continue;
        }

        Tile->SetTerrainType(ERoundTerrainType::Rocks);
        ++CreatedRocks;

        if (GameState)
        {
            for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(Tile))
            {
                ARoundCharacter* Enemy = Neighbor ? Neighbor->OccupyingCharacter : nullptr;
                if (!Enemy || !Geomancer || !Geomancer->IsEnemyFor(Enemy) || DamagedTargets.Contains(Enemy)) continue;
                const int32 Damage = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AdjacentDamage"))));
                if (Damage > 0)
                {
                    bool bDidDie = false;
                    if (ApplyAbilityDamageToTarget(Enemy, Damage, 0, bDidDie) > 0) DamagedTargets.Add(Enemy);
                }
            }
        }
    }

    if (Geomancer && CreatedRocks > 0)
    {
        const int32 HealPerRock = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("HealPerRockCreated"))));
        if (HealPerRock > 0) Geomancer->Heal(HealPerRock * CreatedRocks);
        const int32 InitiativePerRock = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativePerRockCreated"))));
        if (Geomancer->Attributes && InitiativePerRock > 0) Geomancer->Attributes->GainInitiative(FMath::Min(30, InitiativePerRock * CreatedRocks));
    }

    Result.bSuccess = CreatedRocks > 0;
    Result.Reason = FString::Printf(TEXT("Stone Wall created %d rock(s)."), CreatedRocks);
    return Result;
}
