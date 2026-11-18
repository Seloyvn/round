#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoundTypes.h"
#include "RoundObjectiveRewardData.generated.h"

UCLASS(BlueprintType)
class ROUND_API URoundObjectiveRewardData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
    FName RewardId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
    int32 RewardNumericId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
    ERoundObjectiveRewardType RewardType = ERoundObjectiveRewardType::Heal;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
    double Value = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
    int32 DurationInitiative = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
    bool bPermanent = false;
};
