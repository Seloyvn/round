#include "Actors/RoundBladeActor.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Game/RoundGameState.h"

FRoundFieldObjectMoveResult ARoundBladeActor::MoveToField(ARoundGridTile* TargetTile)
{
    FRoundFieldObjectMoveResult Result;
    if (!CurrentTile || !TargetTile || TargetTile == CurrentTile)
    {
        return Result;
    }

    const ARoundGameState* RoundGameState = OwningCharacter ? OwningCharacter->GetRoundGameState() : nullptr;
    if (!RoundGameState)
    {
        return Result;
    }

    TArray<ARoundGridTile*> Path = RoundGameState->GetFieldsBetween(CurrentTile, TargetTile, true);
    if (Path.Num() > 0 && Path[0] == CurrentTile)
    {
        Path.RemoveAt(0);
    }

    return MoveAlongPath(Path);
}

FRoundFieldObjectMoveResult ARoundBladeActor::MoveAlongPath(const TArray<ARoundGridTile*>& Path)
{
    FRoundFieldObjectMoveResult Result;
    if (!CurrentTile || Path.Num() == 0)
    {
        return Result;
    }

    ARoundGridTile* FinalTile = CurrentTile;
    for (ARoundGridTile* StepTile : Path)
    {
        if (!StepTile)
        {
            break;
        }

        if (StepTile->OccupyingFieldObject && StepTile->OccupyingFieldObject != this)
        {
            break;
        }

        if (StepTile != CurrentTile && StepTile->OccupyingCharacter)
        {
            ARoundCharacter* HitCharacter = StepTile->OccupyingCharacter;
            if (!OwningCharacter || OwningCharacter->IsEnemyFor(HitCharacter))
            {
                Result.HitCharacters.AddUnique(HitCharacter);
            }
        }

        Result.TraversedTiles.AddUnique(StepTile);
        FinalTile = StepTile;
    }

    Result.bReachedDestination = FinalTile == Path.Last() && FinalTile != CurrentTile;
    if (FinalTile != CurrentTile)
    {
        SetField(FinalTile);
    }

    return Result;
}
