#include "Effects/RoundSlowEffect.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"

int32 ARoundSlowEffect::GetMovecost_Implementation(int32 In) const
{
    int32 Cost = In;
    if (Amount > 0.0)
    {
        Cost = FMath::RoundToInt(static_cast<double>(Cost) * (1.0 + Amount));
    }

    if (bBlockMovementBonuses && OwningCharacter && OwningCharacter->Attributes)
    {
        Cost = FMath::Max(Cost, OwningCharacter->Attributes->BaseStats.MoveCost);
    }

    return FMath::Max(1, Cost);
}

void ARoundSlowEffect::OnOwnerMoved(ARoundCharacter* TargetCharacter, ARoundGridTile* PreviousTile, ARoundGridTile* NewTile)
{
    if (TargetCharacter && PreviousTile && NewTile && PreviousTile != NewTile && InitiativeLossOnMove > 0)
    {
        TargetCharacter->ApplyDelay(InitiativeLossOnMove);
    }
}

void ARoundSlowEffect::OnTurnEnded(ARoundCharacter* TargetCharacter)
{
    const bool bWillExpire = bAutoExpireOnTurnEnd && RemainingTurns == 1 && RemainingInitiative <= 0;
    Super::OnTurnEnded(TargetCharacter);

    if (bWillExpire && TargetCharacter && InitiativeLossOnExpire > 0)
    {
        TargetCharacter->ApplyDelay(InitiativeLossOnExpire);
    }
}
