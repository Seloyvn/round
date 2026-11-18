#include "Abilities/RoundCallNearestBladeAbility.h"

#include "Actors/RoundBladeActor.h"
#include "Actors/RoundCharacterBladecaller.h"
#include "Actors/RoundGridTile.h"
#include "Game/RoundGameState.h"
#include "Components/RoundAttributeSetComponent.h"

namespace
{
ARoundBladeActor* FindNearestBladeToTile(const ARoundCharacterBladecaller* Bladecaller, const ARoundGridTile* TargetTile)
{
    if (!Bladecaller || !TargetTile)
    {
        return nullptr;
    }

    ARoundBladeActor* BestBlade = nullptr;
    int32 BestDistance = MAX_int32;
    int32 BestDistanceToCaster = MAX_int32;
    for (ARoundBladeActor* Blade : Bladecaller->GetActiveBlades())
    {
        if (!Blade || !Blade->CurrentTile)
        {
            continue;
        }

        const int32 DistanceToTarget = Blade->CurrentTile->GetDistance(TargetTile);
        const int32 DistanceToCaster = Bladecaller->CurrentTile ? Blade->CurrentTile->GetDistance(Bladecaller->CurrentTile) : DistanceToTarget;
        if (DistanceToTarget < BestDistance || (DistanceToTarget == BestDistance && DistanceToCaster < BestDistanceToCaster))
        {
            BestBlade = Blade;
            BestDistance = DistanceToTarget;
            BestDistanceToCaster = DistanceToCaster;
        }
    }

    return BestBlade;
}

TArray<ARoundGridTile*> CollectNearestBladeAoETiles(const ARoundGameState* GameState, ARoundGridTile* CenterTile, int32 Radius)
{
    TArray<ARoundGridTile*> Result;
    if (!GameState || !CenterTile)
    {
        return Result;
    }

    Result.Add(CenterTile);
    TArray<ARoundGridTile*> Frontier{CenterTile};
    for (int32 Step = 0; Step < Radius; ++Step)
    {
        TArray<ARoundGridTile*> NextFrontier;
        for (ARoundGridTile* Tile : Frontier)
        {
            for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(Tile))
            {
                if (Neighbor && !Result.Contains(Neighbor))
                {
                    Result.Add(Neighbor);
                    NextFrontier.Add(Neighbor);
                }
            }
        }

        Frontier = MoveTemp(NextFrontier);
        if (Frontier.Num() == 0)
        {
            break;
        }
    }

    return Result;
}
}

bool ARoundCallNearestBladeAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    const ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    return Bladecaller
        && TargetTile
        && IsTargetInRange(TargetTile)
        && GetAbilityVisibility() == ERoundButtonState::Enabled
        && (!TargetTile->OccupyingFieldObject || Bladecaller->FindBladeAtTile(const_cast<ARoundGridTile*>(TargetTile)) != nullptr)
        && FindNearestBladeToTile(Bladecaller, TargetTile) != nullptr;
}

void ARoundCallNearestBladeAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    const ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    const ARoundGameState* GameState = Bladecaller ? Bladecaller->GetRoundGameState() : nullptr;
    if (!Bladecaller || !GameState || !TargetTile)
    {
        return;
    }

    ARoundBladeActor* Blade = FindNearestBladeToTile(Bladecaller, TargetTile);
    if (Blade && Blade->CurrentTile)
    {
        OutTiles.Append(GameState->GetFieldsBetween(Blade->CurrentTile, const_cast<ARoundGridTile*>(TargetTile), true));
    }

    const int32 Radius = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AOERange"))));
    OutTiles.Append(CollectNearestBladeAoETiles(GameState, const_cast<ARoundGridTile*>(TargetTile), Radius));
}

FRoundActionResult ARoundCallNearestBladeAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    ARoundGameState* GameState = Bladecaller ? Bladecaller->GetRoundGameState() : nullptr;
    if (!Bladecaller || !GameState || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundBladeActor* Blade = FindNearestBladeToTile(Bladecaller, TargetTile);
    if (!Blade || !Blade->CurrentTile)
    {
        Result.Reason = TEXT("No blade available.");
        return Result;
    }

    const FRoundFieldObjectMoveResult MoveResult = Bladecaller->MoveBladeToTile(Blade, TargetTile);
    if (GetAbilityValueFromKey(TEXT("RefreshBladeLifetime")) > 0.0) Blade->SetLifetimeInitiative(Bladecaller->GetBladeLifetimeInitiative());
    const int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    int32 TotalHits = 0;

    for (ARoundCharacter* HitCharacter : MoveResult.HitCharacters)
    {
        if (!HitCharacter || !Bladecaller->IsEnemyFor(HitCharacter))
        {
            continue;
        }

        bool bDidDie = false;
        ApplyAbilityDamageToTarget(HitCharacter, GetAbilityDamage(), DefenseIgnore, bDidDie);
        ++TotalHits;
    }

    const int32 Radius = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AOERange"))));
    const TArray<ARoundGridTile*> AffectedTiles = CollectNearestBladeAoETiles(GameState, TargetTile, Radius);
    for (ARoundGridTile* Tile : AffectedTiles)
    {
        ARoundCharacter* TargetCharacter = Tile ? Tile->OccupyingCharacter : nullptr;
        if (!TargetCharacter || !Bladecaller->IsEnemyFor(TargetCharacter))
        {
            continue;
        }

        bool bDidDie = false;
        ApplyAbilityDamageToTarget(TargetCharacter, GetAbilityDamage(), DefenseIgnore, bDidDie);
        ++TotalHits;
    }

    const int32 InitiativePerHit = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativePerHit"))));
    if (Bladecaller->Attributes && InitiativePerHit > 0) Bladecaller->Attributes->GainInitiative(FMath::Min(30, InitiativePerHit * TotalHits));
    if (TotalHits >= 2) ReduceCooldown(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionOnMultiHit")))));

    Result.bSuccess = true;
    Result.Reason = FString::Printf(TEXT("Nearest blade moved and hit %d time(s)."), TotalHits);
    return Result;
}
