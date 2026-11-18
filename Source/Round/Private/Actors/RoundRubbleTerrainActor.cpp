#include "Actors/RoundRubbleTerrainActor.h"

ARoundRubbleTerrainActor::ARoundRubbleTerrainActor()
{
    TerrainType = ERoundTerrainType::Rubble;
    TerrainBaseMoveCost = 1;
    bTerrainMoveable = true;
}

int32 ARoundRubbleTerrainActor::GetMoveCost_Implementation(const int32 BaseCost) const
{
    const float Multiplier = TerrainData ? GetTerrainMoveCostMultiplier() : 1.3f;
    return FMath::Max(1, FMath::RoundToInt(static_cast<double>(BaseCost > 0 ? BaseCost : TerrainBaseMoveCost) * static_cast<double>(Multiplier)));
}
