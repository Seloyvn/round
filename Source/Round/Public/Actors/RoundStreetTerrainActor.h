#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundTerrainActor.h"
#include "RoundStreetTerrainActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundStreetTerrainActor : public ARoundTerrainActor
{
    GENERATED_BODY()

public:
    ARoundStreetTerrainActor();

    virtual int32 GetMoveCost_Implementation(int32 BaseCost) const override;
};
