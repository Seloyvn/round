#include "Effects/RoundBladeGuardEffect.h"

#include "Net/UnrealNetwork.h"

void ARoundBladeGuardEffect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundBladeGuardEffect, DamageReductionPerStack);
    DOREPLIFETIME(ARoundBladeGuardEffect, StackRemainingInitiatives);
}

void ARoundBladeGuardEffect::AddStack(int32 DurationInitiative)
{
    StackRemainingInitiatives.Add(FMath::Max(1, DurationInitiative));
    RefreshDisplayedDuration();
    ForceNetUpdate();
}

int32 ARoundBladeGuardEffect::ModifyIncomingDamage(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const
{
    const int32 ActiveStacks = StackRemainingInitiatives.Num();
    const double TotalReduction = FMath::Clamp(DamageReductionPerStack * static_cast<double>(ActiveStacks), 0.0, 1.0);
    if (TotalReduction <= 0.0 || Damage <= 0)
    {
        return Damage;
    }

    return FMath::Max(0, FMath::RoundToInt(static_cast<double>(Damage) * (1.0 - TotalReduction)));
}

void ARoundBladeGuardEffect::OnInitiativeAdvanced(ARoundCharacter* TargetCharacter, int32 InitiativeAmount)
{
    if (InitiativeAmount <= 0 || StackRemainingInitiatives.Num() <= 0)
    {
        return;
    }

    for (int32 Index = StackRemainingInitiatives.Num() - 1; Index >= 0; --Index)
    {
        StackRemainingInitiatives[Index] = FMath::Max(0, StackRemainingInitiatives[Index] - InitiativeAmount);
        if (StackRemainingInitiatives[Index] <= 0)
        {
            StackRemainingInitiatives.RemoveAt(Index);
        }
    }

    if (StackRemainingInitiatives.Num() <= 0)
    {
        RemoveEffect();
        return;
    }

    RefreshDisplayedDuration();
    ForceNetUpdate();
}

void ARoundBladeGuardEffect::RefreshDisplayedDuration()
{
    int32 MaxRemainingInitiative = 0;
    for (const int32 StackDuration : StackRemainingInitiatives)
    {
        MaxRemainingInitiative = FMath::Max(MaxRemainingInitiative, StackDuration);
    }

    RemainingTurns = 0;
    RemainingInitiative = MaxRemainingInitiative;
    bAutoExpireOnTurnEnd = false;
}
