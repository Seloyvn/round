#include "Abilities/RoundAimAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter2.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Game/RoundGameState.h"
#include "Triggers/RoundAimFieldTriggerActor.h"
#include "EngineUtils.h"

namespace
{
void BuildAimTiles(const ARoundGameState* GameState, ARoundGridTile* CenterTile, const int32 Radius, TArray<ARoundGridTile*>& OutTiles)
{
    if (!GameState || !CenterTile)
    {
        return;
    }

    TSet<ARoundGridTile*> Visited;
    TArray<TPair<ARoundGridTile*, int32>> Frontier;
    Frontier.Add({CenterTile, 0});
    Visited.Add(CenterTile);

    while (Frontier.Num() > 0)
    {
        const TPair<ARoundGridTile*, int32> Current = Frontier[0];
        Frontier.RemoveAt(0);
        OutTiles.AddUnique(Current.Key);

        if (Current.Value >= Radius)
        {
            continue;
        }

        for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(Current.Key))
        {
            if (Neighbor && !Visited.Contains(Neighbor))
            {
                Visited.Add(Neighbor);
                Frontier.Add({Neighbor, Current.Value + 1});
            }
        }
    }
}
}

void ARoundAimAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    if (!TargetTile || !OwningCharacter)
    {
        return;
    }

    BuildAimTiles(OwningCharacter->GetRoundGameState(), const_cast<ARoundGridTile*>(TargetTile), FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DamageRange")))), OutTiles);
}

FRoundActionResult ARoundAimAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 13)
    {
        return ARoundAbilityActor::ExecuteAbility(TargetTile);
    }
    FRoundActionResult Result;
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundCharacter2* Character2 = Cast<ARoundCharacter2>(OwningCharacter);
    const bool bConsumesAmmo = GetAbilityValueFromKey(TEXT("AmmoCost")) > 0.0;
    if (bConsumesAmmo && (!Character2 || !Character2->HasAmmo()))
    {
        Result.Reason = TEXT("No ammo.");
        return Result;
    }

    if (!ConsumeAbilityActivation(TargetTile, Result)) return Result;

    TArray<ARoundGridTile*> TriggerTiles;
    GetPreviewTiles(TargetTile, TriggerTiles);
    if (GetAbilityMaxCharges() > 1)
    {
        TArray<ARoundAimFieldTriggerActor*> ExistingFields;
        for (TActorIterator<ARoundAimFieldTriggerActor> It(GetWorld()); It; ++It)
            if (It->CauserCharacter == OwningCharacter) ExistingFields.Add(*It);
        while (ExistingFields.Num() >= 2)
        {
            ExistingFields[0]->RemoveTrigger();
            ExistingFields.RemoveAt(0);
        }
    }
    if (ARoundAimFieldTriggerActor* TriggerActor = GetWorld()->SpawnActor<ARoundAimFieldTriggerActor>(ARoundAimFieldTriggerActor::StaticClass()))
    {
        const double TriggerMultiplier = GetAbilityValueFromKey(TEXT("TriggerDamageMultiplier"));
        TriggerActor->TriggerDamage = FMath::RoundToInt(GetAbilityDamage() * (TriggerMultiplier > 0.0 ? TriggerMultiplier : 1.0));
        TriggerActor->DamageType = GetAbilityDamageType();
        TriggerActor->TriggerInitiativeLoss = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("TriggerInitiativeLoss"))));
        TriggerActor->AoEInitiativeLoss = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AoEInitiativeLossOnTrigger"))));
        TriggerActor->PersistTriggerCount = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("PersistTriggerCount"))));
        TriggerActor->bBlockMovementBonusesOnTrigger = GetAbilityValueFromKey(TEXT("BlockMovementBonusesOnTrigger")) > 0.0;
        TriggerActor->bOwnerDamageBonusOnTriggeredTarget = GetAbilityValueFromKey(TEXT("OwnerDamageWindowOnTriggeredTarget")) > 0.0;
        TriggerActor->bExtraDamageIfTargetMoved = GetAbilityValueFromKey(TEXT("ExtraDamageIfTargetMoved")) > 0.0;
        const double LowHealthMultiplier = GetAbilityValueFromKey(TEXT("DamageMultiplierVsLowHealth"));
        TriggerActor->DamageMultiplierVsLowHealth = LowHealthMultiplier > 0.0 ? LowHealthMultiplier : 1.0;
        TriggerActor->DefenseIgnore = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore")));
        TriggerActor->NextOutgoingDamageMultiplier = GetAbilityValueFromKey(TEXT("NextOutgoingDamageMultiplier"));
        TriggerActor->MarkDamageMultiplier = GetAbilityValueFromKey(TEXT("MarkDamageMultiplier"));
        TriggerActor->MarkDurationInitiative = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MarkDurationInitiative")));
        TriggerActor->bBonusIfOwnerStayed = GetAbilityValueFromKey(TEXT("BonusIfOwnerStayed")) > 0.0;
        TriggerActor->StayedDamageMultiplier = GetAbilityValueFromKey(TEXT("StayedDamageMultiplier"));
        TriggerActor->ExtendDebuffDurationInitiative = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ExtendDebuffDurationInitiative")));
        TriggerActor->AmmoOnKill = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AmmoOnKill")));
        TriggerActor->CooldownReductionOnKill = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionOnKill")));
        TriggerActor->bCoverFire = GetAbilityValueFromKey(TEXT("CoverFire")) > 0.0;
        TriggerActor->bApplyTrackingMark = GetAbilityValueFromKey(TEXT("ApplyTrackingMark")) > 0.0;
        TriggerActor->OwnerPlacementTile = OwningCharacter->CurrentTile;
        TriggerActor->InitializeTrigger(OwningCharacter, TriggerTiles);
    }

    if (bConsumesAmmo) Character2->ConsumeAmmo(FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AmmoCost"))));

    Result.bSuccess = true;
    Result.Reason = TEXT("Aim executed.");
    return Result;
}
