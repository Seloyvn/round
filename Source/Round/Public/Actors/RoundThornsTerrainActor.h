#pragma once

#include "Actors/RoundTerrainActor.h"
#include "RoundThornsTerrainActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundThornsTerrainActor : public ARoundTerrainActor
{
    GENERATED_BODY()

public:
    ARoundThornsTerrainActor();

    virtual void OnCharacterEntered_Implementation(class ARoundCharacter* EnteringCharacter, class ARoundGridTile* PreviousTile) override;
};
