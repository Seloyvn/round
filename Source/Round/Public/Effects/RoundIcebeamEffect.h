#pragma once

#include "Effects/RoundSlowEffect.h"
#include "RoundIcebeamEffect.generated.h"

UCLASS()
class ROUND_API ARoundIcebeamEffect : public ARoundSlowEffect
{
    GENERATED_BODY()

public:
    UPROPERTY()
    bool bFirstMovePenaltyAvailable = false;

    UPROPERTY()
    int32 FirstMoveInitiativeLoss = 0;

    UPROPERTY()
    int32 MaximumMovedFields = 0;

    UPROPERTY()
    int32 MovedFields = 0;

    UPROPERTY()
    int32 FrostbreakDamage = 0;

    virtual void OnOwnerMoved(class ARoundCharacter* TargetCharacter, class ARoundGridTile* PreviousTile, class ARoundGridTile* NewTile) override;
    virtual void OnDamageTaken(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 AppliedDamage, ERoundDamageType DamageType) override;
};
