#include "Abilities/RoundShadowstrikeAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter3.h"
#include "Actors/RoundEffectActor.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundAbilityData.h"
#include "Effects/RoundShadowWoundEffect.h"
#include "Game/RoundGameState.h"
#include "EngineUtils.h"

namespace
{
bool HasDebuff(const ARoundCharacter* Character)
{
    if (!Character) return false;
    for (const ARoundEffectActor* Effect : Character->ActiveEffects)
    {
        if (Effect && Effect->IsDebuff()) return true;
    }
    return false;
}
}

bool ARoundShadowstrikeAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    if (!AbilityData || !OwningCharacter || !OwningCharacter->CurrentTile || !TargetTile
        || GetAbilityVisibility() != ERoundButtonState::Enabled || !MatchesFieldState(TargetTile))
    {
        return false;
    }

    int32 Range = GetAbilityRange();
    if (HasDebuff(TargetTile->OccupyingCharacter))
    {
        Range += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RangeVsDebuffed"))));
    }
    return OwningCharacter->CurrentTile->GetDistance(TargetTile) <= Range;
}

FRoundActionResult ARoundShadowstrikeAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    ARoundCharacter3* Assassin = Cast<ARoundCharacter3>(OwningCharacter);
    ARoundGameState* RoundGameState = OwningCharacter->GetRoundGameState();
    if (!TargetCharacter || !Assassin || !RoundGameState)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundGridTile* DestinationTile = nullptr;
    if (GetAbilityValueFromKey(TEXT("LandOppositeTarget")) > 0.0)
    {
        int32 BestDistance = -1;
        for (ARoundGridTile* Candidate : RoundGameState->GetNeighborFields(TargetTile))
        {
            if (!Candidate || !Candidate->IsMoveable() || Candidate->OccupyingCharacter) continue;
            const int32 DistanceFromOrigin = OwningCharacter->CurrentTile->GetDistance(Candidate);
            if (DistanceFromOrigin > BestDistance)
            {
                BestDistance = DistanceFromOrigin;
                DestinationTile = Candidate;
            }
        }
    }
    if (!DestinationTile)
    {
        DestinationTile = RoundGameState->GetCloseFreeFieldFrom(TargetTile, OwningCharacter->CurrentTile);
    }
    if (!DestinationTile)
    {
        Result.Reason = TEXT("No free tile near target.");
        return Result;
    }

    if (!OwningCharacter->Attributes->SpendInitiative(GetAbilityCost()))
    {
        Result.Reason = TEXT("Not enough initiative.");
        return Result;
    }

    const int32 ComboBefore = Assassin->TryGetComboPoints();
    SetCooldown(GetAbilityCooldown());
    OwningCharacter->SetField(DestinationTile, true);

    bool bDidDie = false;
    int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    DefenseIgnore += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnoreOppositeLanding"))));
    const int32 FinalDamage = OwningCharacter->ModifyOutgoingDamage(GetAbilityDamage(), GetAbilityDamageType(), TargetCharacter, DefenseIgnore);
    const int32 AppliedDamage = TargetCharacter->ApplyRoundDamage(FinalDamage, GetAbilityDamageType(), DefenseIgnore, OwningCharacter, bDidDie);

    int32 ComboGain = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ComboPointsGain"))));
    if (ComboBefore == 0) ComboGain += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BonusComboAtZero"))));

    const int32 IsolationRadius = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("IsolationRadius"))));
    bool bIsolated = IsolationRadius > 0;
    if (bIsolated)
    {
        for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
        {
            ARoundCharacter* Other = *It;
            if (Other && Other != TargetCharacter && Other->IsAlive() && OwningCharacter->IsEnemyFor(Other)
                && Other->CurrentTile && TargetCharacter->CurrentTile
                && Other->CurrentTile->GetDistance(TargetCharacter->CurrentTile) <= IsolationRadius)
            {
                bIsolated = false;
                break;
            }
        }
    }
    if (bIsolated) ComboGain += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BonusComboVsIsolated"))));
    Assassin->TryGainComboPoints(ComboGain);

    if (ComboBefore < Assassin->GetMaxComboPoints() && Assassin->TryGetComboPoints() >= Assassin->GetMaxComboPoints())
    {
        const int32 ExecuteReduction = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ExecuteCooldownReductionOnReachMaxCombo"))));
        if (ExecuteReduction > 0)
        {
            for (ARoundAbilityActor* Ability : Assassin->GrantedAbilities)
            {
                if (Ability && Ability->AbilityData && Ability->AbilityData->AbilityId == FName(TEXT("Execute")))
                {
                    Ability->ReduceCooldown(ExecuteReduction);
                    break;
                }
            }
        }
    }

    Assassin->GrantShadowstrikeFollowup(TargetCharacter, FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("FollowupBasicAttackCostReduction")))));
    if (GetAbilityValueFromKey(TEXT("GrantFreeAdjacentMove")) > 0.0) Assassin->GrantShadowstrikeFreeStep();
    const double IncomingMultiplier = GetAbilityValueFromKey(TEXT("NextIncomingDamageMultiplier"));
    if (IncomingMultiplier > 0.0) Assassin->GrantShadowstrikeDamageReduction(IncomingMultiplier);

    if (!bDidDie && AppliedDamage > 0)
    {
        const int32 WoundDuration = GetAbilityDurationInitiativeFromKey(TEXT("ShadowWoundDurationInitiative"));
        if (WoundDuration > 0 && GetWorld())
        {
            ARoundShadowWoundEffect* Wound = GetWorld()->SpawnActor<ARoundShadowWoundEffect>();
            if (Wound)
            {
                Wound->EffectId = TEXT("ShadowstrikeWound");
                Wound->DisplayName = TEXT("Schattenwunde");
                Wound->Description = TEXT("Der naechste Treffer von Character 3 verursacht Bonusschaden und gewaehrt Combo.");
                Wound->BonusDamage = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ShadowWoundBonusDamage"))));
                Wound->ComboGain = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ShadowWoundComboGain"))));
                Wound->CauserCharacter = Assassin;
                Wound->SetDurationInitiative(WoundDuration, false);
                TargetCharacter->AddEffect(Wound);
            }
        }
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Shadowstrike executed.");
    return Result;
}
