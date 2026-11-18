#include "Effects/RoundAssassinHuntMarkEffect.h"

#include "Net/UnrealNetwork.h"

void ARoundAssassinHuntMarkEffect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundAssassinHuntMarkEffect, DamageMultiplierFromCauser);
}

int32 ARoundAssassinHuntMarkEffect::ModifyIncomingDamageBeforeMitigation(
    ARoundCharacter*, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType) const
{
    if (!SourceCharacter || SourceCharacter != CauserCharacter)
    {
        return Damage;
    }

    return FMath::Max(0, FMath::RoundToInt(static_cast<double>(Damage) * FMath::Max(0.0, DamageMultiplierFromCauser)));
}
