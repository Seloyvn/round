#include "Effects/RoundTemporalAnchorEffect.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Game/RoundGameState.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Net/UnrealNetwork.h"

void ARoundTemporalAnchorEffect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundTemporalAnchorEffect, AnchoredTile);
    DOREPLIFETIME(ARoundTemporalAnchorEffect, HealOnReturn);
    DOREPLIFETIME(ARoundTemporalAnchorEffect, InitiativeGainOnReturn);
    DOREPLIFETIME(ARoundTemporalAnchorEffect, NearbyEnemyInitiativeLoss);
}

void ARoundTemporalAnchorEffect::OnApplied_Implementation(ARoundCharacter* TargetCharacter)
{
    AnchoredTile = TargetCharacter ? TargetCharacter->CurrentTile : nullptr;
}

void ARoundTemporalAnchorEffect::OnInitiativeAdvanced(ARoundCharacter* TargetCharacter, int32 InitiativeAmount)
{
    if (InitiativeAmount <= 0 || RemainingInitiative <= 0)
    {
        return;
    }

    RemainingInitiative = FMath::Max(0, RemainingInitiative - InitiativeAmount);
    if (RemainingInitiative > 0)
    {
        return;
    }

    if (TargetCharacter && AnchoredTile)
    {
        ARoundGridTile* DestinationTile = nullptr;
        if (AnchoredTile == TargetCharacter->CurrentTile || AnchoredTile->IsWalkable())
        {
            DestinationTile = AnchoredTile;
        }
        else if (ARoundGameState* GameState = TargetCharacter->GetRoundGameState())
        {
            for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(AnchoredTile))
            {
                if (Neighbor && Neighbor->IsWalkable())
                {
                    DestinationTile = Neighbor;
                    break;
                }
            }
        }

        if (DestinationTile && DestinationTile != TargetCharacter->CurrentTile)
        {
            TargetCharacter->TryForcedSetField(DestinationTile, CauserCharacter, true);
        }
        TargetCharacter->Heal(HealOnReturn);
        if (TargetCharacter->Attributes) TargetCharacter->Attributes->GainInitiative(InitiativeGainOnReturn);
        if (NearbyEnemyInitiativeLoss > 0)
        {
            if (ARoundGameState* GameState = TargetCharacter->GetRoundGameState())
            {
                for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(TargetCharacter->CurrentTile))
                {
                    ARoundCharacter* Other = Neighbor ? Neighbor->GetResolvedOccupyingCharacter() : nullptr;
                    if (Other && Other->Attributes && TargetCharacter->IsEnemyFor(Other)) Other->Attributes->PassInitiative(NearbyEnemyInitiativeLoss);
                }
            }
        }
    }

    RemoveEffect();
}
