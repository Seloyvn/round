#include "Effects/RoundRockEchoEffect.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Game/RoundGameState.h"

void ARoundRockEchoEffect::OnInitiativeAdvanced(ARoundCharacter* TargetCharacter, int32 InitiativeAmount)
{
    if (InitiativeAmount <= 0 || RemainingInitiative <= 0)
    {
        return;
    }
    RemainingInitiative = FMath::Max(0, RemainingInitiative - InitiativeAmount);
    if (RemainingInitiative > 0)
    {
        return;
    }

    ARoundGameState* GameState = ARoundGameState::GetRoundGameState(this);
    TArray<ARoundCharacter*> DamagedTargets;
    if (GameState)
    {
        for (const FVector2D& Position : RockTilePositions)
        {
            ARoundGridTile* RockTile = GameState->GetField(Position);
            if (!RockTile || RockTile->TerrainType != ERoundTerrainType::Rocks)
            {
                continue;
            }
            for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(RockTile))
            {
                ARoundCharacter* Enemy = Neighbor ? Neighbor->OccupyingCharacter : nullptr;
                if (!Enemy || DamagedTargets.Contains(Enemy) || !CauserCharacter || !CauserCharacter->IsEnemyFor(Enemy))
                {
                    continue;
                }
                bool bDidDie = false;
                Enemy->ApplyRoundDamage(FMath::RoundToInt(Amount), ERoundDamageType::Ground, 0, CauserCharacter, bDidDie);
                DamagedTargets.Add(Enemy);
            }
        }
    }
    RemoveEffect();
}
