#include "Actors/RoundStreetTerrainActor.h"

ARoundStreetTerrainActor::ARoundStreetTerrainActor()
{
    TerrainType = ERoundTerrainType::Street;
    TerrainBaseMoveCost = 1;
    bTerrainMoveable = true;
}

int32 ARoundStreetTerrainActor::GetMoveCost_Implementation(const int32 BaseCost) const
{
    const float Multiplier = TerrainData ? GetTerrainMoveCostMultiplier() : 0.7f;
    return FMath::Max(1, FMath::RoundToInt(static_cast<double>(BaseCost > 0 ? BaseCost : TerrainBaseMoveCost) * static_cast<double>(Multiplier)));
}
