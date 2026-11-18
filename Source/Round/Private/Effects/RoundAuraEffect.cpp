#include "Effects/RoundAuraEffect.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "EngineUtils.h"
#include "Game/RoundGameState.h"

void ARoundAuraEffect::OnApplied_Implementation(ARoundCharacter* TargetCharacter)
{
    Super::OnApplied_Implementation(TargetCharacter);
}

void ARoundAuraEffect::RemoveEffect()
{
    ActiveAuraModifiers.Reset();
    Super::RemoveEffect();
}

void ARoundAuraEffect::OnInitiativeAdvanced(ARoundCharacter* TargetCharacter, int32 InitiativeAmount)
{
    Super::OnInitiativeAdvanced(TargetCharacter, InitiativeAmount);
}

bool ARoundAuraEffect::AffectsCharacter(const ARoundCharacter* Character) const
{
    if (!Character || !Character->CurrentTile)
    {
        return false;
    }

    const ARoundCharacter* AuraOwner = OwningCharacter;
    ARoundGridTile* SourceTile = nullptr;
    if (bAuraUsesFixedPosition)
    {
        if (const ARoundGameState* GameState = AuraOwner ? AuraOwner->GetRoundGameState() : nullptr)
        {
            SourceTile = GameState->GetField(AuraFixedPosition);
        }
    }
    else if (AuraOwner)
    {
        SourceTile = AuraOwner->CurrentTile;
    }

    if (!SourceTile)
    {
        return false;
    }

    const bool bIsSelf = AuraOwner == Character;
    const bool bIsAlly = AuraOwner && Character->Team == AuraOwner->Team && !bIsSelf;
    const bool bIsEnemy = AuraOwner && Character->Team != AuraOwner->Team;

    if (bIsSelf && !bAffectSelf)
    {
        return false;
    }

    if (bIsAlly && !bAffectAllies)
    {
        return false;
    }

    if (bIsEnemy && !bAffectEnemies)
    {
        return false;
    }

    return SourceTile->GetDistance(Character->CurrentTile) <= Radius;
}

int32 ARoundAuraEffect::ModifyIncomingDamageForCharacter(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore)
{
    return Damage;
}

bool ARoundAuraEffect::TryGetIncomingDamageReductionFraction(double& OutFraction) const
{
    OutFraction = 0.0;
    return false;
}
