#include "Actors/RoundGroundTerrainActor.h"

ARoundGroundTerrainActor::ARoundGroundTerrainActor()
{
    TerrainType = ERoundTerrainType::Ground;
    TerrainBaseMoveCost = 1;
    bTerrainMoveable = true;
}
