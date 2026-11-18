#include "Effects/RoundShieldOathEffect.h"

#include "Actors/RoundCharacter.h"
#include "Components/RoundAttributeSetComponent.h"

int32 ARoundShieldOathEffect::ModifyIncomingDamageBeforeMitigation(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType) const
{
    const double RedirectFraction = FMath::Clamp(RedirectPreMitigationFraction, 0.0, 1.0);
    if (RedirectFraction <= 0.0 || !CauserCharacter || CauserCharacter == TargetCharacter)
    {
        return Damage;
    }

    const int32 RedirectDamage = FMath::Clamp(FMath::RoundToInt(static_cast<double>(Damage) * RedirectFraction), 0, Damage);
    if (RedirectDamage <= 0)
    {
        return Damage;
    }

    bool bRedirectDidDie = false;
    const int32 DamageToPaladin = FMath::Max(0, FMath::RoundToInt(static_cast<double>(RedirectDamage) * FMath::Max(0.0, RedirectedDamageMultiplier)));
    CauserCharacter->ApplyDirectDamage(DamageToPaladin, DamageType, SourceCharacter, bRedirectDidDie);
    if (PaladinHealFraction > 0.0)
    {
        CauserCharacter->Heal(FMath::RoundToInt(static_cast<double>(RedirectDamage) * PaladinHealFraction));
    }
    if (ProtectedAllyInitiativePerHit > 0 && TargetCharacter && TargetCharacter->Attributes)
    {
        TargetCharacter->Attributes->GainInitiative(ProtectedAllyInitiativePerHit);
    }
    return FMath::Max(0, Damage - RedirectDamage);
}
