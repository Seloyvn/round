#include "Actors/RoundGroveTerrainActor.h"

#include "Actors/RoundCharacter.h"

ARoundGroveTerrainActor::ARoundGroveTerrainActor()
{
    TerrainType = ERoundTerrainType::Grove;
    TerrainBaseMoveCost = 1;
    bTerrainMoveable = true;
}

int32 ARoundGroveTerrainActor::ModifyIncomingDamage_Implementation(ARoundCharacter* Defender, ARoundCharacter* SourceCharacter, const int32 Damage, ERoundDamageType DamageType) const
{
    if (!Defender || !Defender->CurrentTile || Defender->CurrentTile != Field)
    {
        return Damage;
    }

    const double Multiplier = TerrainData ? GetIncomingRangedDamageMultiplier() : 0.8f;
    return FMath::RoundToInt(static_cast<double>(Damage) * static_cast<double>(Multiplier));
}
