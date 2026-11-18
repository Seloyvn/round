#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundTerrainActor.h"
#include "RoundForestTerrainActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundForestTerrainActor : public ARoundTerrainActor
{
    GENERATED_BODY()

public:
    ARoundForestTerrainActor();

    virtual int32 ModifyIncomingDamage_Implementation(ARoundCharacter* Defender, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType) const override;
};
