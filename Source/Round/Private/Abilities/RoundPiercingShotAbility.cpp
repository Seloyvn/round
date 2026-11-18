#include "Abilities/RoundPiercingShotAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter2.h"
#include "Actors/RoundEffectActor.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Game/RoundGameState.h"

namespace
{
ARoundGridTile* FindForwardTile(ARoundGameState* GameState, ARoundGridTile* Current, const FVector2D& Direction, const TSet<ARoundGridTile*>& Visited)
{
    ARoundGridTile* Best = nullptr;
    double BestDot = -DBL_MAX;
    for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(Current))
    {
        if (!Neighbor || Visited.Contains(Neighbor)) continue;
        const double Dot = FVector2D::DotProduct(Direction.GetSafeNormal(), (Neighbor->GridPosition - Current->GridPosition).GetSafeNormal());
        if (Dot > BestDot) { BestDot = Dot; Best = Neighbor; }
    }
    return BestDot > 0.25 ? Best : nullptr;
}

ARoundGridTile* FindPiercingKnockbackTile(ARoundGameState* GameState, ARoundGridTile* Source, ARoundGridTile* Target)
{
    if (!GameState || !Source || !Target) return nullptr;
    const FVector2D Direction = Target->GridPosition - Source->GridPosition;
    ARoundGridTile* Best = nullptr;
    double BestDot = -DBL_MAX;
    for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(Target))
    {
        if (!Neighbor || !Neighbor->IsMoveable()) continue;
        const double Dot = FVector2D::DotProduct(Direction, Neighbor->GridPosition - Target->GridPosition);
        if (Dot > BestDot) { BestDot = Dot; Best = Neighbor; }
    }
    return Best;
}
}

void ARoundPiercingShotAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    if (!TargetTile || !OwningCharacter || !OwningCharacter->CurrentTile) return;
    if (const ARoundGameState* GameState = OwningCharacter->GetRoundGameState())
    {
        OutTiles.Append(GameState->GetFieldsBetween(OwningCharacter->CurrentTile, const_cast<ARoundGridTile*>(TargetTile), true));
    }
}

FRoundActionResult ARoundPiercingShotAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes || !TargetTile || !OwningCharacter->CurrentTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundCharacter2* AmmoCharacter = Cast<ARoundCharacter2>(OwningCharacter);
    ARoundGameState* GameState = OwningCharacter->GetRoundGameState();
    if (!AmmoCharacter || !AmmoCharacter->HasAmmo() || !GameState)
    {
        Result.Reason = TEXT("No ammo.");
        return Result;
    }
    if (!OwningCharacter->Attributes->SpendInitiative(GetAbilityCost()))
    {
        Result.Reason = TEXT("Not enough initiative.");
        return Result;
    }
    SetCooldown(GetAbilityCooldown());

    TArray<ARoundGridTile*> FirstPath;
    TArray<ARoundCharacter*> AllHitTargets;
    int32 TotalHitObjects = 0;
    int32 TotalHitInstances = 0;
    bool bKilledAnyTarget = false;

    auto FirePath = [&](double DamageMultiplier, const TArray<ARoundGridTile*>* FixedPath)
    {
        TArray<ARoundGridTile*> Path = FixedPath ? *FixedPath : GameState->GetFieldsBetween(OwningCharacter->CurrentTile, TargetTile, true);
        Path.Remove(OwningCharacter->CurrentTile);
        const bool bResetRange = !FixedPath && GetAbilityValueFromKey(TEXT("ResetRangeOnEveryHit")) > 0.0;
        const bool bRedirect = !FixedPath && GetAbilityValueFromKey(TEXT("RedirectAfterFirstHit")) > 0.0;
        const int32 FullRange = FMath::Max(1, GetAbilityRange());
        int32 RemainingRange = FixedPath ? Path.Num() : FullRange;
        FVector2D Direction = TargetTile->GridPosition - OwningCharacter->CurrentTile->GridPosition;
        ARoundGridTile* Current = OwningCharacter->CurrentTile;
        TSet<ARoundGridTile*> Visited;
        TSet<ARoundCharacter*> SplashVictims;
        bool bRedirected = false;
        int32 CharacterHitsThisPass = 0;
        int32 PathIndex = 0;

        while (RemainingRange > 0)
        {
            ARoundGridTile* Tile = nullptr;
            if (FixedPath || !bResetRange)
            {
                if (!Path.IsValidIndex(PathIndex)) break;
                Tile = Path[PathIndex++];
            }
            else
            {
                Tile = FindForwardTile(GameState, Current, Direction, Visited);
                if (!Tile) break;
            }
            if (!Tile || Visited.Contains(Tile)) break;
            Visited.Add(Tile);
            if (!FixedPath) FirstPath.Add(Tile);
            Current = Tile;
            --RemainingRange;

            ARoundCharacter* TargetCharacter = Tile->OccupyingCharacter;
            const bool bEnemyCharacter = TargetCharacter && OwningCharacter->IsEnemyFor(TargetCharacter);
            int32 Damage = FMath::RoundToInt(static_cast<double>(GetAbilityDamage()) * DamageMultiplier);
            if (bEnemyCharacter && CharacterHitsThisPass > 0)
            {
                const double LaterMultiplier = GetAbilityValueFromKey(TEXT("LaterTargetDamageMultiplier"));
                if (LaterMultiplier > 0.0) Damage = FMath::RoundToInt(static_cast<double>(Damage) * LaterMultiplier);
            }

            bool bAnyDestroyed = false;
            const int32 HitsOnTile = ApplyAbilityDamageToTile(Tile, Damage,
                FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore")))), bAnyDestroyed, bEnemyCharacter);
            if (HitsOnTile > 0)
            {
                TotalHitInstances += HitsOnTile;
                if (bEnemyCharacter)
                {
                    ++CharacterHitsThisPass;
                    AllHitTargets.AddUnique(TargetCharacter);
                    bKilledAnyTarget = bKilledAnyTarget || !TargetCharacter->IsAlive();
                    const int32 InitiativeLoss = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnHit"))));
                    if (InitiativeLoss > 0 && TargetCharacter->IsAlive()) TargetCharacter->ApplyDelay(InitiativeLoss);
                    if (CharacterHitsThisPass == 1 && TargetCharacter->IsAlive() && GetAbilityValueFromKey(TEXT("KnockbackFirstTargetDistance")) > 0.0)
                    {
                        if (ARoundGridTile* PushTile = FindPiercingKnockbackTile(GameState, OwningCharacter->CurrentTile, Tile)) TargetCharacter->TryForcedSetField(PushTile, OwningCharacter, true);
                    }
                }
                TotalHitObjects += FMath::Max(0, HitsOnTile - (bEnemyCharacter ? 1 : 0));
                if (bResetRange) RemainingRange = FullRange;
            }

            if (bAnyDestroyed)
            {
                const double SplashMultiplier = GetAbilityValueFromKey(TEXT("DestroyedObjectSplashDamageMultiplier"));
                if (SplashMultiplier > 0.0)
                {
                    for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(Tile))
                    {
                        ARoundCharacter* SplashTarget = Neighbor ? Neighbor->OccupyingCharacter : nullptr;
                        if (!SplashTarget || !OwningCharacter->IsEnemyFor(SplashTarget) || SplashVictims.Contains(SplashTarget)) continue;
                        SplashVictims.Add(SplashTarget);
                        bool bDied = false;
                        ApplyAbilityDamageToTarget(SplashTarget, FMath::RoundToInt(GetAbilityDamage() * DamageMultiplier * SplashMultiplier), 0, bDied);
                        bKilledAnyTarget = bKilledAnyTarget || bDied;
                    }
                }
            }

            if (bRedirect && !bRedirected && HitsOnTile > 0)
            {
                ARoundCharacter* Nearest = nullptr;
                int32 NearestDistance = MAX_int32;
                for (ARoundGridTile* CandidateTile : GameState->Fields)
                {
                    ARoundCharacter* Candidate = CandidateTile ? CandidateTile->OccupyingCharacter : nullptr;
                    const int32 Distance = CandidateTile ? Tile->GetDistance(CandidateTile) : MAX_int32;
                    if (Candidate && OwningCharacter->IsEnemyFor(Candidate) && !AllHitTargets.Contains(Candidate) && Distance <= RemainingRange && Distance < NearestDistance)
                    {
                        Nearest = Candidate;
                        NearestDistance = Distance;
                    }
                }
                if (Nearest && Nearest->CurrentTile)
                {
                    Path = GameState->GetFieldsBetween(Tile, Nearest->CurrentTile, true);
                    Path.Remove(Tile);
                    PathIndex = 0;
                    Direction = Nearest->CurrentTile->GridPosition - Tile->GridPosition;
                }
                bRedirected = true;
            }
        }
    };

    FirePath(1.0, nullptr);
    AmmoCharacter->ResolveAmmoShot(bKilledAnyTarget);
    const double RepeatMultiplier = GetAbilityValueFromKey(TEXT("RepeatShotDamageMultiplier"));
    if (RepeatMultiplier > 0.0 && AmmoCharacter->HasAmmo())
    {
        FirePath(RepeatMultiplier, &FirstPath);
        AmmoCharacter->ResolveAmmoShot(bKilledAnyTarget);
    }

    const double ChainMultiplier = GetAbilityValueFromKey(TEXT("ChainMarkDamageMultiplier"));
    if (ChainMultiplier > 0.0) AmmoCharacter->AddPiercingChainMarks(AllHitTargets, ChainMultiplier);

    const int32 MarkDuration = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("TargetMarkDurationInitiative"))));
    const int32 RangeBonus = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AttackRangeVsMarked"))));
    if (MarkDuration > 0 && RangeBonus > 0)
    {
        for (ARoundCharacter* Target : AllHitTargets)
        {
            if (!Target || !Target->IsAlive()) continue;
            if (ARoundEffectActor* Mark = GetWorld()->SpawnActor<ARoundEffectActor>())
            {
                Mark->EffectId = TEXT("PiercingTargetMark");
                Mark->DisplayName = TEXT("Zielspur");
                Mark->Description = TEXT("Character 2 kann dieses Ziel aus groesserer Entfernung normal angreifen.");
                Mark->Amount = RangeBonus;
                Mark->CauserCharacter = OwningCharacter;
                Mark->SetDurationInitiative(MarkDuration, false);
                Target->AddEffect(Mark);
            }
        }
    }

    const int32 ReductionPerStack = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseReductionPerHitStack"))));
    if (ReductionPerStack > 0)
    {
        AmmoCharacter->AddPiercingDefenseStacks(TotalHitInstances, ReductionPerStack,
            FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseReductionDurationInitiative")))));
    }

    AmmoCharacter->ResolveConcentrateHit(AllHitTargets.Num() > 0 ? AllHitTargets[0] : nullptr, this, bKilledAnyTarget, false);
    Result.bSuccess = true;
    Result.Reason = FString::Printf(TEXT("Piercing Shot hit %d target(s) and %d object(s)."), AllHitTargets.Num(), TotalHitObjects);
    return Result;
}
