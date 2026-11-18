#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoundTypes.h"
#include "RoundCharacterProgressionData.generated.h"

UCLASS(BlueprintType)
class ROUND_API URoundCharacterProgressionData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression")
    FName CharacterId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression")
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression")
    TArray<FRoundSkillpointTier> Tiers;
};
