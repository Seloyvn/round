#include "Effects/RoundStaticChargeEffect.h"

#include "Actors/RoundCharacter.h"

void ARoundStaticChargeEffect::OnOwnerMoved(ARoundCharacter* TargetCharacter, ARoundGridTile* PreviousTile, ARoundGridTile* NewTile)
{
    if (!TargetCharacter || !PreviousTile || !NewTile || PreviousTile == NewTile || Amount <= 0.0)
    {
        return;
    }

    const int32 TriggerDamage = FMath::RoundToInt(Amount);
    RemoveEffect();
    bool bDidDie = false;
    TargetCharacter->ApplyRoundDamage(TriggerDamage, ERoundDamageType::Lightning, 0, CauserCharacter, bDidDie);
}
