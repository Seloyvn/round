#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundTerrainActor.h"
#include "RoundWallTerrainActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundWallTerrainActor : public ARoundTerrainActor
{
    GENERATED_BODY()

public:
    ARoundWallTerrainActor();
};

