#include "Abilities/RoundLightningstrikeAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundStaticChargeEffect.h"
#include "Game/RoundGameState.h"

void ARoundLightningstrikeAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    if (!TargetTile || !OwningCharacter || !OwningCharacter->CurrentTile)
    {
        return;
    }

    if (const ARoundGameState* GameState = OwningCharacter->GetRoundGameState())
    {
        OutTiles.Append(GameState->GetFieldsBetween(OwningCharacter->CurrentTile, const_cast<ARoundGridTile*>(TargetTile), true));
    }
}

FRoundActionResult ARoundLightningstrikeAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes || !TargetTile || !OwningCharacter->CurrentTile)
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

    const ARoundGameState* GameState = OwningCharacter->GetRoundGameState();
    TArray<ARoundGridTile*> LineTiles = GameState ? GameState->GetFieldsBetween(OwningCharacter->CurrentTile, TargetTile, true) : TArray<ARoundGridTile*>();
    LineTiles.Remove(OwningCharacter->CurrentTile);

    TArray<ARoundCharacter*> HitTargets;
    int32 HitObjects = 0;
    const double PerPreviousTargetMultiplier = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("DamageMultiplierAddPerPreviousTarget")));
    for (ARoundGridTile* Tile : LineTiles)
    {
        if (!Tile)
        {
            continue;
        }

        int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
        int32 Damage = GetAbilityDamage();
        Damage = FMath::RoundToInt(Damage * (1.0 + PerPreviousTargetMultiplier * HitTargets.Num()));
        if (Tile == LineTiles.Last())
        {
            const double LastTargetMultiplier = GetAbilityValueFromKey(TEXT("DamageMultiplierOnLastTarget"));
            if (LastTargetMultiplier > 0.0)
            {
                Damage = FMath::RoundToInt(static_cast<double>(Damage) * LastTargetMultiplier);
            }
        }

        if (HitTargets.Num() == 0 && LineTiles.Num() == 1)
        {
            const double SingleTargetMultiplier = GetAbilityValueFromKey(TEXT("DamageMultiplierIfSingleTarget"));
            if (SingleTargetMultiplier > 0.0)
            {
                Damage = FMath::RoundToInt(static_cast<double>(Damage) * SingleTargetMultiplier);
            }
        }

        ARoundCharacter* TargetCharacter = Tile->OccupyingCharacter;
        if (TargetCharacter && OwningCharacter->IsEnemyFor(TargetCharacter))
        {
            bool bAnyDestroyed = false;
            const int32 HitsOnTile = ApplyAbilityDamageToTile(Tile, Damage, DefenseIgnore, bAnyDestroyed, true);
            if (HitsOnTile > 0)
            {
                HitTargets.AddUnique(TargetCharacter);
                HitObjects += FMath::Max(0, HitsOnTile - 1);
            }

            const int32 InitiativeLoss = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnHit"))));
            if (InitiativeLoss > 0)
            {
                TargetCharacter->ApplyDelay(InitiativeLoss);
            }

            const int32 StaticDamage = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("StaticChargeMoveDamage"))));
            const int32 StaticDuration = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("StaticChargeDurationInitiative"))));
            if (StaticDamage > 0 && StaticDuration > 0 && TargetCharacter->Attributes && TargetCharacter->Attributes->IsAlive())
            {
                if (ARoundStaticChargeEffect* Static = GetWorld()->SpawnActor<ARoundStaticChargeEffect>())
                {
                    Static->EffectId = TEXT("LightningStaticCharge");
                    Static->DisplayName = TEXT("Statische Ladung");
                    Static->Description = TEXT("Die naechste Bewegung loest Blitzschaden aus.");
                    Static->Amount = StaticDamage;
                    Static->CauserCharacter = OwningCharacter;
                    Static->SetDurationInitiative(StaticDuration, false);
                    TargetCharacter->AddEffect(Static);
                }
            }
        }
        else
        {
            bool bAnyDestroyed = false;
            HitObjects += ApplyAbilityDamageToTile(Tile, Damage, DefenseIgnore, bAnyDestroyed, false);
        }
    }


    const double ForkMultiplier = GetAbilityValueFromKey(TEXT("EndpointForkDamageMultiplier"));
    if (ForkMultiplier > 0.0 && LineTiles.Num() > 0 && GameState)
    {
        for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(LineTiles.Last()))
        {
            ARoundCharacter* ForkTarget = Neighbor ? Neighbor->OccupyingCharacter : nullptr;
            if (!ForkTarget || HitTargets.Contains(ForkTarget) || !OwningCharacter->IsEnemyFor(ForkTarget))
            {
                continue;
            }
            bool bDidDie = false;
            ApplyAbilityDamageToTarget(ForkTarget, FMath::RoundToInt(GetAbilityDamage() * ForkMultiplier), 0, bDidDie);
            HitTargets.AddUnique(ForkTarget);
        }
    }

    const int32 ChainRange = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("FinalChainRange"))));
    const double ChainMultiplier = GetAbilityValueFromKey(TEXT("FinalChainDamageMultiplier"));
    if (ChainRange > 0 && ChainMultiplier > 0.0 && HitTargets.Num() > 0 && HitTargets.Last()->CurrentTile)
    {
        ARoundCharacter* ChainTarget = nullptr;
        int32 BestDistance = MAX_int32;
        ARoundGridTile* ChainOrigin = HitTargets.Last()->CurrentTile;
        for (ARoundGridTile* Tile : ChainOrigin->GetDistanceFields(ChainRange))
        {
            ARoundCharacter* Candidate = Tile ? Tile->OccupyingCharacter : nullptr;
            const int32 Distance = Tile ? ChainOrigin->GetDistance(Tile) : MAX_int32;
            if (Candidate && !HitTargets.Contains(Candidate) && OwningCharacter->IsEnemyFor(Candidate) && Distance < BestDistance)
            {
                ChainTarget = Candidate;
                BestDistance = Distance;
            }
        }
        if (ChainTarget)
        {
            bool bDidDie = false;
            ApplyAbilityDamageToTarget(ChainTarget, FMath::RoundToInt(GetAbilityDamage() * ChainMultiplier), 0, bDidDie);
            HitTargets.AddUnique(ChainTarget);
        }
    }

    const int32 InitiativeGain = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainOnMultiHit"))));
    if (InitiativeGain > 0 && HitTargets.Num() >= 2)
    {
        OwningCharacter->Attributes->GainInitiative(InitiativeGain);
    }

    const int32 CooldownReductionPerTarget = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionPerEnemyHit"))));
    const int32 MaximumCooldownReduction = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MaximumHitCooldownReduction"))));
    if (CooldownReductionPerTarget > 0)
    {
        ReduceCooldown(MaximumCooldownReduction > 0
            ? FMath::Min(MaximumCooldownReduction, CooldownReductionPerTarget * HitTargets.Num())
            : CooldownReductionPerTarget * HitTargets.Num());
    }

    Result.bSuccess = true;
    Result.Reason = FString::Printf(TEXT("Lightningstrike hit %d target(s) and %d object(s)."), HitTargets.Num(), HitObjects);
    return Result;
}
