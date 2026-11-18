#include "Effects/RoundDelayedAbilityEffect.h"

#include "Actors/RoundAbilityActor.h"
#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Game/RoundGameState.h"

bool ARoundDelayedAbilityEffect::PreventsAbilityUse(const ARoundAbilityActor* Ability) const
{
    return Ability && Ability == DelayedAbility;
}

int32 ARoundDelayedAbilityEffect::GetTurnBlockRemainingInitiative() const
{
    return bAllowTurnsWhileDelayed ? 0 : FMath::Max(0, RemainingInitiative);
}

void ARoundDelayedAbilityEffect::OnInitiativeAdvanced(ARoundCharacter* TargetCharacter, int32 InitiativeAmount)
{
    if (InitiativeAmount <= 0 || bHasTriggered || RemainingInitiative <= 0)
    {
        return;
    }

    RemainingInitiative = FMath::Max(0, RemainingInitiative - InitiativeAmount);
    if (RemainingInitiative > 0)
    {
        return;
    }

    bHasTriggered = true;

    ARoundGridTile* ResolvedTargetTile = nullptr;
    if (bPreferTrackedTargetCharacter && TrackedTargetCharacter && TrackedTargetCharacter->CurrentTile)
    {
        ResolvedTargetTile = TrackedTargetCharacter->CurrentTile;
    }
    else if (bHasTargetTile)
    {
        if (ARoundGameState* RoundGameState = ARoundGameState::GetRoundGameState(this))
        {
            ResolvedTargetTile = RoundGameState->GetField(TargetTilePosition);
        }
    }

    if (DelayedAbility)
    {
        DelayedAbility->ExecuteDelayedResolution(ResolvedTargetTile);
    }

    RemoveEffect();
}
