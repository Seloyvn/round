#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RoundSessionListItem.generated.h"

UCLASS(BlueprintType)
class ROUND_API URoundSessionListItem : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|Session")
    FString SessionName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|Session")
    int32 SessionIndex = INDEX_NONE;
};
