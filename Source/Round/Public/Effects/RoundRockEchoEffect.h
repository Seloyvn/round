#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundRockEchoEffect.generated.h"

UCLASS()
class ROUND_API ARoundRockEchoEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TArray<FVector2D> RockTilePositions;

    virtual void OnInitiativeAdvanced(class ARoundCharacter* TargetCharacter, int32 InitiativeAmount) override;
};
