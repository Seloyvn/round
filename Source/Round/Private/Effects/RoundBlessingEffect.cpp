#include "Effects/RoundBlessingEffect.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacterCleric.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Net/UnrealNetwork.h"

void ARoundBlessingEffect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundBlessingEffect, Stacks);
}

void ARoundBlessingEffect::OnDamageTaken(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 AppliedDamage, ERoundDamageType DamageType)
{
    if (AppliedDamage <= 0)
    {
        return;
    }

    AddBlessingStack(TargetCharacter, 1, true);
}

void ARoundBlessingEffect::AddBlessingStack(ARoundCharacter* TargetCharacter, int32 AddedStacks, bool bRefreshDuration)
{
    if (AddedStacks <= 0)
    {
        return;
    }

    Stacks = FMath::Clamp(Stacks + AddedStacks, 0, FMath::Max(1, MaxStacks));
    if (bRefreshDuration && RemainingInitiative > 0)
    {
        const int32 BaseDuration = IntegerState.FindRef(TEXT("BlessingBaseDurationInitiative"));
        if (BaseDuration > 0)
        {
            RemainingInitiative = BaseDuration;
        }
    }

    TryResolveBlessingHeal(TargetCharacter);
}

void ARoundBlessingEffect::TryResolveBlessingHeal(ARoundCharacter* TargetCharacter)
{
    if (!TargetCharacter || HealAmount <= 0 || StackHealThreshold <= 0)
    {
        return;
    }

    if (Stacks < StackHealThreshold)
    {
        return;
    }

    const int32 ActualHeal = TargetCharacter->Heal(HealAmount);
    if (ARoundCharacterCleric* Cleric = Cast<ARoundCharacterCleric>(CauserCharacter))
    {
        const int32 AllyInitiative = FMath::Max(0, FMath::RoundToInt(Cleric->GetPassiveValueFromKey(TEXT("InitiativeOnBlessingResolve"))));
        if (TargetCharacter->Attributes && AllyInitiative > 0) TargetCharacter->Attributes->GainInitiative(AllyInitiative);
        const double SelfHealFraction = FMath::Max(0.0, Cleric->GetPassiveValueFromKey(TEXT("SelfHealFractionOnBlessingResolve")));
        if (SelfHealFraction > 0.0) Cleric->Heal(FMath::RoundToInt(ActualHeal * SelfHealFraction));
    }
    Stacks = 0;
}
