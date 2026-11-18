#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoundTypes.h"
#include "RoundTeamPresetData.generated.h"

UCLASS(BlueprintType)
class ROUND_API URoundTeamPresetData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Teams")
    TArray<FRoundTeamDefinition> Teams;
};
