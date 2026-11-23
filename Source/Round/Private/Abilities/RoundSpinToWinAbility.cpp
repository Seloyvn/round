#include "Abilities/RoundSpinToWinAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter5.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundModifierEffect.h"
#include "Effects/RoundRendEffect.h"
#include "Effects/RoundSpinEchoEffect.h"
#include "Game/RoundGameState.h"

void ARoundSpinToWinAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    if (!OwningCharacter || !OwningCharacter->CurrentTile)
    {
        return;
    }

    ARoundGameState* RoundGameState = OwningCharacter->GetRoundGameState();
    if (!RoundGameState)
    {
        return;
    }

    for (ARoundGridTile* Neighbor : RoundGameState->GetFields())
    {
        if (Neighbor && OwningCharacter->CurrentTile->GetDistance(Neighbor) <= FMath::Max(1, GetAbilityRange()))
        {
            OutTiles.AddUnique(Neighbor);
        }
    }
}

FRoundActionResult ARoundSpinToWinAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 28)
    {
        ARoundGridTile* CoreTargetTile = OwningCharacter ? OwningCharacter->CurrentTile.Get() : TargetTile;
        return ARoundAbilityActor::ExecuteAbility(CoreTargetTile);
    }
    FRoundActionResult Result;
    if (!OwningCharacter || !OwningCharacter->Attributes || !OwningCharacter->CurrentTile)
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
    int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));

    TArray<ARoundGridTile*> AffectedTiles;
    GetPreviewTiles(OwningCharacter->CurrentTile, AffectedTiles);
    int32 HitCount = 0;
    int32 TotalAppliedDamage = 0;
    for (ARoundGridTile* Tile : AffectedTiles)
    {
        ARoundCharacter* TargetCharacter = Tile ? Tile->OccupyingCharacter : nullptr;
        if (!TargetCharacter || !OwningCharacter->IsEnemyFor(TargetCharacter))
        {
            continue;
        }

        bool bDidDie = false;
        int32 Damage = GetAbilityDamage();
        if (TargetCharacter->Attributes && TargetCharacter->Attributes->BaseStats.MaxHealth > 0)
        {
            const double Threshold = GetAbilityValueFromKey(TEXT("LowHealthThreshold"));
            const double Fraction = static_cast<double>(TargetCharacter->Attributes->CurrentHealth) / TargetCharacter->Attributes->BaseStats.MaxHealth;
            const double Multiplier = GetAbilityValueFromKey(TEXT("DamageMultiplierVsLowHealth"));
            if (Threshold > 0.0 && Fraction <= Threshold && Multiplier > 0.0) Damage = FMath::RoundToInt(Damage * Multiplier);
        }
        const int32 FinalDamage = OwningCharacter->ModifyOutgoingDamage(Damage, GetAbilityDamageType(), TargetCharacter, DefenseIgnore);
        const int32 AppliedDamage = TargetCharacter->ApplyRoundDamage(FinalDamage, GetAbilityDamageType(), DefenseIgnore, OwningCharacter, bDidDie);
        ++HitCount; TotalAppliedDamage += AppliedDamage;

        const int32 BleedDamage = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BleedDamagePerTurn"))));
        if (!bDidDie && BleedDamage > 0)
        {
            if (ARoundRendEffect* Bleed = GetWorld()->SpawnActor<ARoundRendEffect>())
            {
                Bleed->DisplayName = TEXT("Aufgerissen"); Bleed->Amount = BleedDamage; Bleed->CauserCharacter = OwningCharacter;
                Bleed->SetDurationInitiative(FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BleedDurationInitiative")))), false); TargetCharacter->AddEffect(Bleed);
            }
        }
    }

    if (ARoundCharacter5* Character5 = Cast<ARoundCharacter5>(OwningCharacter))
    {
        const double BonusFraction = GetAbilityValueFromKey(TEXT("BonusBloodFractionOfDamage"));
        if (BonusFraction > 0.0) Character5->GainRawBlood(FMath::RoundToInt(TotalAppliedDamage * BonusFraction));
        if (HitCount >= FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RedMistRequiredTargets")))))
        {
            Character5->GainRawBlood(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RedMistBloodGain")))));
            OwningCharacter->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RedMistInitiativeGain")))));
        }
    }

    const int32 HealPerTarget = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("HealPerTarget"))));
    if (HealPerTarget > 0) OwningCharacter->Heal(HealPerTarget * HitCount);
    const int32 InitiativePerExtraTarget = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativePerExtraTarget"))));
    if (InitiativePerExtraTarget > 0 && HitCount > 1) OwningCharacter->Attributes->GainInitiative(FMath::Min(20, (HitCount - 1) * InitiativePerExtraTarget));

    const double GuardMultiplier = GetAbilityValueFromKey(TEXT("SelfIncomingDamageMultiplierIfMultiHit"));
    if (HitCount >= 2 && GuardMultiplier > 0.0)
    {
        if (ARoundModifierEffect* Guard = GetWorld()->SpawnActor<ARoundModifierEffect>())
        {
            Guard->DisplayName = TEXT("Mitten im Gemetzel"); Guard->IncomingDamageMultiplier = GuardMultiplier; Guard->bTreatAsBuff = true;
            Guard->SetDurationInitiative(FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("GuardDurationInitiative")))), false); OwningCharacter->AddEffect(Guard);
        }
    }

    const int32 ReductionPerTarget = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionPerTarget"))));
    if (ReductionPerTarget > 0) ReduceCooldown(FMath::Min(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MaxCooldownReduction")))), HitCount * ReductionPerTarget));

    const double EchoMultiplier = GetAbilityValueFromKey(TEXT("EchoDamageMultiplier"));
    if (EchoMultiplier > 0.0)
    {
        if (ARoundSpinEchoEffect* Echo = GetWorld()->SpawnActor<ARoundSpinEchoEffect>())
        {
            Echo->DisplayName = TEXT("Doppelwirbel"); Echo->Damage = FMath::RoundToInt(GetAbilityDamage() * EchoMultiplier); Echo->Radius = FMath::Max(1, GetAbilityRange());
            Echo->CauserCharacter = OwningCharacter; Echo->SetDurationInitiative(FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("EchoDelayInitiative")))), false); OwningCharacter->AddEffect(Echo);
        }
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Spin To Win executed.");
    return Result;
}

// Runtime execution lives exclusively in the sim.
