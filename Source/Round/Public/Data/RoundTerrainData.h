#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoundTypes.h"
#include "RoundTerrainData.generated.h"

UCLASS(BlueprintType)
class ROUND_API URoundTerrainData : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain")
    TSubclassOf<class AActor> TerrainActorClass = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain")
    ERoundTerrainType TerrainType = ERoundTerrainType::Ground;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain")
    TMap<FString, double> Values;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain")
    float DefaultWeight = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain")
    int32 ClusterSizeMin = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain")
    int32 ClusterSizeMax = 1;
};

UCLASS(BlueprintType)
class ROUND_API URoundTerrainRegistryData : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain")
    TMap<ERoundTerrainType, TObjectPtr<URoundTerrainData>> TerrainDataByType;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    const URoundTerrainData* GetTerrainData(const ERoundTerrainType TerrainType) const
    {
        if (const TObjectPtr<URoundTerrainData>* Found = TerrainDataByType.Find(TerrainType))
        {
            return Found->Get();
        }

        return nullptr;
    }
};
