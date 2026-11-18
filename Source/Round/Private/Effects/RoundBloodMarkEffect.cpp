#include "Effects/RoundBloodMarkEffect.h"

int32 ARoundBloodMarkEffect::ModifyIncomingDamage(ARoundCharacter*, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType, int32&) const
{
    if (Damage <= 0 || SourceCharacter != CauserCharacter || DamageMultiplier <= 0.0)
    {
        return Damage;
    }

    const int32 Result = FMath::Max(0, FMath::RoundToInt(static_cast<double>(Damage) * DamageMultiplier));
    const_cast<ARoundBloodMarkEffect*>(this)->RemoveEffect();
    return Result;
}
