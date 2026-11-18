#include "Abilities/RoundHookShotAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter3.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundInvisibilityEffect.h"

ERoundButtonState ARoundHookShotAbility::GetAbilityVisibility() const
{
    if (AnchorTile.IsValid() && OwningCharacter && !OwningCharacter->HasTurnActionBlock())
    {
        return ERoundButtonState::Enabled;
    }
    return Super::GetAbilityVisibility();
}

bool ARoundHookShotAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    if (!OwningCharacter || !OwningCharacter->CurrentTile || !TargetTile || TargetTile == OwningCharacter->CurrentTile)
    {
        return false;
    }

    if (AnchorTile.IsValid())
    {
        return TargetTile == AnchorTile.Get() && TargetTile->IsWalkable();
    }

    if (Super::GetAbilityVisibility() != ERoundButtonState::Enabled || !IsTargetInRange(TargetTile))
    {
        return false;
    }

    if (!TargetTile->OccupyingCharacter)
    {
        return TargetTile->IsWalkable();
    }

    return GetAbilityValueFromKey(TEXT("CanSwapWithAlly")) > 0.0
        && TargetTile->OccupyingCharacter != OwningCharacter
        && !OwningCharacter->IsEnemyFor(TargetTile->OccupyingCharacter);
}

FRoundActionResult ARoundHookShotAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes || !TargetTile || !OwningCharacter->CurrentTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    if (AnchorTile.IsValid())
    {
        OwningCharacter->SetField(TargetTile, true);
        AnchorTile.Reset();
        Result.bSuccess = true;
        Result.Reason = TEXT("Returned to Hook Shot anchor.");
        return Result;
    }

    ARoundGridTile* OriginTile = OwningCharacter->CurrentTile;
    ARoundCharacter* SwapAlly = TargetTile->OccupyingCharacter;
    const int32 TravelDistance = OriginTile->GetDistance(TargetTile);

    if (!ConsumeAbilityActivation(TargetTile, Result))
    {
        return Result;
    }

    if (SwapAlly)
    {
        // Free the caster's origin before swapping. SetField itself rejects
        // every destination that is still occupied by another character.
        if (OriginTile->OccupyingCharacter == OwningCharacter)
        {
            OriginTile->OccupyingCharacter = nullptr;
            OriginTile->ForceNetUpdate();
        }
        SwapAlly->SetField(OriginTile, true);
    }
    OwningCharacter->SetField(TargetTile, true);

    ARoundCharacter3* Assassin = Cast<ARoundCharacter3>(OwningCharacter);
    if (Assassin && GetAbilityValueFromKey(TEXT("GrantFreeAdjacentMove")) > 0.0) Assassin->GrantShadowstrikeFreeStep();
    if (Assassin && TravelDistance >= FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ComboDistanceThreshold")))))
    {
        Assassin->TryGainComboPoints(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ComboGainAtDistance")))));
    }
    if (Assassin) Assassin->GrantShadowstrikeDamageReduction(GetAbilityValueFromKey(TEXT("NextIncomingDamageMultiplier")));
    if (Assassin) Assassin->GrantHookShotBasicAttackCostReduction(FMath::RoundToInt(GetAbilityValueFromKey(TEXT("NextBasicAttackCostReduction"))));

    const int32 InvisibilityDuration = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InvisibilityDurationInitiative"))));
    if (InvisibilityDuration > 0 && GetWorld())
    {
        ARoundInvisibilityEffect* Effect = GetWorld()->SpawnActor<ARoundInvisibilityEffect>();
        if (Effect)
        {
            Effect->EffectId = TEXT("HookShotInvisibility");
            Effect->DisplayName = TEXT("Lautlose Ankunft");
            Effect->Description = TEXT("Unsichtbar bis Character 3 Schaden verursacht.");
            Effect->SetDurationInitiative(InvisibilityDuration, false);
            Effect->CauserCharacter = OwningCharacter;
            OwningCharacter->AddEffect(Effect);
        }
    }

    if (GetAbilityValueFromKey(TEXT("CreateReturnAnchor")) > 0.0) AnchorTile = OriginTile;
    bKillResetArmed = GetAbilityValueFromKey(TEXT("ResetCooldownOnKillAfterUse")) > 0.0;

    Result.bSuccess = true;
    Result.Reason = TEXT("Hook Shot executed.");
    return Result;
}

void ARoundHookShotAbility::OnOwnerTurnStarted()
{
    AnchorTile.Reset();
    bKillResetArmed = false;
    bKillResetUsedThisTurn = false;
}

void ARoundHookShotAbility::OnOwnerTurnEnded()
{
    AnchorTile.Reset();
    bKillResetArmed = false;
}

void ARoundHookShotAbility::NotifyOwnerKilledEnemy()
{
    if (bKillResetArmed && !bKillResetUsedThisTurn)
    {
        SetCooldown(0);
        if (UsesCharges()) CurrentCharges = GetAbilityMaxCharges();
        bKillResetArmed = false;
        bKillResetUsedThisTurn = true;
    }
}
