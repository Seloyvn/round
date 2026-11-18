#include "Data/RoundTerrainRegionData.h"

URoundTerrainRegionData* URoundTerrainRegionRegistryData::GetRegion(const FName RegionId) const
{
    if (const TObjectPtr<URoundTerrainRegionData>* Found = Regions.Find(RegionId))
    {
        return Found->Get();
    }
    return nullptr;
}

double URoundTerrainRegionRegistryData::GetAdjacencyWeight(const FName RegionA, const FName RegionB) const
{
    if (RegionA.IsNone() || RegionB.IsNone())
    {
        return 0.0;
    }

    for (const FRoundTerrainRegionAdjacency& Entry : Adjacencies)
    {
        if ((Entry.RegionA == RegionA && Entry.RegionB == RegionB)
            || (Entry.RegionA == RegionB && Entry.RegionB == RegionA))
        {
            return FMath::Max(0.0, Entry.Weight);
        }
    }

    return 0.0;
}

bool URoundTerrainRegionRegistryData::Validate(FString& OutErrors) const
{
    TArray<FString> Errors;
    if (!Regions.Contains(StartRegionId))
    {
        Errors.Add(FString::Printf(TEXT("Start region '%s' is missing."), *StartRegionId.ToString()));
    }

    for (const TPair<FName, TObjectPtr<URoundTerrainRegionData>>& Pair : Regions)
    {
        if (!Pair.Value) continue;
        if (Pair.Value->bGlobalFeature && Pair.Value->bCanGenerate)
        {
            Errors.Add(FString::Printf(TEXT("Region '%s' cannot be both a global feature and a generated biome."), *Pair.Key.ToString()));
        }
        if (Pair.Value->RegionSizeMin < 1 || Pair.Value->RegionSizeMax < Pair.Value->RegionSizeMin)
        {
            Errors.Add(FString::Printf(TEXT("Region '%s' has invalid size range %d-%d."),
                *Pair.Key.ToString(), Pair.Value->RegionSizeMin, Pair.Value->RegionSizeMax));
        }
        for (const FRoundTerrainRegionStructure& Structure : Pair.Value->Structures)
        {
            if (Structure.Weight <= 0.0 || Structure.Coverage <= 0.0)
            {
                Errors.Add(FString::Printf(TEXT("Region '%s' contains a structure with non-positive weight or coverage."), *Pair.Key.ToString()));
            }
            if (Structure.ClusterCountMin < 1 || Structure.ClusterCountMax < Structure.ClusterCountMin)
            {
                Errors.Add(FString::Printf(TEXT("Region '%s' contains a structure with invalid cluster range %d-%d."),
                    *Pair.Key.ToString(), Structure.ClusterCountMin, Structure.ClusterCountMax));
            }
        }
        for (const FRoundTerrainRegionLayer& Layer : Pair.Value->Layers)
        {
            for (const FName StartId : Layer.PathStartRegionIds)
            {
                if (!Regions.Contains(StartId))
                {
                    Errors.Add(FString::Printf(TEXT("Region '%s' path layer '%s' references missing start region '%s'."),
                        *Pair.Key.ToString(), *Layer.LayerId.ToString(), *StartId.ToString()));
                }
            }
            for (const FName EndId : Layer.PathEndRegionIds)
            {
                if (!Regions.Contains(EndId))
                {
                    Errors.Add(FString::Printf(TEXT("Region '%s' path layer '%s' references missing end region '%s'."),
                        *Pair.Key.ToString(), *Layer.LayerId.ToString(), *EndId.ToString()));
                }
            }
            if (Layer.MaximumUncrossedPathFields > 0
                && Layer.MaximumUncrossedPathFields < Layer.MinimumCrossingGap)
            {
                Errors.Add(FString::Printf(TEXT("Region '%s' crossing layer '%s' has maximum gap %d below minimum gap %d."),
                    *Pair.Key.ToString(), *Layer.LayerId.ToString(), Layer.MaximumUncrossedPathFields, Layer.MinimumCrossingGap));
            }
        }
    }

    TSet<FString> SeenPairs;
    for (const FRoundTerrainRegionAdjacency& Entry : Adjacencies)
    {
        if (!Regions.Contains(Entry.RegionA) || !Regions.Contains(Entry.RegionB))
        {
            Errors.Add(FString::Printf(TEXT("Adjacency '%s' <-> '%s' references a missing region."), *Entry.RegionA.ToString(), *Entry.RegionB.ToString()));
        }
        const FString A = Entry.RegionA.ToString();
        const FString B = Entry.RegionB.ToString();
        const FString PairKey = A < B ? A + TEXT("|") + B : B + TEXT("|") + A;
        if (SeenPairs.Contains(PairKey))
        {
            Errors.Add(FString::Printf(TEXT("Adjacency '%s' is defined more than once."), *PairKey));
        }
        SeenPairs.Add(PairKey);
    }

    OutErrors = FString::Join(Errors, TEXT("\n"));
    return Errors.Num() == 0;
}
