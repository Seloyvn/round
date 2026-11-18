#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundTerrainActor.h"
#include "RoundGroundTerrainActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundGroundTerrainActor : public ARoundTerrainActor
{
    GENERATED_BODY()

public:
    ARoundGroundTerrainActor();
};

