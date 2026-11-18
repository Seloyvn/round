#include "Actors/RoundForestTerrainActor.h"

#include "Actors/RoundCharacter.h"

ARoundForestTerrainActor::ARoundForestTerrainActor()
{
    TerrainType = ERoundTerrainType::Forest;
    TerrainBaseMoveCost = 1;
    bTerrainMoveable = true;
}

int32 ARoundForestTerrainActor::ModifyIncomingDamage_Implementation(ARoundCharacter* Defender, ARoundCharacter* SourceCharacter, const int32 Damage, ERoundDamageType DamageType) const
{
    if (!Defender || !Defender->CurrentTile || Defender->CurrentTile != Field || !SourceCharacter || !SourceCharacter->CurrentTile)
    {
        return Damage;
    }

    if (SourceCharacter->GetPassiveValueFromKey(TEXT("IgnoreForestDamageReduction")) > 0.0)
    {
        return Damage;
    }

    const float Multiplier = TerrainData ? GetIncomingRangedDamageMultiplier() : 0.9f;
    return SourceCharacter->CurrentTile->GetDistance(Field) >= 1
        ? FMath::RoundToInt(static_cast<double>(Damage) * static_cast<double>(Multiplier))
        : Damage;
}
