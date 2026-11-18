#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundFireballAfterburnEffect.generated.h"

UCLASS()
class ROUND_API ARoundFireballAfterburnEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TObjectPtr<class ARoundFireballAbility> FireballAbility = nullptr;

    UPROPERTY()
    FVector2D TargetTilePosition = FVector2D::ZeroVector;

    UPROPERTY()
    double DamageMultiplier = 0.5;

    virtual void OnInitiativeAdvanced(class ARoundCharacter* TargetCharacter, int32 InitiativeAmount) override;
};
