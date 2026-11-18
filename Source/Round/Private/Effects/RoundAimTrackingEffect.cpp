#include "Effects/RoundAimTrackingEffect.h"
#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"

void ARoundAimTrackingEffect::OnOwnerMoved(ARoundCharacter* Target, ARoundGridTile* Previous, ARoundGridTile* Next)
{
    if (!Target || Previous == Next) return;
    bool bDidDie = false;
    Target->ApplyRoundDamage(TrackingDamage, DamageType, 0, CauserCharacter, bDidDie);
    RemoveEffect();
}
