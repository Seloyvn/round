#pragma once

#include "RoundTypes.h"

namespace RoundTerrain
{
inline bool IsNatureTerrain(const ERoundTerrainType TerrainType)
{
    return TerrainType == ERoundTerrainType::Forest
        || TerrainType == ERoundTerrainType::Thorns
        || TerrainType == ERoundTerrainType::Grove;
}

inline bool IsRockLikeTerrain(const ERoundTerrainType TerrainType)
{
    return TerrainType == ERoundTerrainType::Rocks
        || TerrainType == ERoundTerrainType::Rubble;
}
}
