#include "Actors/RoundRocksTerrainActor.h"

ARoundRocksTerrainActor::ARoundRocksTerrainActor()
{
    TerrainType = ERoundTerrainType::Rocks;
    TerrainBaseMoveCost = MAX_int32;
    bTerrainMoveable = false;
}
