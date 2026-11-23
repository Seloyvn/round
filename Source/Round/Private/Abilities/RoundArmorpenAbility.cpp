#include "Abilities/RoundArmorpenAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter3.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundBasicAttackDamageReductionEffect.h"
#include "Effects/RoundBleedEffect.h"
#include "Effects/RoundModifierEffect.h"
#include "Effects/RoundSharedBreachEffect.h"

FRoundActionResult ARoundArmorpenAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 17)
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
    if (!TargetCharacter)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    bool bTargetWasDebuffed = false;
    for (ARoundEffectActor* Effect : TargetCharacter->ActiveEffects)
    {
        if (Effect && Effect->IsDebuff())
        {
            bTargetWasDebuffed = true;
            break;
        }
    }

    if (!OwningCharacter->Attributes->SpendInitiative(GetAbilityCost()))
    {
        Result.Reason = TEXT("Not enough initiative.");
        return Result;
    }

    SetCooldown(GetAbilityCooldown());

    if (GetAbilityValueFromKey(TEXT("RemoveOldestRemovableBuff")) > 0.0)
    {
        const TArray<TObjectPtr<ARoundEffectActor>> EffectsSnapshot = TargetCharacter->ActiveEffects;
        for (ARoundEffectActor* Effect : EffectsSnapshot)
        {
            if (Effect && Effect->IsBuff() && Effect->IsRemovable())
            {
                Effect->RemoveEffect();
                break;
            }
        }
    }

    int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("Armorpen"))));
    const int32 TargetDefenseBeforeHit = TargetCharacter->Attributes ? TargetCharacter->Attributes->BaseStats.Defense : 0;
    bool bDidDie = false;
    const int32 FinalDamage = OwningCharacter->ModifyOutgoingDamage(GetAbilityDamage(), GetAbilityDamageType(), TargetCharacter, DefenseIgnore);
    const int32 AppliedDamage = TargetCharacter->ApplyRoundDamage(FinalDamage, GetAbilityDamageType(), DefenseIgnore, OwningCharacter, bDidDie);

    if (ARoundCharacter3* Character3 = Cast<ARoundCharacter3>(OwningCharacter))
    {
        const int32 ComboBefore = Character3->TryGetComboPoints();
        int32 ComboGain = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ComboPointsGain"))));
        if (TargetDefenseBeforeHit > Character3->Attributes->BaseStats.Defense)
        {
            ComboGain += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BonusComboVsHigherDefense"))));
        }
        Character3->TryGainComboPoints(ComboGain);

        if (ComboBefore < Character3->GetMaxComboPoints() && Character3->TryGetComboPoints() >= Character3->GetMaxComboPoints())
        {
            ReduceCooldown(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionOnReachMaxCombo")))));
        }
    }

    if (bTargetWasDebuffed && OwningCharacter->Attributes)
    {
        OwningCharacter->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeRefundVsDebuffed")))));
    }

    const double BleedFractionOnSurvive = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("BleedDamageFractionOnSurvive")));
    const int32 BleedDuration = FMath::Max(0, GetAbilityDurationInitiativeFromKey(TEXT("BleedDurationInitiative")));
    if (!bDidDie && BleedFractionOnSurvive > 0.0 && BleedDuration > 0)
    {
        const int32 BleedDamage = FMath::Max(1, FMath::RoundToInt(static_cast<double>(AppliedDamage) * BleedFractionOnSurvive));
        if (ARoundBleedEffect* BleedEffect = GetWorld()->SpawnActor<ARoundBleedEffect>(ARoundBleedEffect::StaticClass()))
        {
            BleedEffect->EffectId = TEXT("ArmorpenBleed");
            BleedEffect->DisplayName = TEXT("Bleed");
            BleedEffect->Description = TEXT("Blutet nach Armorpen.");
            BleedEffect->Amount = BleedDamage;
            BleedEffect->CauserCharacter = OwningCharacter;
            BleedEffect->SetDurationInitiative(BleedDuration, false);
            TargetCharacter->AddEffect(BleedEffect);
        }
    }

    if (!bDidDie && GetWorld())
    {
        const int32 DefenseReduction = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseReduction"))));
        const int32 DefenseReductionDuration = GetAbilityDurationInitiativeFromKey(TEXT("DefenseReductionDurationInitiative"));
        if (DefenseReduction > 0 && DefenseReductionDuration > 0)
        {
            if (ARoundModifierEffect* Debuff = GetWorld()->SpawnActor<ARoundModifierEffect>())
            {
                Debuff->EffectId = TEXT("ArmorpenDefenseReduction");
                Debuff->DisplayName = TEXT("Ruestungsbruch");
                Debuff->Description = TEXT("Defense reduziert.");
                Debuff->DefenseIgnoreAdd = DefenseReduction;
                Debuff->bTreatAsDebuff = true;
                Debuff->CauserCharacter = OwningCharacter;
                Debuff->SetDurationInitiative(DefenseReductionDuration, false);
                TargetCharacter->AddEffect(Debuff);
            }
        }

        const double BasicAttackMultiplier = GetAbilityValueFromKey(TEXT("NextBasicAttackDamageMultiplier"));
        const int32 BasicAttackDuration = GetAbilityDurationInitiativeFromKey(TEXT("NextBasicAttackReductionDurationInitiative"));
        if (BasicAttackMultiplier > 0.0 && BasicAttackMultiplier < 1.0 && BasicAttackDuration > 0)
        {
            if (ARoundBasicAttackDamageReductionEffect* Debuff = GetWorld()->SpawnActor<ARoundBasicAttackDamageReductionEffect>())
            {
                Debuff->EffectId = TEXT("ArmorpenWeakenedBasicAttack");
                Debuff->DisplayName = TEXT("Durchtrennter Schlagarm");
                Debuff->Description = TEXT("Der naechste normale Angriff verursacht weniger Schaden.");
                Debuff->DamageMultiplier = BasicAttackMultiplier;
                Debuff->CauserCharacter = OwningCharacter;
                Debuff->SetDurationInitiative(BasicAttackDuration, false);
                TargetCharacter->AddEffect(Debuff);
            }
        }

        const int32 BreachDuration = GetAbilityDurationInitiativeFromKey(TEXT("SharedBreachDurationInitiative"));
        if (BreachDuration > 0 && DefenseIgnore > 0)
        {
            if (ARoundSharedBreachEffect* Breach = GetWorld()->SpawnActor<ARoundSharedBreachEffect>())
            {
                Breach->EffectId = TEXT("ArmorpenSharedBreach");
                Breach->DisplayName = TEXT("Geteilte Bresche");
                Breach->Description = TEXT("Der erste Treffer jedes Verbuendeten ignoriert zusaetzliche Defense.");
                Breach->DefenseIgnorePerCharacter = DefenseIgnore;
                Breach->CauserCharacter = OwningCharacter;
                Breach->SetDurationInitiative(BreachDuration, false);
                TargetCharacter->AddEffect(Breach);
            }
        }
    }

    const double SecondStrikeFraction = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("SecondStrikeAppliedDamageFraction")));
    if (!bDidDie && AppliedDamage > 0 && SecondStrikeFraction > 0.0)
    {
        bool bSecondStrikeKilled = false;
        TargetCharacter->ApplyUnreactiveFinalDamage(
            FMath::Max(1, FMath::RoundToInt(static_cast<double>(AppliedDamage) * SecondStrikeFraction)),
            bSecondStrikeKilled);
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Armorpen executed.");
    return Result;
}
