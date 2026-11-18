#include "Effects/RoundBleedEffect.h"

#include "Actors/RoundCharacter.h"

void ARoundBleedEffect::OnTurnStarted(ARoundCharacter* TargetCharacter)
{
    if (!TargetCharacter)
    {
        return;
    }

    const int32 BleedDamage = FMath::Max(0, FMath::RoundToInt(Amount));
    if (BleedDamage <= 0)
    {
        return;
    }

    bool bDidDie = false;
    TargetCharacter->ApplyDirectDamage(BleedDamage, ERoundDamageType::Physical, CauserCharacter, bDidDie);
}
