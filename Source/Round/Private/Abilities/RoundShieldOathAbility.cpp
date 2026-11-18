#include "Abilities/RoundShieldOathAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundShieldOathEffect.h"

FRoundActionResult ARoundShieldOathAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
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

    ARoundShieldOathEffect* Effect = OwningCharacter->GetWorld() ? OwningCharacter->GetWorld()->SpawnActor<ARoundShieldOathEffect>(ARoundShieldOathEffect::StaticClass()) : nullptr;
    if (!Effect)
    {
        Result.Reason = TEXT("Failed to create shield oath effect.");
        return Result;
    }

    Effect->EffectId = TEXT("ShieldOath");
    Effect->DisplayName = TEXT("Shield Oath");
    Effect->Description = TEXT("Leitet einen Teil des Schadens auf den Paladin um.");
    Effect->RedirectPreMitigationFraction = GetAbilityValueFromKey(TEXT("RedirectPreMitigationFraction")) > 0.0
        ? GetAbilityValueFromKey(TEXT("RedirectPreMitigationFraction"))
        : GetAbilityValueFromKey(TEXT("RedirectDamageFraction"));
    Effect->RedirectedDamageMultiplier = FMath::Max(0.0, 1.0 + GetAbilityValueFromKey(TEXT("RedirectedDamageMultiplierAdd")));
    Effect->PaladinHealFraction = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("PaladinHealFractionFromRedirect")));
    Effect->ProtectedAllyInitiativePerHit = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ProtectedAllyInitiativePerHit"))));
    Effect->SetDurationInitiative(FMath::Max(1, GetAbilityDurationInitiativeFromKey(TEXT("DurationInitiative"))), false);
    Effect->CauserCharacter = OwningCharacter;
    TargetCharacter->AddEffect(Effect);

    Result.bSuccess = true;
    Result.Reason = TEXT("Shield Oath applied.");
    return Result;
}
