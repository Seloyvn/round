#include "Abilities/RoundConcentrateAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter2.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundAbilityData.h"
#include "Effects/RoundConcentrateEffect.h"

FRoundActionResult ARoundConcentrateAbility::ExecuteAbility(ARoundGridTile* TargetTile)
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

    if (ARoundConcentrateEffect* ConcentrateEffect = GetWorld()->SpawnActor<ARoundConcentrateEffect>(ARoundConcentrateEffect::StaticClass()))
    {
        ConcentrateEffect->EffectId = TEXT("Concentrate");
        ConcentrateEffect->DisplayName = AbilityData ? AbilityData->DisplayName.ToString() : TEXT("Concentrate");
        ConcentrateEffect->Description = AbilityData ? AbilityData->Description.ToString() : TEXT("Verstaerkt den naechsten Treffer.");
        const double ConfiguredMultiplier = GetAbilityValueFromKey(TEXT("ConcentrateBonusMultiplier"));
        const double DamagePerMissingInitiative = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("DamagePerInitative")));
        const int32 MissingInitiative = FMath::Max(
            0,
            OwningCharacter->Attributes->BaseStats.MaxInitiative - OwningCharacter->Attributes->CurrentInitiative);
        const double InitiativeScalingMultiplier = 1.0 + static_cast<double>(MissingInitiative) * DamagePerMissingInitiative;
        ConcentrateEffect->Amount = ConfiguredMultiplier > 0.0
            ? ConfiguredMultiplier * InitiativeScalingMultiplier
            : InitiativeScalingMultiplier;
        ConcentrateEffect->DefenseReductionOnHit = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseReductionOnHit")));
        ConcentrateEffect->DefenseReductionDuration = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseReductionDurationInitiative")));
        ConcentrateEffect->CurrentInitiativeLossFraction = GetAbilityValueFromKey(TEXT("CurrentInitiativeLossFraction"));
        ConcentrateEffect->bGrantFreeStepAfterHit = GetAbilityValueFromKey(TEXT("GrantFreeStepAfterHit")) > 0.0;
        ConcentrateEffect->bBasicAttackConsumesNoAmmo = GetAbilityValueFromKey(TEXT("BasicAttackConsumesNoAmmo")) > 0.0;
        ConcentrateEffect->AbilityCooldownReductionOnConsume = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AbilityCooldownReductionOnConsume")));
        ConcentrateEffect->SplashDamageFraction = GetAbilityValueFromKey(TEXT("SplashDamageFraction"));
        ConcentrateEffect->bPersistOnKill = GetAbilityValueFromKey(TEXT("PersistOnKill")) > 0.0;
        ConcentrateEffect->bDumpMagazineAfterBasic = GetAbilityValueFromKey(TEXT("DumpMagazineAfterBasic")) > 0.0;
        ConcentrateEffect->RemainingEmpoweredHits = GetAbilityValueFromKey(TEXT("EmpoweredHitCount")) > 0.0 ? FMath::RoundToInt(GetAbilityValueFromKey(TEXT("EmpoweredHitCount"))) : 1;
        ConcentrateEffect->SecondHitBonusFraction = GetAbilityValueFromKey(TEXT("SecondHitBonusFraction"));
        ConcentrateEffect->bExpireOnMove = GetAbilityValueFromKey(TEXT("ExpireOnMove")) > 0.0;
        ConcentrateEffect->bExpireOnDamage = GetAbilityValueFromKey(TEXT("ExpireOnDamage")) > 0.0;
        ConcentrateEffect->bBlockAmmoRegeneration = GetAbilityValueFromKey(TEXT("BlockAmmoRegeneration")) > 0.0;
        ConcentrateEffect->ChargeBonusPerInterval = GetAbilityValueFromKey(TEXT("ChargeBonusPerInterval"));
        ConcentrateEffect->MaxChargeBonus = GetAbilityValueFromKey(TEXT("MaxChargeBonus"));
        ConcentrateEffect->ChargeInterval = FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ChargeInterval"))));
        if (ARoundCharacter2* Character2 = Cast<ARoundCharacter2>(OwningCharacter); Character2 && Character2->Ammo == 0 && GetAbilityValueFromKey(TEXT("ReloadAmmoAfterDelay")) > 0.0)
            ConcentrateEffect->PendingAmmoReload = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ReloadAmmoAfterDelay")));
        ConcentrateEffect->CauserCharacter = OwningCharacter;
        const bool bUnlimitedUntilMove = GetAbilityValueFromKey(TEXT("UnlimitedUntilMove")) > 0.0;
        if (bUnlimitedUntilMove) ConcentrateEffect->SetDurationInitiative(0, false);
        else ConcentrateEffect->SetDurationInitiative(40, false);
        OwningCharacter->AddEffect(ConcentrateEffect);
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Concentrate executed.");
    return Result;
}
