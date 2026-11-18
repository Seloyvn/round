#include "Effects/RoundNextOutgoingDamageEffect.h"

int32 ARoundNextOutgoingDamageEffect::ModifyOutgoingDamage(ARoundCharacter*, ARoundCharacter*, int32 Damage, ERoundDamageType, int32&) const
{
    if (Damage <= 0 || Amount <= 0.0) return Damage;
    const int32 ModifiedDamage = FMath::Max(0, FMath::RoundToInt(Damage * Amount));
    const_cast<ARoundNextOutgoingDamageEffect*>(this)->RemoveEffect();
    return ModifiedDamage;
}
