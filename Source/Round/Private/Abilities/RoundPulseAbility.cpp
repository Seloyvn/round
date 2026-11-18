#include "Abilities/RoundPulseAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Game/RoundGameState.h"
#include "Effects/RoundShieldEffect.h"

namespace
{
ARoundGridTile* FindPulseKnockbackTile(ARoundGameState* GameState, ARoundGridTile* CenterTile, ARoundGridTile* EnemyTile)
{
    if (!GameState || !CenterTile || !EnemyTile)
    {
        return nullptr;
    }

    const FVector2D Direction = EnemyTile->GridPosition - CenterTile->GridPosition;
    ARoundGridTile* BestTile = nullptr;
    double BestDot = -DBL_MAX;

    for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(EnemyTile))
    {
        if (!Neighbor || Neighbor->OccupyingCharacter)
        {
            continue;
        }

        const FVector2D NeighborDirection = Neighbor->GridPosition - EnemyTile->GridPosition;
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

void ARoundPulseAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    if (!TargetTile || !OwningCharacter)
    {
        return;
    }

    OutTiles.AddUnique(const_cast<ARoundGridTile*>(TargetTile));
    if (const ARoundGameState* GameState = OwningCharacter->GetRoundGameState())
    {
        for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(const_cast<ARoundGridTile*>(TargetTile)))
        {
            if (Neighbor)
            {
                OutTiles.AddUnique(Neighbor);
            }
        }
    }
}

FRoundActionResult ARoundPulseAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundCharacter* CenterCharacter = ResolveTargetCharacter(TargetTile);
    ARoundGameState* RoundGameState = OwningCharacter->GetRoundGameState();
    if (!CenterCharacter || !RoundGameState)
    {
        Result.Reason = TEXT("No target.");
        return Result;
    }

    if (!OwningCharacter->Attributes->SpendInitiative(GetAbilityCost()))
    {
        Result.Reason = TEXT("Not enough initiative.");
        return Result;
    }

    SetCooldown(GetAbilityCooldown());

    TArray<ARoundGridTile*> NeighborTiles = RoundGameState->GetNeighborFields(TargetTile);
    int32 TargetsHit = 0;
    for (ARoundGridTile* NeighborTile : NeighborTiles)
    {
        ARoundCharacter* EnemyCharacter = NeighborTile ? NeighborTile->OccupyingCharacter : nullptr;
        if (!EnemyCharacter || !OwningCharacter->IsEnemyFor(EnemyCharacter))
        {
            continue;
        }

        bool bDidDie = false;
        int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
        ApplyAbilityDamageToTarget(EnemyCharacter, GetAbilityDamage(), DefenseIgnore, bDidDie);
        ++TargetsHit;

        if (!bDidDie)
        {
            EnemyCharacter->ApplyDelay(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnHit")))));
            const int32 KnockbackDistance = FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("KnockbackDistance"))));
            ARoundGridTile* CurrentEnemyTile = EnemyCharacter->CurrentTile;
            for (int32 Step = 0; Step < KnockbackDistance && CurrentEnemyTile; ++Step)
            {
                ARoundGridTile* NextTile = FindPulseKnockbackTile(RoundGameState, TargetTile, CurrentEnemyTile);
                if (!NextTile)
                {
                    break;
                }

                if (!EnemyCharacter->TryForcedSetField(NextTile, OwningCharacter, true))
                {
                    break;
                }

                CurrentEnemyTile = EnemyCharacter->CurrentTile;
            }
        }
    }

    const int32 HealCenter = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("HealCenterAlly"))));
    if (HealCenter > 0) CenterCharacter->Heal(HealCenter);
    const int32 ShieldAmount = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ShieldCenterAlly"))));
    if (ShieldAmount > 0 && GetWorld())
    {
        if (ARoundShieldEffect* Shield = GetWorld()->SpawnActor<ARoundShieldEffect>(ARoundShieldEffect::StaticClass()))
        {
            Shield->EffectId = TEXT("PulseShield"); Shield->DisplayName = TEXT("Pulse Shield"); Shield->CauserCharacter = OwningCharacter;
            Shield->ShieldAmountRemaining = ShieldAmount; Shield->SetDurationInitiative(50, false); CenterCharacter->AddEffect(Shield);
        }
    }

    NotifyAllyAffectedBySpell(CenterCharacter);
    Result.bSuccess = true;
    Result.Reason = FString::Printf(TEXT("Pulse hit %d target(s)."), TargetsHit);
    return Result;
}
