#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundTerrainActor.h"
#include "RoundFireTerrainActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundFireTerrainActor : public ARoundTerrainActor
{
    GENERATED_BODY()

public:
    ARoundFireTerrainActor();

    virtual void SetTerrainData(URoundTerrainData* InTerrainData) override;
    virtual void OnCharacterTurnStarted_Implementation(ARoundCharacter* OccupyingCharacter) override;
};
