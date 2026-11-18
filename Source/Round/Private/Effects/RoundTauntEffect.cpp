#include "Effects/RoundTauntEffect.h"

#include "Actors/RoundCharacter.h"

int32 ARoundTauntEffect::ModifyOutgoingDamage(ARoundCharacter* SourceCharacter, ARoundCharacter* TargetCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const
{
    if (!SourceCharacter || !TargetCharacter)
    {
        return Damage;
    }

    if (CauserCharacter && TargetCharacter != CauserCharacter && Amount > 0.0)
    {
        return FMath::Max(0, FMath::RoundToInt(static_cast<double>(Damage) * Amount));
    }

    return Damage;
}
