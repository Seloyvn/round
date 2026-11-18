#include "Abilities/RoundStasisAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Effects/RoundStasisEffect.h"

FRoundActionResult ARoundStasisAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    if (!OwningCharacter || !TargetCharacter || !GetWorld())
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundStasisEffect* StasisEffect = GetWorld()->SpawnActor<ARoundStasisEffect>(ARoundStasisEffect::StaticClass());
    if (!StasisEffect)
    {
        Result.Reason = TEXT("Failed to create effect.");
        return Result;
    }

    StasisEffect->EffectId = TEXT("Stasis");
    StasisEffect->DisplayName = TEXT("Stasis");
    StasisEffect->Description = TEXT("Das Ziel ist immun und generiert keine Initiative.");
    StasisEffect->CauserCharacter = OwningCharacter;
    StasisEffect->HealOnRelease = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("HealOnRelease"))));
    StasisEffect->InitiativeGainOnRelease = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainOnRelease"))));
    const int32 DurationInitiative = GetAbilityDurationInitiativeFromKey(TEXT("DurationInitiative"));
    StasisEffect->SetDurationInitiative(FMath::Max(1, DurationInitiative > 0 ? DurationInitiative : 60), false);
    TargetCharacter->AddEffect(StasisEffect);
    NotifyAllyAffectedBySpell(TargetCharacter);

    Result.bSuccess = true;
    Result.Reason = TEXT("Stasis applied.");
    return Result;
}
