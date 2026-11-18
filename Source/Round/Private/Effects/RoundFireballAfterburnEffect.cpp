#include "Effects/RoundFireballAfterburnEffect.h"

#include "Abilities/RoundFireballAbility.h"
#include "Actors/RoundGridTile.h"
#include "Game/RoundGameState.h"

void ARoundFireballAfterburnEffect::OnInitiativeAdvanced(ARoundCharacter* TargetCharacter, int32 InitiativeAmount)
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

    ARoundGridTile* TargetTile = nullptr;
    if (ARoundGameState* GameState = ARoundGameState::GetRoundGameState(this))
    {
        TargetTile = GameState->GetField(TargetTilePosition);
    }
    if (FireballAbility && TargetTile)
    {
        FireballAbility->ResolveAfterburn(TargetTile, DamageMultiplier);
    }
    RemoveEffect();
}
