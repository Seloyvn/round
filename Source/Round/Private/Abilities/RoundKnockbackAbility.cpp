#include "Abilities/RoundKnockbackAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Game/RoundGameState.h"

namespace
{
ARoundGridTile* FindKnockbackTile(ARoundGameState* GameState, ARoundGridTile* SourceTile, ARoundGridTile* TargetTile)
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
        if (!Neighbor || Neighbor->OccupyingCharacter)
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

FRoundActionResult ARoundKnockbackAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 6)
    {
        return ARoundAbilityActor::ExecuteAbility(TargetTile);
    }
    FRoundActionResult Result;
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes || !TargetTile || !OwningCharacter->CurrentTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    ARoundGameState* RoundGameState = OwningCharacter->GetRoundGameState();
    if (!TargetCharacter || !RoundGameState)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    if (!OwningCharacter->Attributes->SpendInitiative(GetAbilityCost()))
    {
        Result.Reason = TEXT("Not enough initiative.");
        return Result;
    }

    SetCooldown(GetAbilityCooldown());

    bool bDidDie = false;
    int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    const int32 FinalDamage = OwningCharacter->ModifyOutgoingDamage(GetAbilityDamage(), GetAbilityDamageType(), TargetCharacter, DefenseIgnore);
    TargetCharacter->ApplyRoundDamage(FinalDamage, GetAbilityDamageType(), DefenseIgnore, OwningCharacter, bDidDie);

    if (!bDidDie)
    {
        const int32 KnockbackDistance = FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("KnockbackDistance"))));
        ARoundGridTile* CurrentTargetTile = TargetCharacter->CurrentTile;
        for (int32 Step = 0; Step < KnockbackDistance && CurrentTargetTile; ++Step)
        {
            ARoundGridTile* NextTile = FindKnockbackTile(RoundGameState, OwningCharacter->CurrentTile, CurrentTargetTile);
            if (!NextTile)
            {
                break;
            }

            if (!TargetCharacter->TryForcedSetField(NextTile, OwningCharacter, true))
            {
                break;
            }

            CurrentTargetTile = TargetCharacter->CurrentTile;
        }
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Knockback executed.");
    return Result;
}
