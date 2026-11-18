#include "Effects/RoundShadowWoundEffect.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter3.h"
#include "Net/UnrealNetwork.h"

void ARoundShadowWoundEffect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundShadowWoundEffect, BonusDamage);
    DOREPLIFETIME(ARoundShadowWoundEffect, ComboGain);
}

void ARoundShadowWoundEffect::OnDamageTaken(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 AppliedDamage, ERoundDamageType DamageType)
{
    if (bTriggered || AppliedDamage <= 0 || !TargetCharacter || SourceCharacter != CauserCharacter) return;

    bTriggered = true;
    ARoundCharacter3* Assassin = Cast<ARoundCharacter3>(CauserCharacter);
    RemoveEffect();
    if (Assassin && ComboGain > 0) Assassin->TryGainComboPoints(ComboGain);

    bool bDidDie = false;
    TargetCharacter->ApplyRoundDamage(FMath::Max(0, BonusDamage), DamageType, 0, CauserCharacter, bDidDie);
}
