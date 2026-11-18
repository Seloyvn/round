#include "Effects/RoundDeferredDamageEffect.h"

#include "Actors/RoundCharacter.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Net/UnrealNetwork.h"

void ARoundDeferredDamageEffect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundDeferredDamageEffect, DeferredDamageFraction);
    DOREPLIFETIME(ARoundDeferredDamageEffect, StoredDeferredDamage);
    DOREPLIFETIME(ARoundDeferredDamageEffect, DeferredDamageForgivenFraction);
    DOREPLIFETIME(ARoundDeferredDamageEffect, HealOnResolve);
    DOREPLIFETIME(ARoundDeferredDamageEffect, InitiativeGainOnResolve);
}

int32 ARoundDeferredDamageEffect::ModifyIncomingDamageBeforeMitigation(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType) const
{
    ARoundDeferredDamageEffect* MutableThis = const_cast<ARoundDeferredDamageEffect*>(this);
    if (!MutableThis || Damage <= 0 || DeferredDamageFraction <= 0.0)
    {
        return Damage;
    }

    const int32 DeferredDamage = FMath::Clamp(FMath::RoundToInt(static_cast<double>(Damage) * DeferredDamageFraction), 0, Damage);
    MutableThis->StoredDeferredDamage += DeferredDamage;
    return FMath::Max(0, Damage - DeferredDamage);
}

void ARoundDeferredDamageEffect::OnInitiativeAdvanced(ARoundCharacter* TargetCharacter, int32 InitiativeAmount)
{
    if (InitiativeAmount <= 0 || RemainingInitiative <= 0)
    {
        return;
    }

    RemainingInitiative = FMath::Max(0, RemainingInitiative - InitiativeAmount);
    if (RemainingInitiative > 0)
    {
        return;
    }

    if (TargetCharacter && StoredDeferredDamage > 0)
    {
        bool bDidDie = false;
        const int32 ResolvedDamage = FMath::RoundToInt(StoredDeferredDamage * (1.0 - FMath::Clamp(DeferredDamageForgivenFraction, 0.0, 1.0)));
        TargetCharacter->ApplyDirectDamage(ResolvedDamage, ERoundDamageType::TrueDamage, CauserCharacter, bDidDie);
        if (!bDidDie)
        {
            TargetCharacter->Heal(HealOnResolve);
            if (TargetCharacter->Attributes) TargetCharacter->Attributes->GainInitiative(InitiativeGainOnResolve);
        }
    }

    StoredDeferredDamage = 0;
    RemoveEffect();
}
