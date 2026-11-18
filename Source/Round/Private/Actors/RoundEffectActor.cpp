#include "Actors/RoundEffectActor.h"

#include "Actors/RoundAbilityActor.h"
#include "Actors/RoundCharacter.h"
#include "Net/UnrealNetwork.h"

namespace
{
int32 GetRuntimeLegacyTurnToInitiativeScale()
{
    return 40;
}
}

ARoundEffectActor::ARoundEffectActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(false);
}

void ARoundEffectActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundEffectActor, EffectId);
    DOREPLIFETIME(ARoundEffectActor, DisplayName);
    DOREPLIFETIME(ARoundEffectActor, Description);
    DOREPLIFETIME(ARoundEffectActor, Amount);
    DOREPLIFETIME(ARoundEffectActor, RemainingTurns);
    DOREPLIFETIME(ARoundEffectActor, RemainingInitiative);
    DOREPLIFETIME(ARoundEffectActor, OwningCharacter);
    DOREPLIFETIME(ARoundEffectActor, CauserCharacter);
    DOREPLIFETIME(ARoundEffectActor, bAutoExpireOnTurnEnd);
}

void ARoundEffectActor::ApplyToCharacter(ARoundCharacter* TargetCharacter)
{
    OwningCharacter = TargetCharacter;
    OnApplied(TargetCharacter);
}

void ARoundEffectActor::RemoveEffect()
{
    if (OwningCharacter)
    {
        ARoundCharacter* TargetCharacter = OwningCharacter;
        OwningCharacter = nullptr;
        TargetCharacter->RemoveEffect(this);
    }

    Destroy();
}

void ARoundEffectActor::SetDurationInitiative(int32 DurationInitiative, bool bExpireOnTurnEnd)
{
    RemainingTurns = 0;
    RemainingInitiative = FMath::Max(0, DurationInitiative);
    bAutoExpireOnTurnEnd = bExpireOnTurnEnd;
}

void ARoundEffectActor::SetDurationLegacyTurns(int32 DurationTurns, bool bExpireOnTurnEnd)
{
    RemainingTurns = FMath::Max(0, DurationTurns);
    RemainingInitiative = 0;
    bAutoExpireOnTurnEnd = bExpireOnTurnEnd;
}

int32 ARoundEffectActor::GetRemainingDurationInitiative() const
{
    if (RemainingInitiative > 0)
    {
        return RemainingInitiative;
    }

    if (RemainingTurns > 0)
    {
        return RemainingTurns * GetRuntimeLegacyTurnToInitiativeScale();
    }

    return 0;
}

void ARoundEffectActor::OnApplied_Implementation(ARoundCharacter* TargetCharacter)
{
}

bool ARoundEffectActor::SetHasVision_Implementation(ARoundCharacter* TargetCharacter, bool bCurrentVision) const
{
    return bCurrentVision;
}

int32 ARoundEffectActor::GetMovecost_Implementation(int32 In) const
{
    return In;
}

int32 ARoundEffectActor::ModifyIncomingDamage(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const
{
    return Damage;
}

int32 ARoundEffectActor::ModifyOutgoingDamage(ARoundCharacter* SourceCharacter, ARoundCharacter* TargetCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const
{
    return Damage;
}

int32 ARoundEffectActor::ModifyIncomingDamageBeforeMitigation(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType) const
{
    return Damage;
}

bool ARoundEffectActor::PreventsAbilityUse(const ARoundAbilityActor* Ability) const
{
    return false;
}

bool ARoundEffectActor::PreventsEffectApplication(const ARoundEffectActor* IncomingEffect) const
{
    return false;
}

bool ARoundEffectActor::IsBuff() const
{
    return false;
}

bool ARoundEffectActor::IsDebuff() const
{
    return false;
}

bool ARoundEffectActor::IsRemovable() const
{
    return true;
}

bool ARoundEffectActor::PreventsTurnActions() const
{
    return false;
}

bool ARoundEffectActor::PreventsMovement() const
{
    return false;
}

int32 ARoundEffectActor::GetTurnBlockRemainingInitiative() const
{
    return 0;
}

int32 ARoundEffectActor::ModifyInitiativeGain(int32 InitiativeAmount) const
{
    return InitiativeAmount;
}

int32 ARoundEffectActor::ModifyBasicAttackDamage(ARoundCharacter*, ARoundCharacter*, int32 Damage) const
{
    return Damage;
}

void ARoundEffectActor::OnTurnStarted(ARoundCharacter* TargetCharacter)
{
}

void ARoundEffectActor::OnTurnEnded(ARoundCharacter* TargetCharacter)
{
    if (bAutoExpireOnTurnEnd && RemainingTurns > 0)
    {
        RemainingTurns = FMath::Max(0, RemainingTurns - 1);
        if (RemainingTurns <= 0 && RemainingInitiative <= 0)
        {
            RemoveEffect();
        }
    }
}

void ARoundEffectActor::OnInitiativeAdvanced(ARoundCharacter* TargetCharacter, int32 InitiativeAmount)
{
    if (InitiativeAmount <= 0)
    {
        return;
    }

    if (RemainingInitiative > 0)
    {
        RemainingInitiative = FMath::Max(0, RemainingInitiative - InitiativeAmount);
        if (RemainingInitiative <= 0 && RemainingTurns <= 0)
        {
            RemoveEffect();
        }
    }
}

void ARoundEffectActor::OnOwnerMoved(ARoundCharacter* TargetCharacter, ARoundGridTile* PreviousTile, ARoundGridTile* NewTile)
{
}

void ARoundEffectActor::OnOtherCharacterMoved(ARoundCharacter* TargetCharacter, ARoundCharacter* MovedCharacter, ARoundGridTile* PreviousTile, ARoundGridTile* NewTile)
{
}

void ARoundEffectActor::OnDamageTaken(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 AppliedDamage, ERoundDamageType DamageType)
{
}

int32 ARoundEffectActor::ModifyBasicAttackCost(ARoundCharacter*, int32 AttackCost) const
{
    return AttackCost;
}

void ARoundEffectActor::OnBasicAttackExecuted(ARoundCharacter*, ARoundCharacter*)
{
}
