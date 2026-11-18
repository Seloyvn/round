#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundSlowEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundSlowEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    int32 InitiativeLossOnMove = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    int32 InitiativeLossOnExpire = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    bool bBlockMovementBonuses = false;

    virtual bool IsDebuff() const override { return true; }
    virtual int32 GetMovecost_Implementation(int32 In) const override;
    virtual void OnOwnerMoved(class ARoundCharacter* TargetCharacter, class ARoundGridTile* PreviousTile, class ARoundGridTile* NewTile) override;
    virtual void OnTurnEnded(class ARoundCharacter* TargetCharacter) override;
};
