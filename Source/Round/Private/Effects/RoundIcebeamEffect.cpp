#include "Effects/RoundIcebeamEffect.h"

#include "Actors/RoundCharacter.h"

void ARoundIcebeamEffect::OnOwnerMoved(ARoundCharacter* TargetCharacter, ARoundGridTile* PreviousTile, ARoundGridTile* NewTile)
{
    ARoundSlowEffect::OnOwnerMoved(TargetCharacter, PreviousTile, NewTile);
    if (!TargetCharacter || !PreviousTile || !NewTile || PreviousTile == NewTile)
    {
        return;
    }

    if (bFirstMovePenaltyAvailable && FirstMoveInitiativeLoss > 0)
    {
        bFirstMovePenaltyAvailable = false;
        TargetCharacter->ApplyDelay(FirstMoveInitiativeLoss);
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
