#include "Effects/RoundFireballBurnEffect.h"

#include "Actors/RoundCharacter.h"

void ARoundFireballBurnEffect::OnTurnStarted(ARoundCharacter* TargetCharacter)
{
    if (!TargetCharacter || Amount <= 0.0)
    {
        return;
    }

    bool bDidDie = false;
    TargetCharacter->ApplyRoundDamage(FMath::RoundToInt(Amount), ERoundDamageType::Fire, 0, CauserCharacter, bDidDie);
}
