#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundTerrainActor.h"
#include "RoundCompositeTerrainActor.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

USTRUCT(BlueprintType)
struct FRoundTerrainVisualPart
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Terrain|Visual")
    TSoftObjectPtr<UStaticMesh> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Terrain|Visual")
    TSoftObjectPtr<UMaterialInterface> Material;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Terrain|Visual")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Terrain|Visual")
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Terrain|Visual")
    FVector Scale = FVector::OneVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Terrain|Visual")
    bool bCastShadow = true;
};

/**
 * Data-driven presentation actor used by terrain Blueprint classes.
 * Each Blueprint owns its visual-part list, so terrain silhouettes can be
 * changed in the editor without adding terrain-specific runtime branches.
 */
UCLASS(Blueprintable)
class ROUND_API ARoundCompositeTerrainActor : public ARoundTerrainActor
{
    GENERATED_BODY()

public:
    ARoundCompositeTerrainActor();
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round|Terrain|Visual")
    TArray<FRoundTerrainVisualPart> VisualParts;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<UStaticMeshComponent>> GeneratedVisualComponents;

    void RebuildVisualParts();
};
