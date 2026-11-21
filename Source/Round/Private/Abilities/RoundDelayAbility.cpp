#include "Abilities/RoundDelayAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Game/RoundGameState.h"

FRoundActionResult ARoundDelayAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 7)
    {
        return ARoundAbilityActor::ExecuteAbility(TargetTile);
    }
    FRoundActionResult Result = ExecuteDefaultAbility(TargetTile);
    if (!Result.bSuccess || !OwningCharacter || !TargetTile)
    {
        return Result;
    }

    ARoundGameState* RoundGameState = OwningCharacter->GetRoundGameState();
    const int32 Radius = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AoEInitiativeLossRadius"))));
    const int32 AoELoss = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AoEInitiativeLossAmount"))));
    if (!RoundGameState || Radius <= 0 || AoELoss <= 0)
    {
        return Result;
    }

    TSet<ARoundGridTile*> Visited;
    TArray<TPair<ARoundGridTile*, int32>> Frontier;
    Frontier.Add({TargetTile, 0});
    Visited.Add(TargetTile);

    while (Frontier.Num() > 0)
    {
        const TPair<ARoundGridTile*, int32> Current = Frontier[0];
        Frontier.RemoveAt(0);
        if (Current.Value > 0)
        {
            if (ARoundCharacter* Character = Current.Key ? Current.Key->OccupyingCharacter : nullptr)
            {
                if (OwningCharacter->IsEnemyFor(Character))
                {
                    Character->ApplyDelay(AoELoss);
                }
            }
        }

        if (Current.Value >= Radius)
        {
            continue;
        }

        for (ARoundGridTile* Neighbor : RoundGameState->GetNeighborFields(Current.Key))
        {
            if (Neighbor && !Visited.Contains(Neighbor))
            {
                Visited.Add(Neighbor);
                Frontier.Add({Neighbor, Current.Value + 1});
            }
        }
    }

    return Result;
}
