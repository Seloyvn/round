#include "Effects/RoundShieldEffect.h"

int32 ARoundShieldEffect::ModifyIncomingDamageBeforeMitigation(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType) const
{
    ARoundShieldEffect* MutableThis = const_cast<ARoundShieldEffect*>(this);
    if (!MutableThis || MutableThis->ShieldAmountRemaining <= 0 || Damage <= 0)
    {
        return Damage;
    }

    const int32 PreventedDamage = FMath::Clamp(Damage, 0, MutableThis->ShieldAmountRemaining);
    MutableThis->ShieldAmountRemaining = FMath::Max(0, MutableThis->ShieldAmountRemaining - PreventedDamage);
    if (MutableThis->ShieldAmountRemaining <= 0)
    {
        MutableThis->RemoveEffect();
    }

    return FMath::Max(0, Damage - PreventedDamage);
}
