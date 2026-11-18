#include "Effects/RoundSpinEchoEffect.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Game/RoundGameState.h"

void ARoundSpinEchoEffect::OnInitiativeAdvanced(ARoundCharacter* TargetCharacter, int32 InitiativeAmount)
{
    const bool bResolve = RemainingInitiative > 0 && InitiativeAmount >= RemainingInitiative;
    if (bResolve && TargetCharacter && TargetCharacter->CurrentTile)
    {
        if (ARoundGameState* GameState = TargetCharacter->GetRoundGameState())
        {
            for (ARoundGridTile* Tile : GameState->GetFields())
            {
                if (!Tile || TargetCharacter->CurrentTile->GetDistance(Tile) > FMath::Max(1, Radius))
                {
                    continue;
                }
                ARoundCharacter* Enemy = Tile ? Tile->OccupyingCharacter : nullptr;
                if (!Enemy || !TargetCharacter->IsEnemyFor(Enemy))
                {
                    continue;
                }
                bool bDidDie = false;
                int32 DefenseIgnore = 0;
                const int32 FinalDamage = TargetCharacter->ModifyOutgoingDamage(Damage, ERoundDamageType::Physical, Enemy, DefenseIgnore);
                Enemy->ApplyRoundDamage(FinalDamage, ERoundDamageType::Physical, DefenseIgnore, TargetCharacter, bDidDie);
            }
        }
    }
    Super::OnInitiativeAdvanced(TargetCharacter, InitiativeAmount);
}
