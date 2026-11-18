#include "Effects/RoundSpottedEffect.h"

bool ARoundSpottedEffect::SetHasVision_Implementation(ARoundCharacter* TargetCharacter, bool bCurrentVision) const
{
    return true;
}

int32 ARoundSpottedEffect::ModifyIncomingDamage(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const
{
    return FMath::Max(0, FMath::RoundToInt(static_cast<double>(Damage) * FMath::Max(0.0, DamageTakenMultiplier)));
}
