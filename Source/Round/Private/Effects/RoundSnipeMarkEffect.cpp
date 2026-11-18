#include "Effects/RoundSnipeMarkEffect.h"

#include "Actors/RoundCharacter.h"

int32 ARoundSnipeMarkEffect::ModifyIncomingDamage(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const
{
    if (!CauserCharacter || SourceCharacter != CauserCharacter || Amount <= 0.0)
    {
        return Damage;
    }

    const int32 ModifiedDamage = FMath::Max(0, FMath::RoundToInt(static_cast<double>(Damage) * Amount));
    const_cast<ARoundSnipeMarkEffect*>(this)->RemoveEffect();
    return ModifiedDamage;
}
