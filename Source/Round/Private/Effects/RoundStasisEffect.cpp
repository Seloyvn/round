#include "Effects/RoundStasisEffect.h"

#include "Actors/RoundCharacter.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Net/UnrealNetwork.h"

void ARoundStasisEffect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundStasisEffect, HealOnRelease);
    DOREPLIFETIME(ARoundStasisEffect, InitiativeGainOnRelease);
}

bool ARoundStasisEffect::PreventsAbilityUse(const ARoundAbilityActor* Ability) const
{
    return true;
}

bool ARoundStasisEffect::PreventsTurnActions() const
{
    return true;
}

int32 ARoundStasisEffect::GetTurnBlockRemainingInitiative() const
{
    return FMath::Max(0, RemainingInitiative);
}

int32 ARoundStasisEffect::ModifyInitiativeGain(int32 InitiativeAmount) const
{
    return 0;
}

int32 ARoundStasisEffect::ModifyIncomingDamageBeforeMitigation(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType) const
{
    return 0;
}

void ARoundStasisEffect::OnInitiativeAdvanced(ARoundCharacter* TargetCharacter, int32 InitiativeAmount)
{
    if (InitiativeAmount <= 0 || RemainingInitiative <= 0) return;
    RemainingInitiative = FMath::Max(0, RemainingInitiative - InitiativeAmount);
    if (RemainingInitiative > 0) return;
    if (TargetCharacter)
    {
        TargetCharacter->Heal(HealOnRelease);
        if (TargetCharacter->Attributes) TargetCharacter->Attributes->GainInitiative(InitiativeGainOnRelease);
    }
    RemoveEffect();
}
