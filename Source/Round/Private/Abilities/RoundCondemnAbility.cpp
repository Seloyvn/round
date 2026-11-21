#include "Abilities/RoundCondemnAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacterPaladin.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundPaladinAuraEffect.h"
#include "Effects/RoundSilenceEffect.h"
#include "Game/RoundGameState.h"

bool ARoundCondemnAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    if (!Super::CanTargetTile(TargetTile) || !OwningCharacter)
    {
        return false;
    }

    const ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    if (!TargetCharacter)
    {
        return false;
    }

    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 1)
    {
        const ARoundGameState* GameState = OwningCharacter->GetRoundGameState();
        return GameState && GameState->IsCoreAbilityLegal(OwningCharacter, this, TargetCharacter);
    }

    const ARoundCharacterPaladin* Paladin = Cast<ARoundCharacterPaladin>(OwningCharacter);
    if (!Paladin)
    {
        return false;
    }
    const int32 WindowInitiative = FMath::Max(1, GetAbilityDurationInitiativeFromKey(TEXT("RecentAllyDamageWindowInitiative")));
    const ARoundGameState* GameState = OwningCharacter->GetRoundGameState();
    for (ARoundEffectActor* Effect : OwningCharacter->ActiveEffects)
    {
        if (const ARoundPaladinAuraEffect* AuraEffect = Cast<ARoundPaladinAuraEffect>(Effect))
        {
            if (AuraEffect->HasRecentPreventedDamageFrom(TargetCharacter, WindowInitiative))
            {
                return true;
            }
        }
    }

    return false;
}

FRoundActionResult ARoundCondemnAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 1)
    {
        return ARoundAbilityActor::ExecuteAbility(TargetTile);
    }

    FRoundActionResult Result;
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    if (!TargetCharacter)
    {
        Result.Reason = TEXT("No target.");
        return Result;
    }

    if (!OwningCharacter->Attributes->SpendInitiative(GetAbilityCost()))
    {
        Result.Reason = TEXT("Not enough initiative.");
        return Result;
    }

    SetCooldown(GetAbilityCooldown());

    if (GetAbilityDamage() > 0)
    {
        int32 DefenseIgnore = 0;
        int32 Damage = OwningCharacter->ModifyOutgoingDamage(GetAbilityDamage(), GetAbilityDamageType(), TargetCharacter, DefenseIgnore);
        bool bDidDie = false;
        TargetCharacter->ApplyRoundDamage(Damage, GetAbilityDamageType(), DefenseIgnore, OwningCharacter, bDidDie);
    }

    ARoundSilenceEffect* SilenceEffect = OwningCharacter->GetWorld() ? OwningCharacter->GetWorld()->SpawnActor<ARoundSilenceEffect>(ARoundSilenceEffect::StaticClass()) : nullptr;
    if (SilenceEffect)
    {
        SilenceEffect->EffectId = TEXT("CondemnSilence");
        SilenceEffect->DisplayName = TEXT("Condemned");
        SilenceEffect->Description = TEXT("Kann keine Skills einsetzen.");
        SilenceEffect->SetDurationInitiative(FMath::Max(1, GetAbilityDurationInitiativeFromKey(TEXT("SilenceDurationInitiative"))), false);
        SilenceEffect->CauserCharacter = OwningCharacter;
        TargetCharacter->AddEffect(SilenceEffect);
    }

    OwningCharacter->Heal(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("SelfHealOnCondemn")))));
    OwningCharacter->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainOnCondemn")))));

    Result.bSuccess = true;
    Result.Reason = TEXT("Condemn applied.");
    return Result;
}
