#include "Abilities/RoundAccelerateAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Game/RoundGameState.h"

FRoundActionResult ARoundAccelerateAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    if (!OwningCharacter || !TargetCharacter)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    const int32 CooldownReductionValue = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReduction")));
    const int32 CooldownReduction = FMath::Max(0, CooldownReductionValue > 0 ? CooldownReductionValue : 40);
    auto ApplyAcceleration = [&](ARoundCharacter* Character)
    {
        if (!Character) return 0;
        int32 Count = 0;
        ARoundAbilityActor* HighestCooldownAbility = nullptr;
        for (ARoundAbilityActor* AbilityActor : Character->GrantedAbilities)
        {
            if (!AbilityActor) continue;
            if (!HighestCooldownAbility || AbilityActor->CurrentCooldown > HighestCooldownAbility->CurrentCooldown) HighestCooldownAbility = AbilityActor;
            const int32 Before = AbilityActor->CurrentCooldown;
            AbilityActor->ReduceCooldown(CooldownReduction);
            if (AbilityActor->CurrentCooldown < Before) ++Count;
        }
        if (HighestCooldownAbility)
        {
            HighestCooldownAbility->ReduceCooldown(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ExtraReductionToHighestCooldown")))));
        }
        if (Character->Attributes) Character->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("TargetInitiativeGain")))));
        return Count;
    };

    int32 ReducedCount = ApplyAcceleration(TargetCharacter);
    if (GetAbilityValueFromKey(TEXT("AffectAdjacentAllies")) > 0.0)
    {
        if (ARoundGameState* GameState = OwningCharacter->GetRoundGameState())
        {
            for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(TargetTile))
            {
                ARoundCharacter* Other = Neighbor ? Neighbor->GetResolvedOccupyingCharacter() : nullptr;
                if (Other && !OwningCharacter->IsEnemyFor(Other)) ReducedCount += ApplyAcceleration(Other);
            }
        }
    }
    if (TargetCharacter != OwningCharacter && GetAbilityValueFromKey(TEXT("AccelerateSelf")) > 0.0) ReducedCount += ApplyAcceleration(OwningCharacter);

    NotifyAllyAffectedBySpell(TargetCharacter);
    Result.bSuccess = true;
    Result.Reason = FString::Printf(TEXT("Accelerate reduced %d cooldown(s)."), ReducedCount);
    return Result;
}
