#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RoundAuraListItemData.generated.h"

UCLASS(BlueprintType)
class ROUND_API URoundAuraListItemData : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI")
    int32 RemainingInitiative = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI")
    FString AuraInstanceId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI")
    FString SourceCharacterId;
};
