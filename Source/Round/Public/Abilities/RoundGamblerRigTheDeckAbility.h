#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundAbilityActor.h"
#include "RoundGamblerRigTheDeckAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundGamblerRigTheDeckAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual bool RequiresHandCardSelection() const override;
    virtual int32 GetMinHandCardSelections() const override;
    virtual int32 GetMaxHandCardSelections() const override;
    virtual bool IsValidHandCardSelectionIndex(int32 CardIndex) const override;
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
