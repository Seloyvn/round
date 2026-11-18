#include "Abilities/RoundTemporalAnchorAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Effects/RoundTemporalAnchorEffect.h"

FRoundActionResult ARoundTemporalAnchorAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    if (!OwningCharacter || !TargetCharacter || !GetWorld())
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundTemporalAnchorEffect* AnchorEffect = GetWorld()->SpawnActor<ARoundTemporalAnchorEffect>(ARoundTemporalAnchorEffect::StaticClass());
    if (!AnchorEffect)
    {
        Result.Reason = TEXT("Failed to create effect.");
        return Result;
    }

    AnchorEffect->EffectId = TEXT("TemporalAnchor");
    AnchorEffect->DisplayName = TEXT("Temporal Anchor");
    AnchorEffect->Description = TEXT("Bringt das Ziel spaeter an die gespeicherte Position zurueck.");
    AnchorEffect->CauserCharacter = OwningCharacter;
    AnchorEffect->HealOnReturn = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("HealOnReturn"))));
    AnchorEffect->InitiativeGainOnReturn = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainOnReturn"))));
    AnchorEffect->NearbyEnemyInitiativeLoss = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("NearbyEnemyInitiativeLossOnReturn"))));
    const int32 DurationInitiative = GetAbilityDurationInitiativeFromKey(TEXT("DurationInitiative"));
    AnchorEffect->SetDurationInitiative(FMath::Max(1, DurationInitiative > 0 ? DurationInitiative : 80), false);
    TargetCharacter->AddEffect(AnchorEffect);
    NotifyAllyAffectedBySpell(TargetCharacter);

    Result.bSuccess = true;
    Result.Reason = TEXT("Temporal Anchor applied.");
    return Result;
}
