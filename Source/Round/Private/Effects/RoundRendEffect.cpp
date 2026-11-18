#include "Effects/RoundRendEffect.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter5.h"

namespace
{
void ApplyRendTick(ARoundRendEffect* Effect, ARoundCharacter* TargetCharacter, int32 Damage)
{
    if (!Effect || !TargetCharacter || Damage <= 0)
    {
        return;
    }
    bool bDidDie = false;
    const int32 AppliedDamage = TargetCharacter->ApplyDirectDamage(Damage, ERoundDamageType::Physical, Effect->CauserCharacter, bDidDie);
    if (AppliedDamage > 0 && Effect->BloodPerTick > 0)
    {
        if (ARoundCharacter5* Character5 = Cast<ARoundCharacter5>(Effect->CauserCharacter))
        {
            Character5->GainRawBlood(Effect->BloodPerTick);
        }
    }
}
}

void ARoundRendEffect::OnTurnStarted(ARoundCharacter* TargetCharacter)
{
    ApplyRendTick(this, TargetCharacter, FMath::Max(0, FMath::RoundToInt(Amount)));
}

void ARoundRendEffect::OnOwnerMoved(ARoundCharacter* TargetCharacter, ARoundGridTile*, ARoundGridTile*)
{
    if (!bMovementTriggered && MovementTriggerDamage > 0)
    {
        bMovementTriggered = true;
        ApplyRendTick(this, TargetCharacter, MovementTriggerDamage);
    }
}
