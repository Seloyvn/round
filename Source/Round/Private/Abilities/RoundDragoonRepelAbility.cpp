#include "Abilities/RoundDragoonRepelAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacterDragoon.h"
#include "Actors/RoundGridTile.h"
#include "Game/RoundGameState.h"
#include "Components/RoundAttributeSetComponent.h"

namespace
{
ARoundGridTile* FindRepelTile(ARoundGameState* GameState, ARoundGridTile* SourceTile, ARoundGridTile* TargetTile)
{
    if (!GameState || !SourceTile || !TargetTile)
    {
        return nullptr;
    }

    const FVector2D Direction = TargetTile->GridPosition - SourceTile->GridPosition;
    ARoundGridTile* BestTile = nullptr;
    double BestDot = -DBL_MAX;

    for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(TargetTile))
    {
        if (!Neighbor || Neighbor->OccupyingCharacter || Neighbor->HasBlockingFieldObject() || !Neighbor->IsMoveable())
        {
            continue;
        }

        const FVector2D NeighborDirection = Neighbor->GridPosition - TargetTile->GridPosition;
        const double Dot = FVector2D::DotProduct(Direction, NeighborDirection);
        if (Dot > BestDot)
        {
            BestDot = Dot;
            BestTile = Neighbor;
        }
    }

    return BestTile;
}
}

bool ARoundDragoonRepelAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    return OwningCharacter
        && OwningCharacter->CurrentTile
        && TargetTile
        && ResolveTargetCharacter(TargetTile)
        && OwningCharacter->IsEnemyFor(ResolveTargetCharacter(TargetTile))
        && IsTargetInRange(TargetTile)
        && GetAbilityVisibility() == ERoundButtonState::Enabled;
}

FRoundActionResult ARoundDragoonRepelAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterDragoon* Dragoon = Cast<ARoundCharacterDragoon>(OwningCharacter);
    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    ARoundGameState* GameState = OwningCharacter ? OwningCharacter->GetRoundGameState() : nullptr;
    if (!Dragoon || !TargetCharacter || !TargetCharacter->CurrentTile || !GameState)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    bool bDidDie = false;
    const int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    const int32 AppliedDamage = ApplyAbilityDamageToTarget(TargetCharacter, GetAbilityDamage(), DefenseIgnore, bDidDie);
    Dragoon->RecordEnemyHit(TargetCharacter, AppliedDamage, GetAbilityDamageType());

    if (!bDidDie)
    {
        const int32 PushDistance = FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("PushDistance"))));
        ARoundGridTile* CurrentTargetTile = TargetCharacter->CurrentTile;
        int32 PushedSteps = 0;
        for (int32 Step = 0; Step < PushDistance && CurrentTargetTile; ++Step)
        {
            ARoundGridTile* NextTile = FindRepelTile(GameState, Dragoon->CurrentTile, CurrentTargetTile);
            if (!NextTile)
            {
                break;
            }

            if (!TargetCharacter->TryForcedSetField(NextTile, OwningCharacter, true))
            {
                break;
            }

            CurrentTargetTile = TargetCharacter->CurrentTile;
            ++PushedSteps;
        }
        if (TargetCharacter->Attributes) TargetCharacter->Attributes->PassInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnHit")))));
        if (PushedSteps < PushDistance)
        {
            bool bCollisionKill = false;
            TargetCharacter->ApplyRoundDamage(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CollisionDamage")))), GetAbilityDamageType(), DefenseIgnore, Dragoon, bCollisionKill);
        }
    }
    if (Dragoon->Attributes) Dragoon->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("SelfInitiativeGain")))));
    Dragoon->GainMomentumStacks(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BonusMomentumOnHit")))));

    Result.bSuccess = true;
    Result.Reason = TEXT("Repel executed.");
    return Result;
}
