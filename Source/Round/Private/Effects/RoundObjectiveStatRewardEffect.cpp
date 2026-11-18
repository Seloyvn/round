#include "Effects/RoundObjectiveStatRewardEffect.h"

#include "Actors/RoundCharacter.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Net/UnrealNetwork.h"

void ARoundObjectiveStatRewardEffect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundObjectiveStatRewardEffect, AttackAdd);
    DOREPLIFETIME(ARoundObjectiveStatRewardEffect, DefenseAdd);
    DOREPLIFETIME(ARoundObjectiveStatRewardEffect, MaxHealthAdd);
    DOREPLIFETIME(ARoundObjectiveStatRewardEffect, MaxInitiativeAdd);
}

void ARoundObjectiveStatRewardEffect::ApplyToCharacter(ARoundCharacter* TargetCharacter)
{
    Super::ApplyToCharacter(TargetCharacter);

    if (bApplied || !TargetCharacter || !TargetCharacter->Attributes)
    {
        return;
    }

    FRoundStatBlock& Stats = TargetCharacter->Attributes->BaseStats;
    Stats.Attack += AttackAdd;
    Stats.Defense += DefenseAdd;
    Stats.MaxHealth += MaxHealthAdd;
    Stats.MaxInitiative += MaxInitiativeAdd;

    if (MaxHealthAdd > 0)
    {
        TargetCharacter->Attributes->CurrentHealth += MaxHealthAdd;
    }

    if (MaxInitiativeAdd > 0)
    {
        TargetCharacter->Attributes->CurrentInitiative += MaxInitiativeAdd;
    }

    bApplied = true;
}

void ARoundObjectiveStatRewardEffect::RemoveEffect()
{
    if (bApplied && OwningCharacter && OwningCharacter->Attributes)
    {
        FRoundStatBlock& Stats = OwningCharacter->Attributes->BaseStats;
        Stats.Attack -= AttackAdd;
        Stats.Defense -= DefenseAdd;
        Stats.MaxHealth = FMath::Max(1, Stats.MaxHealth - MaxHealthAdd);
        Stats.MaxInitiative = FMath::Max(1, Stats.MaxInitiative - MaxInitiativeAdd);
        OwningCharacter->Attributes->CurrentHealth = FMath::Min(OwningCharacter->Attributes->CurrentHealth, Stats.MaxHealth);
        OwningCharacter->Attributes->CurrentInitiative = FMath::Min(OwningCharacter->Attributes->CurrentInitiative, Stats.MaxInitiative);
        bApplied = false;
    }

    Super::RemoveEffect();
}
