#include "Effects/RoundPaladinAuraEffect.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacterPaladin.h"
#include "Game/RoundGameState.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Actors/RoundAbilityActor.h"

int32 ARoundPaladinAuraEffect::ModifyIncomingDamageForCharacter(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore)
{
    if (!AffectsCharacter(TargetCharacter))
    {
        return Damage;
    }

    const int32 OriginalDamage = Damage;
    const ARoundCharacterPaladin* PaladinCharacter = Cast<ARoundCharacterPaladin>(OwningCharacter);
    const double DamageReduction = FMath::Clamp(
        PaladinCharacter ? PaladinCharacter->GetAuraDamageReduction() : 0.0,
        0.0,
        0.95);
    Damage = FMath::Max(0, FMath::RoundToInt(static_cast<double>(Damage) * (1.0 - DamageReduction)));
    const int32 PreventedDamage = FMath::Max(0, OriginalDamage - Damage);
    if (PreventedDamage > 0)
    {
        NumericState.FindOrAdd(TEXT("AuraPreventedDamageTotal")) += PreventedDamage;
        if (ARoundCharacterPaladin* Paladin = Cast<ARoundCharacterPaladin>(OwningCharacter))
        {
            Paladin->Heal(FMath::RoundToInt(static_cast<double>(PreventedDamage) * FMath::Max(0.0, Paladin->GetPassiveValueFromKey(TEXT("HealFractionOfAuraPreventedDamage")))));
            if (Paladin->Attributes)
            {
                Paladin->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(Paladin->GetPassiveValueFromKey(TEXT("InitiativeOnAuraPrevent")))));
            }
            const int32 CooldownReduction = FMath::Max(0, FMath::RoundToInt(Paladin->GetPassiveValueFromKey(TEXT("CooldownReductionOnAuraPrevent"))));
            if (CooldownReduction > 0)
            {
                for (ARoundAbilityActor* Ability : Paladin->GrantedAbilities) if (Ability) Ability->ReduceCooldown(CooldownReduction);
            }
        }
    }

    if (PreventedDamage > 0 && SourceCharacter)
    {
        const int32 CurrentInitiative = ARoundGameState::GetRoundGameState(TargetCharacter)
            ? ARoundGameState::GetRoundGameState(TargetCharacter)->GetRecordedGlobalInitiativeElapsed()
            : 0;
        RecentPreventedDamageAtBySource.Add(SourceCharacter, CurrentInitiative);
    }

    return Damage;
}

bool ARoundPaladinAuraEffect::TryGetIncomingDamageReductionFraction(double& OutFraction) const
{
    const ARoundCharacterPaladin* PaladinCharacter = Cast<ARoundCharacterPaladin>(OwningCharacter);
    OutFraction = FMath::Clamp(
        PaladinCharacter ? PaladinCharacter->GetAuraDamageReduction() : 0.0,
        0.0,
        0.95);
    return OutFraction > 0.0;
}

bool ARoundPaladinAuraEffect::HasRecentPreventedDamageFrom(const ARoundCharacter* SourceCharacter, int32 WindowInitiative) const
{
    if (!SourceCharacter)
    {
        return false;
    }

    if (const int32* LastPreventedAt = RecentPreventedDamageAtBySource.Find(SourceCharacter))
    {
        const int32 CurrentInitiative = ARoundGameState::GetRoundGameState(SourceCharacter)
            ? ARoundGameState::GetRoundGameState(SourceCharacter)->GetRecordedGlobalInitiativeElapsed()
            : 0;
        return (*LastPreventedAt + WindowInitiative) >= CurrentInitiative;
    }

    return false;
}
