#pragma once

#include "Actors/RoundTerrainActor.h"
#include "RoundGroveTerrainActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundGroveTerrainActor : public ARoundTerrainActor
{
    GENERATED_BODY()

public:
    ARoundGroveTerrainActor();

    virtual int32 ModifyIncomingDamage_Implementation(class ARoundCharacter* Defender, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType) const override;
};
