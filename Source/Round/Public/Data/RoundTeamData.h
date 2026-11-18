#pragma once

#include "CoreMinimal.h"
#include "RoundTeamData.generated.h"

USTRUCT(BlueprintType)
struct FSelectedCharacterEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
    FName CharacterId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
    int32 SlotIndex = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FTeamLoadout
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
    FString TeamName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
    TArray<FSelectedCharacterEntry> Characters;
};
