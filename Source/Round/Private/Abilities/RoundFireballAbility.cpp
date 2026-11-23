#include "Abilities/RoundFireballAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundFireballAfterburnEffect.h"
#include "Effects/RoundFireballBurnEffect.h"
#include "Effects/RoundModifierEffect.h"
#include "Game/RoundGameState.h"

namespace
{
static TArray<ARoundGridTile*> CollectTilesInRadius(const ARoundGameState* GameState, ARoundGridTile* CenterTile, int32 Radius)
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

static ARoundGridTile* FindFireballKnockbackTile(ARoundGameState* GameState, ARoundGridTile* CenterTile, ARoundGridTile* EnemyTile)
{
    if (!GameState || !CenterTile || !EnemyTile || CenterTile == EnemyTile)
    {
        return nullptr;
    }

    const FVector2D Direction = EnemyTile->GridPosition - CenterTile->GridPosition;
    ARoundGridTile* BestTile = nullptr;
    double BestDot = 0.0;
    for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(EnemyTile))
    {
        if (!Neighbor || !Neighbor->IsMoveable() || Neighbor->OccupyingCharacter)
        {
            continue;
        }
        const double Dot = FVector2D::DotProduct(Direction, Neighbor->GridPosition - EnemyTile->GridPosition);
        if (Dot > BestDot)
        {
            BestDot = Dot;
            BestTile = Neighbor;
        }
    }
    return BestTile;
}
}

void ARoundFireballAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    if (!TargetTile || !OwningCharacter)
    {
        return;
    }

    const ARoundGameState* GameState = OwningCharacter->GetRoundGameState();
    const int32 Radius = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DamageRange"))));
    OutTiles.Append(CollectTilesInRadius(GameState, const_cast<ARoundGridTile*>(TargetTile), Radius));
}

FRoundActionResult ARoundFireballAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 21)
        return ARoundAbilityActor::ExecuteAbility(TargetTile);
    return ExecuteDefaultAbility(TargetTile);
}

FRoundActionResult ARoundFireballAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!OwningCharacter || !OwningCharacter->Attributes || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    const int32 TargetsHit = ResolveExplosion(TargetTile, 1.0, true);

    const int32 AfterburnDelay = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AfterburnDelayInitiative"))));
    const double AfterburnMultiplier = GetAbilityValueFromKey(TEXT("AfterburnDamageMultiplier"));
    if (AfterburnDelay > 0 && AfterburnMultiplier > 0.0 && GetWorld())
    {
        if (ARoundFireballAfterburnEffect* Effect = GetWorld()->SpawnActor<ARoundFireballAfterburnEffect>())
        {
            Effect->EffectId = TEXT("FireballAfterburn");
            Effect->DisplayName = TEXT("Nachbrenner");
            Effect->Description = TEXT("Das Zielgebiet explodiert erneut.");
            Effect->FireballAbility = this;
            Effect->TargetTilePosition = TargetTile->GridPosition;
            Effect->DamageMultiplier = AfterburnMultiplier;
            Effect->CauserCharacter = OwningCharacter;
            Effect->SetDurationInitiative(AfterburnDelay, false);
            OwningCharacter->AddEffect(Effect);
        }
    }

    const int32 RecoilDistance = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("SelfRecoilDistance"))));
    ARoundGameState* GameState = OwningCharacter->GetRoundGameState();
    for (int32 Step = 0; Step < RecoilDistance && GameState && OwningCharacter->CurrentTile; ++Step)
    {
        ARoundGridTile* RecoilTile = FindFireballKnockbackTile(GameState, TargetTile, OwningCharacter->CurrentTile);
        if (!RecoilTile || !OwningCharacter->TryForcedSetField(RecoilTile, OwningCharacter, true))
        {
            break;
        }
    }

    Result.bSuccess = true;
    Result.Reason = FString::Printf(TEXT("Fireball hit %d target(s)."), TargetsHit);
    return Result;
}

int32 ARoundFireballAbility::ResolveExplosion(ARoundGridTile* TargetTile, const double DamageMultiplier, const bool bApplySecondaryEffects)
{
    if (!OwningCharacter || !TargetTile)
    {
        return 0;
    }

    ARoundGameState* GameState = OwningCharacter->GetRoundGameState();
    const int32 Radius = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DamageRange"))));
    const TArray<ARoundGridTile*> AffectedTiles = CollectTilesInRadius(GameState, TargetTile, Radius);

    int32 TargetsHit = 0;
    const int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    for (ARoundGridTile* Tile : AffectedTiles)
    {
        if (!Tile)
        {
            continue;
        }

        ARoundCharacter* TargetCharacter = Tile->OccupyingCharacter;
        double TileDamageMultiplier = DamageMultiplier;
        if (Radius > 0 && Tile->GetDistance(TargetTile) == Radius)
        {
            const double OuterRingMultiplier = GetAbilityValueFromKey(TEXT("OuterRingDamageMultiplier"));
            if (OuterRingMultiplier > 0.0)
            {
                TileDamageMultiplier *= OuterRingMultiplier;
            }
        }
        if (Tile == TargetTile && TargetCharacter && OwningCharacter->IsEnemyFor(TargetCharacter))
        {
            const int32 CenterDamageAdd = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CenterDamageAdd"))));
            TileDamageMultiplier *= GetAbilityDamage() > 0 ? static_cast<double>(GetAbilityDamage() + CenterDamageAdd) / GetAbilityDamage() : 1.0;
        }
        const int32 TileDamage = FMath::Max(0, FMath::RoundToInt(GetAbilityDamage() * TileDamageMultiplier));
        if (TargetCharacter && OwningCharacter->IsEnemyFor(TargetCharacter))
        {
            bool bAnyDestroyed = false;
            TargetsHit += ApplyAbilityDamageToTile(Tile, TileDamage, DefenseIgnore, bAnyDestroyed, true);

            const int32 InitiativeLoss = bApplySecondaryEffects ? FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnHit")))) : 0;
            if (InitiativeLoss > 0)
            {
                TargetCharacter->ApplyDelay(InitiativeLoss);
            }

            if (bApplySecondaryEffects && GetAbilityValueFromKey(TEXT("KnockbackDistance")) > 0.0 && TargetCharacter->Attributes && TargetCharacter->Attributes->IsAlive())
            {
                if (ARoundGridTile* PushTile = FindFireballKnockbackTile(GameState, TargetTile, TargetCharacter->CurrentTile))
                {
                    TargetCharacter->TryForcedSetField(PushTile, OwningCharacter, true);
                }
            }

            const int32 BurnDamage = bApplySecondaryEffects ? FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BurnDamageOnTurnStart")))) : 0;
            const int32 BurnDuration = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BurnDurationInitiative"))));
            if (BurnDamage > 0 && BurnDuration > 0 && TargetCharacter->Attributes && TargetCharacter->Attributes->IsAlive())
            {
                if (ARoundFireballBurnEffect* Burn = GetWorld()->SpawnActor<ARoundFireballBurnEffect>())
                {
                    Burn->EffectId = TEXT("FireballInferno");
                    Burn->DisplayName = TEXT("Inferno");
                    Burn->Description = TEXT("Erleidet zu Zugbeginn Feuerschaden.");
                    Burn->Amount = BurnDamage;
                    Burn->CauserCharacter = OwningCharacter;
                    Burn->SetDurationInitiative(BurnDuration, false);
                    TargetCharacter->AddEffect(Burn);
                }
            }

            const int32 DefenseReduction = bApplySecondaryEffects ? FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseReductionOnHit")))) : 0;
            const int32 DefenseReductionDuration = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseReductionDurationInitiative"))));
            if (DefenseReduction > 0 && DefenseReductionDuration > 0 && TargetCharacter->Attributes && TargetCharacter->Attributes->IsAlive())
            {
                if (ARoundModifierEffect* Melt = GetWorld()->SpawnActor<ARoundModifierEffect>())
                {
                    Melt->EffectId = TEXT("FireballMelt");
                    Melt->DisplayName = TEXT("Schmelzpunkt");
                    Melt->Description = TEXT("Angriffe ignorieren einen Teil der Defense.");
                    Melt->DefenseIgnoreAdd = DefenseReduction;
                    Melt->bTreatAsDebuff = true;
                    Melt->CauserCharacter = OwningCharacter;
                    Melt->SetDurationInitiative(DefenseReductionDuration, false);
                    TargetCharacter->AddEffect(Melt);
                }
            }
        }
        else
        {
            bool bAnyDestroyed = false;
            TargetsHit += ApplyAbilityDamageToTile(Tile, TileDamage, DefenseIgnore, bAnyDestroyed, false);
        }
    }

    if (bApplySecondaryEffects && GetAbilityValueFromKey(TEXT("CreateFireTerrainOnTarget")) > 0.0)
    {
        TargetTile->SetTerrainType(ERoundTerrainType::Fire);
    }
    return TargetsHit;
}

void ARoundFireballAbility::ResolveAfterburn(ARoundGridTile* TargetTile, const double DamageMultiplier)
{
    ResolveExplosion(TargetTile, DamageMultiplier, false);
}
