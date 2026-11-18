#include "Effects/RoundBasicAttackDamageReductionEffect.h"

#include "Net/UnrealNetwork.h"

void ARoundBasicAttackDamageReductionEffect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundBasicAttackDamageReductionEffect, DamageMultiplier);
}

int32 ARoundBasicAttackDamageReductionEffect::ModifyBasicAttackDamage(ARoundCharacter*, ARoundCharacter*, int32 Damage) const
{
    const_cast<ARoundBasicAttackDamageReductionEffect*>(this)->RemoveEffect();
    return FMath::Max(0, FMath::RoundToInt(static_cast<double>(Damage) * FMath::Max(0.0, DamageMultiplier)));
}
