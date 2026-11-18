#include "Abilities/RoundDragoonExecuteDiveAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacterDragoon.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Game/RoundGameState.h"

bool ARoundDragoonExecuteDiveAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    return OwningCharacter
        && OwningCharacter->CurrentTile
        && TargetTile
        && ResolveTargetCharacter(TargetTile)
        && OwningCharacter->IsEnemyFor(ResolveTargetCharacter(TargetTile))
        && IsTargetInRange(TargetTile)
        && GetAbilityVisibility() == ERoundButtonState::Enabled;
}

FRoundActionResult ARoundDragoonExecuteDiveAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterDragoon* Dragoon = Cast<ARoundCharacterDragoon>(OwningCharacter);
    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    ARoundGameState* GameState = OwningCharacter ? OwningCharacter->GetRoundGameState() : nullptr;
    if (!Dragoon || !TargetCharacter || !TargetCharacter->Attributes || !TargetCharacter->CurrentTile || !GameState)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundGridTile* DestinationTile = GameState->GetCloseFreeFieldFrom(TargetCharacter->CurrentTile, Dragoon->CurrentTile);
    if (!DestinationTile)
    {
        Result.Reason = TEXT("No free tile near target.");
        return Result;
    }

    Dragoon->SetField(DestinationTile, true);

    int32 BaseDamage = GetAbilityDamage();
    const double LowHealthThresholdPercent = GetAbilityValueFromKey(TEXT("LowHealthThresholdPercent"));
    const double DamageMultiplierVsLowHealth = GetAbilityValueFromKey(TEXT("DamageMultiplierVsLowHealth"));
    const double CurrentHealthPercent = TargetCharacter->Attributes->BaseStats.MaxHealth > 0
        ? static_cast<double>(TargetCharacter->Attributes->CurrentHealth) / static_cast<double>(TargetCharacter->Attributes->BaseStats.MaxHealth)
        : 1.0;
    if (LowHealthThresholdPercent > 0.0
        && CurrentHealthPercent <= (LowHealthThresholdPercent <= 1.0 ? LowHealthThresholdPercent : LowHealthThresholdPercent / 100.0)
        && DamageMultiplierVsLowHealth > 0.0)
    {
        BaseDamage = FMath::RoundToInt(static_cast<double>(BaseDamage) * DamageMultiplierVsLowHealth);
    }

    const int32 MomentumBeforeHit = Dragoon->GetMomentumStacks();
    BaseDamage += MomentumBeforeHit * FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BonusDamagePerMomentumConsumed"))));
    bool bDidDie = false;
    const int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    const int32 AppliedDamage = ApplyAbilityDamageToTarget(TargetCharacter, BaseDamage, DefenseIgnore, bDidDie);
    Dragoon->RecordEnemyHit(TargetCharacter, AppliedDamage, GetAbilityDamageType());
    if (GetAbilityValueFromKey(TEXT("ConsumeMomentum")) > 0.0) Dragoon->ResetMomentumStacks();
    if (bDidDie)
    {
        if (Dragoon->Attributes) Dragoon->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainOnKill")))));
        ReduceCooldown(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionOnKill")))));
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Execute Dive executed.");
    return Result;
}
