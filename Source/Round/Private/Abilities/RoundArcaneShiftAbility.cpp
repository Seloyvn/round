#include "Abilities/RoundArcaneShiftAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundModifierEffect.h"
#include "Effects/RoundOneHitProtectionEffect.h"
#include "Game/RoundGameState.h"

bool ARoundArcaneShiftAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    if (Super::CanTargetTile(TargetTile))
    {
        return true;
    }
    return GetAbilityValueFromKey(TEXT("CanSwapWithAlly")) > 0.0
        && OwningCharacter
        && OwningCharacter->CurrentTile
        && TargetTile
        && TargetTile->OccupyingCharacter
        && TargetTile->OccupyingCharacter->Team == OwningCharacter->Team
        && OwningCharacter->CurrentTile->GetDistance(TargetTile) <= GetAbilityRange();
}

FRoundActionResult ARoundArcaneShiftAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes || !TargetTile)
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

    ARoundGridTile* OriginTile = OwningCharacter->CurrentTile;
    ARoundCharacter* SwapAlly = TargetTile->OccupyingCharacter;
    if (SwapAlly && SwapAlly != OwningCharacter)
    {
        // Free the caster's origin first so both SetField calls retain their
        // invariant that a destination may never contain another character.
        if (OriginTile && OriginTile->OccupyingCharacter == OwningCharacter)
        {
            OriginTile->OccupyingCharacter = nullptr;
            OriginTile->ForceNetUpdate();
        }
        SwapAlly->SetField(OriginTile, true);
    }
    OwningCharacter->SetField(TargetTile, true);

    ARoundGameState* GameState = OwningCharacter->GetRoundGameState();
    const int32 StartInitiativeLoss = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossAroundStart"))));
    const int32 SpatialDamage = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("SpatialRuptureDamage"))));
    TArray<ARoundCharacter*> SpatialTargets;
    auto AffectAdjacentEnemies = [&](ARoundGridTile* Center, const bool bAtStart)
    {
        if (!GameState || !Center)
        {
            return;
        }
        for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(Center))
        {
            ARoundCharacter* Enemy = Neighbor ? Neighbor->OccupyingCharacter : nullptr;
            if (!Enemy || !OwningCharacter->IsEnemyFor(Enemy))
            {
                continue;
            }
            if (bAtStart && StartInitiativeLoss > 0)
            {
                Enemy->ApplyDelay(StartInitiativeLoss);
            }
            if (SpatialDamage > 0 && !SpatialTargets.Contains(Enemy))
            {
                bool bDidDie = false;
                ApplyAbilityDamageToTarget(Enemy, SpatialDamage, 0, bDidDie);
                SpatialTargets.Add(Enemy);
            }
            const int32 TrailMoveCost = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MoveCostAddNearShiftEndpoints"))));
            if (TrailMoveCost > 0 && Enemy->Attributes && Enemy->Attributes->IsAlive())
            {
                if (ARoundModifierEffect* Trail = GetWorld()->SpawnActor<ARoundModifierEffect>())
                {
                    Trail->EffectId = TEXT("ArcaneShiftTrail");
                    Trail->DisplayName = TEXT("Phasenspur");
                    Trail->Description = TEXT("Bewegung kostet mehr Initiative.");
                    Trail->MoveCostAdd = TrailMoveCost;
                    Trail->bTreatAsDebuff = true;
                    Trail->CauserCharacter = OwningCharacter;
                    Trail->SetDurationInitiative(FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ShiftEndpointDebuffDurationInitiative")))), false);
                    Enemy->AddEffect(Trail);
                }
            }
        }
    };
    AffectAdjacentEnemies(OriginTile, true);
    AffectAdjacentEnemies(TargetTile, false);

    const int32 ArrivalDamage = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DamageAroundDestination"))));
    int32 EnemiesAtDestination = 0;
    if (GameState)
    {
        for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(TargetTile))
        {
            ARoundCharacter* Enemy = Neighbor ? Neighbor->OccupyingCharacter : nullptr;
            if (Enemy && OwningCharacter->IsEnemyFor(Enemy))
            {
                ++EnemiesAtDestination;
                if (ArrivalDamage > 0)
                {
                    bool bDidDie = false;
                    ApplyAbilityDamageToTarget(Enemy, ArrivalDamage, 0, bDidDie);
                }
            }
        }
    }

    const int32 SafeRefund = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeRefundIfNoAdjacentEnemy"))));
    if (EnemiesAtDestination == 0 && SafeRefund > 0)
    {
        OwningCharacter->Attributes->GainInitiative(SafeRefund);
    }

    double ProtectionMultiplier = GetAbilityValueFromKey(TEXT("NextHitIncomingDamageMultiplier"));
    const bool bNegateNextHit = GetAbilityValueFromKey(TEXT("NegateNextHit")) > 0.0;
    if (bNegateNextHit)
    {
        ProtectionMultiplier = 0.0;
    }
    const int32 ProtectionDuration = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("NextHitProtectionDurationInitiative"))));
    if ((bNegateNextHit || ProtectionMultiplier > 0.0) && ProtectionMultiplier < 1.0 && ProtectionDuration > 0)
    {
        if (ARoundOneHitProtectionEffect* Protection = GetWorld()->SpawnActor<ARoundOneHitProtectionEffect>())
        {
            Protection->EffectId = TEXT("ArcaneShiftProtection");
            Protection->DisplayName = bNegateNextHit ? TEXT("Nachbild") : TEXT("Phasenschutz");
            Protection->Description = TEXT("Der naechste Treffer wird abgeschwaecht.");
            Protection->IncomingDamageMultiplier = ProtectionMultiplier;
            Protection->CauserCharacter = OwningCharacter;
            Protection->SetDurationInitiative(ProtectionDuration, false);
            OwningCharacter->AddEffect(Protection);
        }
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Arcane Shift executed.");
    return Result;
}
