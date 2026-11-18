#include "Abilities/RoundRockblastAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundModifierEffect.h"
#include "Effects/RoundRockEchoEffect.h"
#include "Effects/RoundRootEffect.h"
#include "Game/RoundGameState.h"

namespace
{
ARoundGridTile* FindRockblastPushTile(ARoundGameState* GameState, ARoundGridTile* CenterTile, ARoundGridTile* EnemyTile)
{
    if (!GameState || !CenterTile || !EnemyTile)
    {
        return nullptr;
    }
    const FVector2D Direction = EnemyTile->GridPosition - CenterTile->GridPosition;
    ARoundGridTile* Best = nullptr;
    double BestDot = 0.0;
    for (ARoundGridTile* Tile : GameState->GetNeighborFields(EnemyTile))
    {
        if (!Tile || !Tile->IsMoveable() || Tile->OccupyingCharacter)
        {
            continue;
        }
        const double Dot = FVector2D::DotProduct(Direction, Tile->GridPosition - EnemyTile->GridPosition);
        if (Dot > BestDot)
        {
            BestDot = Dot;
            Best = Tile;
        }
    }
    return Best;
}
}

FRoundActionResult ARoundRockblastAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    const bool bTargetWasRock = TargetTile && TargetTile->TerrainType == ERoundTerrainType::Rocks;
    bool bAdjacentRockOrWall = false;
    if (OwningCharacter && TargetTile)
    {
        if (ARoundGameState* GameState = OwningCharacter->GetRoundGameState())
        {
            for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(TargetTile))
            {
                bAdjacentRockOrWall |= Neighbor && (Neighbor->TerrainType == ERoundTerrainType::Rocks || Neighbor->TerrainType == ERoundTerrainType::Wall);
            }
        }
    }

    FRoundActionResult Result = ExecuteDefaultAbility(TargetTile);
    if (!Result.bSuccess || !OwningCharacter || !TargetTile)
    {
        return Result;
    }

    ARoundGameState* RoundGameState = OwningCharacter->GetRoundGameState();
    if (!RoundGameState)
    {
        return Result;
    }

    ARoundCharacter* PrimaryTarget = TargetTile->OccupyingCharacter;
    if (bAdjacentRockOrWall && PrimaryTarget && OwningCharacter->IsEnemyFor(PrimaryTarget))
    {
        const int32 CrushingDamage = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BonusDamageNearRockOrWall"))));
        if (CrushingDamage > 0)
        {
            bool bDidDie = false;
            ApplyAbilityDamageToTarget(PrimaryTarget, CrushingDamage, 0, bDidDie);
        }
    }

    if (bTargetWasRock)
    {
        const double ShatterMultiplier = GetAbilityValueFromKey(TEXT("ShatterSplashDamageMultiplier"));
        if (ShatterMultiplier > 0.0)
        {
            for (ARoundGridTile* Neighbor : RoundGameState->GetNeighborFields(TargetTile))
            {
                ARoundCharacter* Enemy = Neighbor ? Neighbor->OccupyingCharacter : nullptr;
                if (Enemy && OwningCharacter->IsEnemyFor(Enemy))
                {
                    bool bDidDie = false;
                    ApplyAbilityDamageToTarget(Enemy, FMath::RoundToInt(GetAbilityDamage() * ShatterMultiplier), 0, bDidDie);
                }
            }
            TargetTile->SetTerrainType(ERoundTerrainType::Rubble);
        }
    }

    TArray<ARoundGridTile*> CreatedRockTiles;
    if (TargetTile->TerrainType != ERoundTerrainType::Rubble)
    {
        TargetTile->SetTerrainType(ERoundTerrainType::Rocks);
        CreatedRockTiles.Add(TargetTile);
    }
    int32 AdditionalRockCount = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AdditionalRockCount"))));
    for (ARoundGridTile* Neighbor : RoundGameState->GetNeighborFields(TargetTile))
    {
        if (AdditionalRockCount <= 0)
        {
            break;
        }

        if (Neighbor && !Neighbor->OccupyingCharacter)
        {
            Neighbor->SetTerrainType(ERoundTerrainType::Rocks);
            CreatedRockTiles.Add(Neighbor);
            --AdditionalRockCount;
        }
    }

    if (PrimaryTarget && PrimaryTarget->Attributes && PrimaryTarget->Attributes->IsAlive() && GetAbilityValueFromKey(TEXT("RootIfSurroundedByRocks")) > 0.0)
    {
        int32 AdjacentRocks = 0;
        for (ARoundGridTile* Neighbor : RoundGameState->GetNeighborFields(PrimaryTarget->CurrentTile))
        {
            AdjacentRocks += Neighbor && Neighbor->TerrainType == ERoundTerrainType::Rocks ? 1 : 0;
        }
        if (AdjacentRocks >= FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RequiredAdjacentRocks")))))
        {
            if (ARoundRootEffect* Root = GetWorld()->SpawnActor<ARoundRootEffect>())
            {
                Root->EffectId = TEXT("RockblastPrison");
                Root->DisplayName = TEXT("Steingefaengnis");
                Root->Description = TEXT("Kann sich nicht bewegen.");
                Root->CauserCharacter = OwningCharacter;
                Root->SetDurationInitiative(FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RockPrisonDurationInitiative")))), false);
                PrimaryTarget->AddEffect(Root);
            }
        }
    }

    const double CoverMultiplier = GetAbilityValueFromKey(TEXT("SelfIncomingDamageMultiplierAfterRockCreate"));
    if (CoverMultiplier > 0.0 && CoverMultiplier < 1.0 && CreatedRockTiles.Num() > 0)
    {
        if (ARoundModifierEffect* Cover = GetWorld()->SpawnActor<ARoundModifierEffect>())
        {
            Cover->EffectId = TEXT("RockblastCover");
            Cover->DisplayName = TEXT("Steindeckung");
            Cover->Description = TEXT("Erleidet weniger Schaden.");
            Cover->IncomingDamageMultiplier = CoverMultiplier;
            Cover->bTreatAsBuff = true;
            Cover->CauserCharacter = OwningCharacter;
            Cover->SetDurationInitiative(FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RockCoverDurationInitiative")))), false);
            OwningCharacter->AddEffect(Cover);
        }
    }

    const int32 LandslideDamage = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DamageNearCreatedRocks"))));
    TArray<ARoundCharacter*> LandslideTargets;
    if (LandslideDamage > 0)
    {
        for (ARoundGridTile* RockTile : CreatedRockTiles)
        {
            for (ARoundGridTile* Neighbor : RoundGameState->GetNeighborFields(RockTile))
            {
                ARoundCharacter* Enemy = Neighbor ? Neighbor->OccupyingCharacter : nullptr;
                if (!Enemy || LandslideTargets.Contains(Enemy) || !OwningCharacter->IsEnemyFor(Enemy))
                {
                    continue;
                }
                bool bDidDie = false;
                ApplyAbilityDamageToTarget(Enemy, LandslideDamage, 0, bDidDie);
                LandslideTargets.Add(Enemy);
                if (!bDidDie)
                {
                    if (ARoundGridTile* PushTile = FindRockblastPushTile(RoundGameState, TargetTile, Enemy->CurrentTile))
                    {
                        Enemy->TryForcedSetField(PushTile, OwningCharacter, true);
                    }
                }
            }
        }
    }

    const int32 EchoDamage = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RockEchoDamage"))));
    const int32 EchoDelay = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RockEchoDelayInitiative"))));
    if (EchoDamage > 0 && EchoDelay > 0 && CreatedRockTiles.Num() > 0)
    {
        if (ARoundRockEchoEffect* Echo = GetWorld()->SpawnActor<ARoundRockEchoEffect>())
        {
            Echo->EffectId = TEXT("RockblastEcho");
            Echo->DisplayName = TEXT("Seismisches Echo");
            Echo->Description = TEXT("Die erschaffenen Felsen beben erneut.");
            Echo->Amount = EchoDamage;
            Echo->CauserCharacter = OwningCharacter;
            for (ARoundGridTile* RockTile : CreatedRockTiles)
            {
                Echo->RockTilePositions.Add(RockTile->GridPosition);
            }
            Echo->SetDurationInitiative(EchoDelay, false);
            OwningCharacter->AddEffect(Echo);
        }
    }

    return Result;
}
