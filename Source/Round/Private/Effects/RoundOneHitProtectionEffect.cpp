#include "Effects/RoundOneHitProtectionEffect.h"

int32 ARoundOneHitProtectionEffect::ModifyIncomingDamage(ARoundCharacter*, ARoundCharacter*, int32 Damage, ERoundDamageType, int32&) const
{
    if (Damage <= 0)
    {
        return Damage;
    }
    const int32 Result = FMath::Max(0, FMath::RoundToInt(Damage * FMath::Clamp(IncomingDamageMultiplier, 0.0, 1.0)));
    const_cast<ARoundOneHitProtectionEffect*>(this)->RemoveEffect();
    return Result;
}
