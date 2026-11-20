#include "Effects/RoundIcebeamEffect.h"

#include "Actors/RoundCharacter.h"
#include "Game/RoundGameState.h"

void ARoundIcebeamEffect::OnOwnerMoved(ARoundCharacter* TargetCharacter, ARoundGridTile* PreviousTile, ARoundGridTile* NewTile)
{
    ARoundSlowEffect::OnOwnerMoved(TargetCharacter, PreviousTile, NewTile);
    if (!TargetCharacter || !PreviousTile || !NewTile || PreviousTile == NewTile)
    {
        return;
    }

    ARoundGameState* GameState = ARoundGameState::GetRoundGameState(TargetCharacter);
    const bool bCoreAppliedMovement = GameState && GameState->IsApplyingCoreMovementCallbacks();
    if (bFirstMovePenaltyAvailable)
    {
        bFirstMovePenaltyAvailable = false;
        if (FirstMoveInitiativeLoss > 0)
        {
            if (bCoreAppliedMovement)
            {
                GameState->RecordDelayApplied(TargetCharacter, FirstMoveInitiativeLoss);
            }
            else
            {
                TargetCharacter->ApplyDelay(FirstMoveInitiativeLoss);
            }
        }
    }

    if (MaximumMovedFields > 0 && ++MovedFields >= MaximumMovedFields)
    {
        RemoveEffect();
    }
}

void ARoundIcebeamEffect::OnDamageTaken(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 AppliedDamage, ERoundDamageType DamageType)
{
    if (!TargetCharacter || SourceCharacter != CauserCharacter || AppliedDamage <= 0 || DamageType == ERoundDamageType::Physical || FrostbreakDamage <= 0)
    {
        return;
    }

    const int32 BonusDamage = FrostbreakDamage;
    RemoveEffect();
    bool bDidDie = false;
    TargetCharacter->ApplyRoundDamage(BonusDamage, ERoundDamageType::Ice, 0, SourceCharacter, bDidDie);
}
