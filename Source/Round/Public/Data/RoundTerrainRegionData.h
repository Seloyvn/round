#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoundTypes.h"
#include "RoundTerrainRegionData.generated.h"

UENUM(BlueprintType)
enum class ERoundTerrainLayerPattern : uint8
{
    Cluster,
    Scatter,
    MapEdgeBand,
    RegionBorder,
    RegionCore,
    Path,
    AdjacentToLayer,
    Crossing
};

UENUM(BlueprintType)
enum class ERoundTerrainRegionPlacement : uint8
{
    Any,
    EdgeOnly,
    InteriorOnly
};

UENUM(BlueprintType)
enum class ERoundTerrainStructureTemplate : uint8
{
    Open,
    Scattered,
    Split,
    Corridor,
    Ring,
    Chokepoint,
    Landmark
};

USTRUCT(BlueprintType)
struct FRoundTerrainRegionStructure
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Structure")
    ERoundTerrainStructureTemplate Template = ERoundTerrainStructureTemplate::Open;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Structure")
    ERoundTerrainType TerrainType = ERoundTerrainType::Rocks;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Structure", meta = (ClampMin = "0.0"))
    double Weight = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Structure", meta = (ClampMin = "0.0", ClampMax = "0.8"))
    double Coverage = 0.15;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Structure", meta = (ClampMin = "1"))
    int32 ClusterCountMin = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Structure", meta = (ClampMin = "1"))
    int32 ClusterCountMax = 1;

    // Walkable lanes reserved from every neighbouring region towards the
    // structure's centre. This keeps macro structures useful but traversable.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Structure", meta = (ClampMin = "1", ClampMax = "4"))
    int32 PassageWidth = 1;
};

USTRUCT(BlueprintType)
struct FRoundTerrainRegionLayer
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region")
    FName LayerId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region")
    ERoundTerrainType TerrainType = ERoundTerrainType::Ground;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region")
    ERoundTerrainLayerPattern Pattern = ERoundTerrainLayerPattern::Cluster;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region")
    FName SourceLayerId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    double Coverage = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region", meta = (ClampMin = "1"))
    int32 PatchSizeMin = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region", meta = (ClampMin = "1"))
    int32 PatchSizeMax = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region", meta = (ClampMin = "0"))
    int32 DepthMin = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region", meta = (ClampMin = "0"))
    int32 DepthMax = 0;

    // If either list is populated, Path connects a boundary tile belonging to
    // one of the allowed start regions with one of the allowed end regions.
    // This lets structures such as rivers use explicit sources and sinks.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Path")
    TArray<FName> PathStartRegionIds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Path")
    TArray<FName> PathEndRegionIds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Path", meta = (ClampMin = "1"))
    int32 PathMinimumLength = 4;

    // Number of ordinary source-path fields required between two crossings.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Crossing", meta = (ClampMin = "0"))
    int32 MinimumCrossingGap = 0;

    // Zero disables automatic spacing. Otherwise another valid crossing is
    // inserted before more than this many source-path fields remain uncrossed.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Crossing", meta = (ClampMin = "0"))
    int32 MaximumUncrossedPathFields = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Crossing")
    bool bRequireWalkableOppositeSides = false;
};

UCLASS(BlueprintType)
class ROUND_API URoundTerrainRegionData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region")
    FName RegionId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region")
    FText DisplayName;

    // Base colour used by the development terrain-region overlay. Individual
    // generated regions receive a small deterministic brightness variation.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Debug")
    FLinearColor DebugColor = FLinearColor(0.25f, 0.55f, 1.0f, 0.55f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region", meta = (ClampMin = "0.0"))
    double BaseWeight = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region")
    bool bCanGenerate = true;

    // Global features do not claim a biome-sized area. Their layers are
    // applied once after all normal regions, e.g. a river crossing biomes.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region")
    bool bGlobalFeature = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Placement")
    ERoundTerrainRegionPlacement Placement = ERoundTerrainRegionPlacement::Any;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Placement", meta = (ClampMin = "0"))
    int32 EdgeDistance = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Placement", meta = (ClampMin = "0"))
    int32 MinimumOccurrences = 0;

    // Zero means unlimited. Useful for special structures such as a single
    // coast, river or ruin complex on one generated map.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Placement", meta = (ClampMin = "0"))
    int32 MaximumOccurrences = 0;

    // Conceptual size before clipping at the board boundary. A region may
    // grow beyond the playable board, so edge regions keep a natural shape.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Size", meta = (ClampMin = "1"))
    int32 RegionSizeMin = 40;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Size", meta = (ClampMin = "1"))
    int32 RegionSizeMax = 80;

    // Every tile in the region starts with this terrain. Layers then carve
    // connected sub-areas such as clearings, roads or rock formations into it.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region")
    ERoundTerrainType BaseTerrainType = ERoundTerrainType::Ground;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region")
    TArray<FRoundTerrainRegionLayer> Layers;

    // One weighted macro structure is selected before the detail layers.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Structure")
    TArray<FRoundTerrainRegionStructure> Structures;
};

USTRUCT(BlueprintType)
struct FRoundTerrainRegionAdjacency
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region")
    FName RegionA;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region")
    FName RegionB;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region", meta = (ClampMin = "0.0"))
    double Weight = 1.0;
};

UCLASS(BlueprintType)
class ROUND_API URoundTerrainRegionRegistryData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region")
    TMap<FName, TObjectPtr<URoundTerrainRegionData>> Regions;

    // Each unordered pair is stored once and is automatically symmetric.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region")
    TArray<FRoundTerrainRegionAdjacency> Adjacencies;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region")
    FName StartRegionId = TEXT("Start");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region", meta = (ClampMin = "0"))
    int32 StartRegionRadius = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region", meta = (ClampMin = "1"))
    int32 RegionCountMin = 6;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region", meta = (ClampMin = "1"))
    int32 RegionCountMax = 6;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region", meta = (ClampMin = "1"))
    int32 MinimumSeedDistance = 4;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region", meta = (ClampMin = "0.0"))
    double OffMapGrowthWeight = 0.35;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Connectivity")
    bool bEnsureWalkableConnectivity = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Connectivity")
    bool bEnsureStartPositionsConnected = true;

    // If blocking terrain separates walkable areas, the generator carves the
    // cheapest narrow connection using this editor-configured terrain type.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain Region|Connectivity")
    ERoundTerrainType ConnectivityPassTerrainType = ERoundTerrainType::Rubble;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain Region")
    URoundTerrainRegionData* GetRegion(FName RegionId) const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain Region")
    double GetAdjacencyWeight(FName RegionA, FName RegionB) const;

    bool Validate(FString& OutErrors) const;
};
