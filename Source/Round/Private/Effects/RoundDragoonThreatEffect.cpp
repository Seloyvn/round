#include "Effects/RoundDragoonThreatEffect.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacterDragoon.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Net/UnrealNetwork.h"

void ARoundDragoonThreatEffect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundDragoonThreatEffect, TriggerDamage);
    DOREPLIFETIME(ARoundDragoonThreatEffect, TriggerDamageType);
    DOREPLIFETIME(ARoundDragoonThreatEffect, TriggerDefenseIgnore);
    DOREPLIFETIME(ARoundDragoonThreatEffect, ThreatRangeAdd);
    DOREPLIFETIME(ARoundDragoonThreatEffect, InitiativeLossOnTrigger);
    DOREPLIFETIME(ARoundDragoonThreatEffect, IncomingDamageMultiplier);
    DOREPLIFETIME(ARoundDragoonThreatEffect, MaxTriggers);
    DOREPLIFETIME(ARoundDragoonThreatEffect, TriggerCount);
}

bool ARoundDragoonThreatEffect::IsBuff() const
{
    return true;
}

bool ARoundDragoonThreatEffect::IsRemovable() const
{
    return true;
}

void ARoundDragoonThreatEffect::OnTurnStarted(ARoundCharacter* TargetCharacter)
{
    if (TargetCharacter == OwningCharacter)
    {
        RemoveEffect();
    }
}

void ARoundDragoonThreatEffect::OnOtherCharacterMoved(ARoundCharacter* TargetCharacter, ARoundCharacter* MovedCharacter, ARoundGridTile* PreviousTile, ARoundGridTile* NewTile)
{
    if (!OwningCharacter || !MovedCharacter || !PreviousTile || !NewTile || MovedCharacter == OwningCharacter)
    {
        return;
    }

    if (!OwningCharacter->IsEnemyFor(MovedCharacter) || !OwningCharacter->CurrentTile)
    {
        return;
    }

    const int32 ThreatRange = OwningCharacter->GetAttackRange() + FMath::Max(0, ThreatRangeAdd);
    const bool bWasInRange = OwningCharacter->CurrentTile->GetDistance(PreviousTile) <= ThreatRange;
    const bool bIsStillInRange = OwningCharacter->CurrentTile->GetDistance(NewTile) <= ThreatRange;
    if (!bWasInRange || bIsStillInRange)
    {
        return;
    }

    bool bDidDie = false;
    MovedCharacter->ApplyRoundDamage(
        FMath::Max(0, TriggerDamage),
        TriggerDamageType,
        FMath::Max(0, TriggerDefenseIgnore),
        OwningCharacter,
        bDidDie);
    if (MovedCharacter->Attributes) MovedCharacter->Attributes->PassInitiative(FMath::Max(0, InitiativeLossOnTrigger));
    ++TriggerCount;

    if (ARoundCharacterDragoon* Dragoon = Cast<ARoundCharacterDragoon>(OwningCharacter))
    {
        Dragoon->RecordEnemyHit(MovedCharacter, TriggerDamage, TriggerDamageType);
    }
    if (MaxTriggers > 0 && TriggerCount >= MaxTriggers) RemoveEffect();
}

int32 ARoundDragoonThreatEffect::ModifyIncomingDamage(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const
{
    return FMath::Max(0, FMath::RoundToInt(Damage * FMath::Max(0.0, IncomingDamageMultiplier)));
}
