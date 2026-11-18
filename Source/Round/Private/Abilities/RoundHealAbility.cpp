#include "Abilities/RoundHealAbility.h"

#include "Actors/RoundCharacter5.h"
#include "Actors/RoundEffectActor.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundModifierEffect.h"
#include "Game/RoundGameState.h"

FRoundActionResult ARoundHealAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!OwningCharacter || !OwningCharacter->Attributes)
    {
        Result.Reason = TEXT("Ability owner is invalid.");
        return Result;
    }

    if (GetAbilityVisibility() != ERoundButtonState::Enabled)
    {
        Result.Reason = TEXT("Ability is not ready.");
        return Result;
    }

    if (!OwningCharacter->Attributes->SpendInitiative(GetAbilityCost()))
    {
        Result.Reason = TEXT("Not enough initiative.");
        return Result;
    }

    SetCooldown(GetAbilityCooldown());

    int32 HealAmount = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BaseHeal"))));
    int32 ConsumedBlood = 0;
    if (ARoundCharacter5* Character5 = Cast<ARoundCharacter5>(OwningCharacter))
    {
        const int32 SpendLimit = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MaximumBloodConsumed"))));
        ConsumedBlood = Character5->TryConsumeBlood(SpendLimit);
        const int32 HealPerBlood = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("HealFromBlood"))));
        int32 BloodHealing = HealPerBlood * ConsumedBlood;
        if (OwningCharacter->Attributes->BaseStats.MaxHealth > 0)
        {
            const double HealthFraction = static_cast<double>(OwningCharacter->Attributes->CurrentHealth) / OwningCharacter->Attributes->BaseStats.MaxHealth;
            const double CriticalThreshold = GetAbilityValueFromKey(TEXT("CriticalHealthThreshold"));
            const double CriticalMultiplier = GetAbilityValueFromKey(TEXT("BloodHealMultiplierWhileCritical"));
            if (CriticalThreshold > 0.0 && HealthFraction <= CriticalThreshold && CriticalMultiplier > 0.0)
            {
                BloodHealing = FMath::RoundToInt(BloodHealing * CriticalMultiplier);
            }
        }
        HealAmount += BloodHealing;
        const double RetainFraction = GetAbilityValueFromKey(TEXT("BloodRetainFraction"));
        if (RetainFraction > 0.0) Character5->GainRawBlood(FMath::RoundToInt(ConsumedBlood * RetainFraction));
    }
    else
    {
        HealAmount += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("Heal"))));
    }

    int32 ActualHealing = 0;
    if (HealAmount > 0)
    {
        ActualHealing = OwningCharacter->Heal(HealAmount);
    }

    if (GetAbilityValueFromKey(TEXT("CleanseOneDebuff")) > 0.0)
    {
        const TArray<TObjectPtr<ARoundEffectActor>> Effects = OwningCharacter->ActiveEffects;
        for (ARoundEffectActor* Effect : Effects)
        {
            if (Effect && Effect->IsDebuff()) { Effect->RemoveEffect(); break; }
        }
    }

    const int32 Duration = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("EffectDurationInitiative"))));
    const double GuardMultiplier = GetAbilityValueFromKey(TEXT("IncomingDamageMultiplierAfterUse"));
    const double DamageMultiplier = GetAbilityValueFromKey(TEXT("OutgoingDamageMultiplierAfterUse"));
    const double Lifesteal = GetAbilityValueFromKey(TEXT("LifestealFractionAfterUse"));
    if (Duration > 0 && (GuardMultiplier > 0.0 || DamageMultiplier > 0.0 || Lifesteal > 0.0))
    {
        if (ARoundModifierEffect* Buff = GetWorld()->SpawnActor<ARoundModifierEffect>())
        {
            Buff->DisplayName = TEXT("Blutrausch"); Buff->IncomingDamageMultiplier = GuardMultiplier > 0.0 ? GuardMultiplier : 1.0;
            Buff->OutgoingDamageMultiplier = DamageMultiplier > 0.0 ? DamageMultiplier : 1.0; Buff->HealFractionFromDamageDealt = Lifesteal;
            Buff->bTreatAsBuff = true; Buff->SetDurationInitiative(Duration, false); OwningCharacter->AddEffect(Buff);
        }
    }

    if (ARoundGameState* GameState = OwningCharacter->GetRoundGameState())
    {
        const int32 InitiativeLoss = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AdjacentInitiativeLoss"))));
        const double DamageFraction = GetAbilityValueFromKey(TEXT("AdjacentDamageFractionOfHealing"));
        for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(OwningCharacter->CurrentTile))
        {
            ARoundCharacter* Enemy = Neighbor ? Neighbor->OccupyingCharacter : nullptr;
            if (!Enemy || !OwningCharacter->IsEnemyFor(Enemy)) continue;
            if (InitiativeLoss > 0) Enemy->ApplyDelay(InitiativeLoss);
            if (DamageFraction > 0.0 && ActualHealing > 0)
            {
                bool bDidDie = false; int32 DefenseIgnore = 0;
                const int32 Damage = OwningCharacter->ModifyOutgoingDamage(FMath::RoundToInt(ActualHealing * DamageFraction), ERoundDamageType::Physical, Enemy, DefenseIgnore);
                Enemy->ApplyRoundDamage(Damage, ERoundDamageType::Physical, DefenseIgnore, OwningCharacter, bDidDie);
            }
        }
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Heal executed.");
    return Result;
}
