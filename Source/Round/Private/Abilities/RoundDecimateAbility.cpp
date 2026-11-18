#include "Abilities/RoundDecimateAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter5.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundModifierEffect.h"
#include "Game/RoundGameState.h"

FRoundActionResult ARoundDecimateAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    if (!TargetCharacter || !TargetCharacter->Attributes)
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

    const int32 HealthBeforeHit = TargetCharacter->Attributes->CurrentHealth;
    int32 Damage = GetAbilityDamage();
    const double PercentDamage = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("PercentDamage")));
    if (PercentDamage > 0.0)
    {
        Damage += FMath::RoundToInt(static_cast<double>(TargetCharacter->Attributes->BaseStats.MaxHealth) * PercentDamage);
    }

    const double HealthFraction = TargetCharacter->Attributes->BaseStats.MaxHealth > 0
        ? static_cast<double>(TargetCharacter->Attributes->CurrentHealth) / TargetCharacter->Attributes->BaseStats.MaxHealth : 1.0;
    const double HighThreshold = GetAbilityValueFromKey(TEXT("HighHealthThreshold"));
    const double HighMultiplier = GetAbilityValueFromKey(TEXT("DamageMultiplierVsHighHealth"));
    if (HighThreshold > 0.0 && HealthFraction >= HighThreshold && HighMultiplier > 0.0) Damage = FMath::RoundToInt(Damage * HighMultiplier);
    const double LowThreshold = GetAbilityValueFromKey(TEXT("LowHealthThreshold"));
    const double LowMultiplier = GetAbilityValueFromKey(TEXT("DamageMultiplierVsLowHealth"));
    if (LowThreshold > 0.0 && HealthFraction <= LowThreshold && LowMultiplier > 0.0) Damage = FMath::RoundToInt(Damage * LowMultiplier);

    int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    if (ARoundCharacter5* Character5 = Cast<ARoundCharacter5>(OwningCharacter))
    {
        const int32 RequiredBlood = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RequiredBloodForDefenseIgnore"))));
        if (RequiredBlood > 0 && Character5->GetCurrentBlood() >= RequiredBlood)
        {
            DefenseIgnore += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnoreIfBloodAtLeast"))));
        }
        const int32 BloodCost = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BloodCostForBonusDamage"))));
        if (BloodCost > 0 && Character5->GetCurrentBlood() >= BloodCost)
        {
            Character5->TryConsumeBlood(BloodCost);
            Damage += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BonusDamageForBlood"))));
        }
    }

    bool bDidDie = false;
    const int32 FinalDamage = OwningCharacter->ModifyOutgoingDamage(Damage, GetAbilityDamageType(), TargetCharacter, DefenseIgnore);
    const int32 AppliedDamage = TargetCharacter->ApplyRoundDamage(FinalDamage, GetAbilityDamageType(), DefenseIgnore, OwningCharacter, bDidDie);

    const int32 InitiativeLoss = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnHit"))));
    if (!bDidDie && InitiativeLoss > 0) TargetCharacter->ApplyDelay(InitiativeLoss);

    const int32 DefenseReduction = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseReduction"))));
    const int32 DebuffDuration = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DebuffDurationInitiative"))));
    if (!bDidDie && DefenseReduction > 0 && DebuffDuration > 0)
    {
        if (ARoundModifierEffect* Break = GetWorld()->SpawnActor<ARoundModifierEffect>())
        {
            Break->DisplayName = TEXT("Ruestungsbruch"); Break->DefenseIgnoreAdd = DefenseReduction; Break->bOnlyAffectCauserDamage = true;
            Break->bTreatAsDebuff = true; Break->CauserCharacter = OwningCharacter; Break->SetDurationInitiative(DebuffDuration, false); TargetCharacter->AddEffect(Break);
        }
    }

    const double LifestealFraction = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("LifestealFractionOnHit")));
    if (LifestealFraction > 0.0)
    {
        OwningCharacter->Heal(FMath::Max(0, FMath::RoundToInt(static_cast<double>(AppliedDamage) * LifestealFraction)));
    }

    if (bDidDie)
    {
        const int32 InitiativeGainOnKill = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainOnKill"))));
        if (InitiativeGainOnKill > 0)
        {
            OwningCharacter->Attributes->GainInitiative(InitiativeGainOnKill);
        }
        if (ARoundCharacter5* Character5 = Cast<ARoundCharacter5>(OwningCharacter))
        {
            const double OverkillFraction = GetAbilityValueFromKey(TEXT("OverkillBloodFraction"));
            if (OverkillFraction > 0.0) Character5->GainRawBlood(FMath::RoundToInt(FMath::Max(0, AppliedDamage - HealthBeforeHit) * OverkillFraction));
        }
    }

    const double CleaveMultiplier = GetAbilityValueFromKey(TEXT("CleaveDamageMultiplier"));
    if (CleaveMultiplier > 0.0 && OwningCharacter->GetRoundGameState())
    {
        for (ARoundGridTile* Neighbor : OwningCharacter->GetRoundGameState()->GetNeighborFields(TargetTile))
        {
            ARoundCharacter* Enemy = Neighbor ? Neighbor->OccupyingCharacter : nullptr;
            if (!Enemy || Enemy == TargetCharacter || !OwningCharacter->IsEnemyFor(Enemy)) continue;
            bool bCleaveDied = false; int32 CleaveIgnore = DefenseIgnore;
            const int32 CleaveDamage = OwningCharacter->ModifyOutgoingDamage(FMath::RoundToInt(GetAbilityDamage() * CleaveMultiplier), GetAbilityDamageType(), Enemy, CleaveIgnore);
            Enemy->ApplyRoundDamage(CleaveDamage, GetAbilityDamageType(), CleaveIgnore, OwningCharacter, bCleaveDied);
        }
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Decimate executed.");
    return Result;
}
