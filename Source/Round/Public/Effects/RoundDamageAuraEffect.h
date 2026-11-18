#pragma once

#include "CoreMinimal.h"
#include "Effects/RoundAuraEffect.h"
#include "RoundDamageAuraEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundDamageAuraEffect : public ARoundAuraEffect
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Aura")
    int32 AuraTriggerDamage = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Aura")
    ERoundDamageType AuraTriggerDamageType = ERoundDamageType::Physical;

    virtual void OnInitiativeAdvanced(class ARoundCharacter* TargetCharacter, int32 InitiativeAmount) override;
};
