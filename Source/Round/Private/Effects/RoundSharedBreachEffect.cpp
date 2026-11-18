#include "Effects/RoundSharedBreachEffect.h"

#include "Actors/RoundCharacter.h"
#include "Net/UnrealNetwork.h"

void ARoundSharedBreachEffect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundSharedBreachEffect, DefenseIgnorePerCharacter);
}

int32 ARoundSharedBreachEffect::ModifyIncomingDamage(
    ARoundCharacter*, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType, int32& DefenseIgnore) const
{
    if (!SourceCharacter || !CauserCharacter || CauserCharacter->IsEnemyFor(SourceCharacter)
        || TriggeredCharacters.Contains(SourceCharacter))
    {
        return Damage;
    }

    TriggeredCharacters.Add(SourceCharacter);
    DefenseIgnore += FMath::Max(0, DefenseIgnorePerCharacter);
    return Damage;
}
