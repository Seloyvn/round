#include "Actors/RoundCharacterChronomancer.h"

#include "Actors/RoundAbilityActor.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundAbilityData.h"
#include "Net/UnrealNetwork.h"

void ARoundCharacterChronomancer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundCharacterChronomancer, Fragments);
}

int32 ARoundCharacterChronomancer::GetFragments() const
{
    return FMath::Max(0, Fragments);
}

int32 ARoundCharacterChronomancer::GetMaxFragments() const
{
    const int32 BaseMax = FMath::RoundToInt(GetValueFromKey(TEXT("MaxFragments")));
    const int32 BonusMax = FMath::RoundToInt(GetPassiveValueFromKey(TEXT("MaxFragments")));
    return FMath::Max(1, (BaseMax > 0 ? BaseMax : 5) + BonusMax);
}

void ARoundCharacterChronomancer::BeginTurn()
{
    Super::BeginTurn();
    bUsedFragmentDiscountThisTurn = false;
    bUsedFirstSpendRefundThisTurn = false;
    if (Fragments <= 0)
    {
        GainFragments(FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("FragmentGainAtTurnStartIfEmpty")))));
    }
    if (Fragments >= GetMaxFragments() && Attributes)
    {
        Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("InitiativeGainAtTurnStartIfFull")))));
    }
}

int32 ARoundCharacterChronomancer::GainFragments(int32 Amount)
{
    if (Amount <= 0)
    {
        return Fragments;
    }

    const int32 UnclampedFragments = Fragments + Amount;
    Fragments = FMath::Clamp(UnclampedFragments, 0, GetMaxFragments());
    const int32 Overflow = FMath::Max(0, UnclampedFragments - GetMaxFragments());
    if (Overflow > 0)
    {
        Heal(Overflow * FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("HealPerOverflowFragment")))));
        const int32 CooldownReduction = Overflow * FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("AllCooldownReductionPerOverflowFragment"))));
        if (CooldownReduction > 0)
        {
            for (ARoundAbilityActor* Ability : GrantedAbilities) if (Ability) Ability->ReduceCooldown(CooldownReduction);
        }
    }
    NotifyRoundStateChanged();
    ForceNetUpdate();
    return Fragments;
}

bool ARoundCharacterChronomancer::HasEnoughFragments(int32 Cost) const
{
    int32 RequiredCost = FMath::Max(0, Cost);
    if (!bUsedFragmentDiscountThisTurn)
    {
        RequiredCost = FMath::Max(0, RequiredCost - FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("FirstFragmentSpendDiscount")))));
    }
    return GetFragments() >= RequiredCost;
}

bool ARoundCharacterChronomancer::SpendFragments(int32 Cost)
{
    int32 RequiredCost = FMath::Max(0, Cost);
    if (!bUsedFragmentDiscountThisTurn)
    {
        const int32 Discount = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("FirstFragmentSpendDiscount"))));
        if (Discount > 0)
        {
            RequiredCost = FMath::Max(0, RequiredCost - Discount);
            bUsedFragmentDiscountThisTurn = true;
        }
    }
    if (RequiredCost <= 0)
    {
        return true;
    }

    if (Fragments < RequiredCost)
    {
        return false;
    }

    Fragments -= RequiredCost;
    if (RequiredCost > 0 && !bUsedFirstSpendRefundThisTurn && Attributes)
    {
        const int32 Refund = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("InitiativeGainOnFirstFragmentSpend"))));
        if (Refund > 0) Attributes->GainInitiative(Refund);
        bUsedFirstSpendRefundThisTurn = true;
    }
    const int32 SiphonReduction = RequiredCost * FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("TimeSiphonCooldownReductionPerFragmentSpent"))));
    if (SiphonReduction > 0)
    {
        for (ARoundAbilityActor* Ability : GrantedAbilities)
        {
            if (Ability && Ability->AbilityData && Ability->AbilityData->AbilityId == FName(TEXT("TimeSiphon")))
            {
                Ability->ReduceCooldown(SiphonReduction);
                break;
            }
        }
    }
    const int32 HealPerFragment = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("HealPerFragmentSpent"))));
    if (HealPerFragment > 0) Heal(RequiredCost * HealPerFragment);
    NotifyRoundStateChanged();
    ForceNetUpdate();
    return true;
}
