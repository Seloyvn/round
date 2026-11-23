#include "Abilities/RoundExecuteAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter3.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundBleedEffect.h"

namespace
{
bool ExecuteTargetHasDebuff(const ARoundCharacter* TargetCharacter)
{
    if (!TargetCharacter) return false;
    for (const ARoundEffectActor* Effect : TargetCharacter->ActiveEffects)
    {
        if (Effect && Effect->IsDebuff()) return true;
    }
    return false;
}
}

bool ARoundExecuteAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    const ARoundCharacter3* Character3 = Cast<ARoundCharacter3>(OwningCharacter);
    if (!AbilityData || !Character3 || !OwningCharacter->CurrentTile || !TargetTile
        || GetAbilityVisibility() != ERoundButtonState::Enabled || !MatchesFieldState(TargetTile))
    {
        return false;
    }

    if (GetAbilityValueFromKey(TEXT("RequireMaxCombo")) > 0.0
        && Character3->TryGetComboPoints() < Character3->GetMaxComboPoints())
    {
        return false;
    }

    int32 Range = GetAbilityRange();
    const ARoundCharacter* TargetCharacter = TargetTile->OccupyingCharacter;
    if (TargetCharacter && TargetCharacter->Attributes && TargetCharacter->Attributes->BaseStats.MaxHealth > 0)
    {
        const double HealthFraction = static_cast<double>(TargetCharacter->Attributes->CurrentHealth)
            / static_cast<double>(TargetCharacter->Attributes->BaseStats.MaxHealth);
        const double Threshold = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("BonusRangeHealthThreshold")));
        if (Threshold > 0.0 && HealthFraction <= Threshold)
        {
            Range += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RangeVsLowHealth"))));
        }
    }

    return OwningCharacter->CurrentTile->GetDistance(TargetTile) <= Range;
}

FRoundActionResult ARoundExecuteAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 18)
    {
        return ARoundAbilityActor::ExecuteAbility(TargetTile);
    }
    FRoundActionResult Result;
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    ARoundCharacter3* Character3 = Cast<ARoundCharacter3>(OwningCharacter);
    if (!TargetCharacter || !Character3)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    if (!OwningCharacter->Attributes->SpendInitiative(GetAbilityCost()))
    {
        Result.Reason = TEXT("Not enough initiative.");
        return Result;
    }

    SetCooldown(GetAbilityCooldown());

    const int32 MaxComboConsumed = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MaxComboConsumed"))) > 0
        ? FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MaxComboConsumed")))
        : MAX_int32;
    const int32 ConsumedCombo = Character3->TryConsumeComboPoints(MaxComboConsumed);
    int32 DamagePerCombo = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DamagePerCombopoint")));
    const double ComboDamageMultiplier = GetAbilityValueFromKey(TEXT("ComboDamageMultiplier"));
    if (ComboDamageMultiplier > 0.0)
    {
        DamagePerCombo = FMath::RoundToInt(static_cast<double>(DamagePerCombo) * ComboDamageMultiplier);
    }
    int32 Damage = GetAbilityDamage() + DamagePerCombo * ConsumedCombo;
    int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    if (ExecuteTargetHasDebuff(TargetCharacter))
    {
        DefenseIgnore += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnoreVsDebuffed"))));
    }
    if (ConsumedCombo >= FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ComboThresholdForDefenseIgnore")))))
    {
        DefenseIgnore += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnoreAtComboThreshold"))));
    }
    DefenseIgnore += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnoreAtMaxCombo"))));

    if (TargetCharacter->Attributes && TargetCharacter->Attributes->BaseStats.MaxHealth > 0)
    {
        const double HealthFraction = static_cast<double>(TargetCharacter->Attributes->CurrentHealth)
            / static_cast<double>(TargetCharacter->Attributes->BaseStats.MaxHealth);
        const double LowHealthThreshold = GetAbilityValueFromKey(TEXT("LowHealthDamageThreshold"));
        const double LowHealthMultiplier = GetAbilityValueFromKey(TEXT("DamageMultiplierVsLowHealth"));
        if (LowHealthThreshold > 0.0 && HealthFraction <= LowHealthThreshold && LowHealthMultiplier > 0.0)
        {
            Damage = FMath::RoundToInt(static_cast<double>(Damage) * LowHealthMultiplier);
        }
    }
    bool bDidDie = false;
    const int32 FinalDamage = OwningCharacter->ModifyOutgoingDamage(Damage, GetAbilityDamageType(), TargetCharacter, DefenseIgnore);
    const int32 AppliedDamage = TargetCharacter->ApplyRoundDamage(FinalDamage, GetAbilityDamageType(), DefenseIgnore, OwningCharacter, bDidDie);

    if (!bDidDie)
    {
        const double BleedFractionOnSurvive = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("BleedDamageFractionOnSurvive")));
        const int32 BleedDuration = FMath::Max(0, GetAbilityDurationInitiativeFromKey(TEXT("BleedDurationInitiative")));
        if (BleedFractionOnSurvive > 0.0 && BleedDuration > 0)
        {
            const int32 BleedDamage = FMath::Max(1, FMath::RoundToInt(static_cast<double>(AppliedDamage) * BleedFractionOnSurvive));
            if (ARoundBleedEffect* BleedEffect = GetWorld()->SpawnActor<ARoundBleedEffect>(ARoundBleedEffect::StaticClass()))
            {
                BleedEffect->EffectId = TEXT("ExecuteBleed");
                BleedEffect->DisplayName = TEXT("Bleed");
                BleedEffect->Description = TEXT("Blutet nach Execute.");
                BleedEffect->Amount = BleedDamage;
                BleedEffect->CauserCharacter = OwningCharacter;
                BleedEffect->SetDurationInitiative(BleedDuration, false);
                TargetCharacter->AddEffect(BleedEffect);
            }
        }
    }

    if (bDidDie)
    {
        const int32 InitiativeGainOnKill = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainOnKill"))));
        if (InitiativeGainOnKill > 0)
        {
            OwningCharacter->Attributes->GainInitiative(InitiativeGainOnKill);
        }

        const int32 ComboRefundOnKillDivisor = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ComboRefundOnKillDivisor"))));
        if (ComboRefundOnKillDivisor > 0)
        {
            Character3->TryGainComboPoints(ConsumedCombo / ComboRefundOnKillDivisor);
        }
        if (GetAbilityValueFromKey(TEXT("GrantFreeAdjacentMoveOnKill")) > 0.0)
        {
            Character3->GrantShadowstrikeFreeStep();
        }
    }
    else
    {
        Character3->TryGainComboPoints(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ComboRefundOnSurvive")))));
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Execute executed.");
    return Result;
}
