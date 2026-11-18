#include "Actors/RoundThornsTerrainActor.h"

#include "Actors/RoundCharacter.h"

ARoundThornsTerrainActor::ARoundThornsTerrainActor()
{
    TerrainType = ERoundTerrainType::Thorns;
    TerrainBaseMoveCost = 2;
    bTerrainMoveable = true;
}

void ARoundThornsTerrainActor::OnCharacterEntered_Implementation(ARoundCharacter* EnteringCharacter, ARoundGridTile* PreviousTile)
{
    if (!EnteringCharacter)
    {
        return;
    }

    const int32 Damage = TerrainData ? GetOnEnterDamage() : 80;
    if (Damage <= 0)
    {
        return;
    }

    bool bDidDie = false;
    EnteringCharacter->ApplyRoundDamage(Damage, GetOnEnterDamageType(), 0, nullptr, bDidDie);
}
