#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoundTypes.h"
#include "RoundSkillProgressionData.generated.h"

UCLASS(BlueprintType)
class ROUND_API URoundSkillProgressionData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression")
    FName SkillId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression")
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression")
    TArray<FRoundSkillpointDetails> Levels;
};
