#include "Abilities/RoundCleanseAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Actors/RoundEffectActor.h"
#include "Components/RoundAttributeSetComponent.h"

FRoundActionResult ARoundCleanseAbility::ExecuteAbility(ARoundGridTile* TargetTile)
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

    TArray<ARoundEffectActor*> RemovableDebuffs;
    for (ARoundEffectActor* Effect : TargetCharacter->ActiveEffects)
    {
        if (Effect && Effect->IsDebuff() && Effect->IsRemovable())
        {
            RemovableDebuffs.Add(Effect);
        }
    }

    const int32 RemoveCount = FMath::Max(1, 1 + FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AdditionalDebuffsRemoved"))));
    int32 RemovedCount = 0;
    while (RemovableDebuffs.Num() > 0 && RemovedCount < RemoveCount)
    {
        const int32 RandomIndex = FMath::RandRange(0, RemovableDebuffs.Num() - 1);
        if (ARoundEffectActor* RemovedEffect = RemovableDebuffs[RandomIndex])
        {
            RemovedEffect->RemoveEffect();
            ++RemovedCount;
        }
        RemovableDebuffs.RemoveAt(RandomIndex);
    }

    const int32 HealPerRemoved = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("HealPerDebuffRemoved"))));
    if (HealPerRemoved > 0 && RemovedCount > 0) TargetCharacter->Heal(HealPerRemoved * RemovedCount);
    if (TargetCharacter->Attributes && RemovedCount > 0) TargetCharacter->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainOnCleanse")))));

    NotifyAllyAffectedBySpell(TargetCharacter);
    Result.bSuccess = true;
    Result.Reason = RemovedCount > 0 ? TEXT("Cleanse executed.") : TEXT("Cleanse applied, but no removable debuff was found.");
    return Result;
}
