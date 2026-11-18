#pragma once

#include "CoreMinimal.h"
#include "Effects/RoundDamageAuraEffect.h"
#include "RoundSanctifiedGroundAuraEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundSanctifiedGroundAuraEffect : public ARoundDamageAuraEffect
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Paladin") int32 InitiativeLossPerPulse = 0;
    virtual void OnInitiativeAdvanced(class ARoundCharacter* TargetCharacter, int32 InitiativeAmount) override;
};
