#include "Effects/RoundRootEffect.h"

#include "Actors/RoundCharacter.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Game/RoundGameState.h"
#include "Net/UnrealNetwork.h"

void ARoundRootEffect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundRootEffect, IncomingDamageMultiplierFromCauser);
    DOREPLIFETIME(ARoundRootEffect, InitiativeLossOnExpire);
}

int32 ARoundRootEffect::GetMovecost_Implementation(const int32 In) const
{
    return MAX_int32;
}

int32 ARoundRootEffect::ModifyIncomingDamage(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const
{
    if (SourceCharacter == CauserCharacter && IncomingDamageMultiplierFromCauser > 0.0)
    {
        return FMath::Max(0, FMath::RoundToInt(Damage * IncomingDamageMultiplierFromCauser));
    }
    return Damage;
}

void ARoundRootEffect::OnInitiativeAdvanced(ARoundCharacter* TargetCharacter, int32 InitiativeAmount)
{
    if (InitiativeAmount <= 0 || RemainingInitiative <= 0) return;
    RemainingInitiative = FMath::Max(0, RemainingInitiative - InitiativeAmount);
    if (RemainingInitiative > 0) return;
    const ARoundGameState* GameState = ARoundGameState::GetRoundGameState(TargetCharacter);
    const bool bCoreAlreadyAppliedExpiry = GameState && GameState->IsApplyingCoreInitiativeAdvance();
    if (!bCoreAlreadyAppliedExpiry && TargetCharacter && TargetCharacter->Attributes && InitiativeLossOnExpire > 0)
    {
        TargetCharacter->Attributes->PassInitiative(InitiativeLossOnExpire);
    }
    RemoveEffect();
}
