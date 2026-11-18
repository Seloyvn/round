#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundTerrainActor.h"
#include "RoundRubbleTerrainActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundRubbleTerrainActor : public ARoundTerrainActor
{
    GENERATED_BODY()

public:
    ARoundRubbleTerrainActor();

    virtual int32 GetMoveCost_Implementation(int32 BaseCost) const override;
};
