#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RoundTypes.h"
#include "RoundSaveGame.generated.h"

UCLASS(BlueprintType)
class ROUND_API URoundSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Save")
    TArray<FRoundTeamDefinition> Teams;
};
