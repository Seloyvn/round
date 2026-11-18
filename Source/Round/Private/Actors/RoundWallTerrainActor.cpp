#include "Actors/RoundWallTerrainActor.h"

ARoundWallTerrainActor::ARoundWallTerrainActor()
{
    TerrainType = ERoundTerrainType::Wall;
    TerrainBaseMoveCost = MAX_int32;
    bTerrainMoveable = false;
}
