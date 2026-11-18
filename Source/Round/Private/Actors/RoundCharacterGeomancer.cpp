#include "Actors/RoundCharacterGeomancer.h"

#include "Actors/RoundGridTile.h"
#include "Actors/RoundTerrainActor.h"
#include "Actors/RoundAbilityActor.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundTerrainHelpers.h"
#include "Effects/RoundGeomancerMarkEffect.h"
#include "Game/RoundGameState.h"
#include "Interfaces/RoundBoardDamageableInterface.h"
#include "Net/UnrealNetwork.h"

namespace
{
ARoundGridTile* FindGeomancerTileBehindTarget(ARoundGameState* GameState, ARoundGridTile* SourceTile, ARoundGridTile* TargetTile)
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
        if (!Neighbor || Neighbor->OccupyingCharacter || Neighbor->GetResolvedOccupyingFieldObject())
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

void ARoundCharacterGeomancer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundCharacterGeomancer, MarkedEnemies);
}

FRoundActionResult ARoundCharacterGeomancer::AttackField(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (HasTurnActionBlock())
    {
        Result.Reason = TEXT("Character cannot act.");
        return Result;
    }

    if (!TargetTile)
    {
        Result.Reason = TEXT("Invalid attack tile.");
        return Result;
    }

    ARoundCharacter* TargetCharacter = TargetTile->OccupyingCharacter;
    AActor* TargetBoardObject = TargetTile->OccupyingFieldObject;
    if (!TargetBoardObject || !TargetBoardObject->GetClass()->ImplementsInterface(URoundBoardDamageableInterface::StaticClass()) || !IRoundBoardDamageableInterface::Execute_CanReceiveBoardDamage(TargetBoardObject))
    {
        TargetBoardObject = TargetTile->TerrainObject;
        if (!TargetBoardObject || !TargetBoardObject->GetClass()->ImplementsInterface(URoundBoardDamageableInterface::StaticClass()) || !IRoundBoardDamageableInterface::Execute_CanReceiveBoardDamage(TargetBoardObject))
        {
            TargetBoardObject = nullptr;
        }
    }

    if (TargetCharacter && !EnemyField(TargetTile))
    {
        Result.Reason = TEXT("Target is not an enemy.");
        return Result;
    }

    if (!CurrentTile)
    {
        Result.Reason = TEXT("Character has no current tile.");
        return Result;
    }

    ARoundGameState* RoundGameState = GetRoundGameState();
    if (!RoundGameState)
    {
        Result.Reason = TEXT("No game state available.");
        return Result;
    }

    const bool bInRange = CurrentTile->GetDistance(TargetTile) <= GetAttackRange();
    if (!bInRange || ((!TargetCharacter || !EnemyField(TargetTile)) && !TargetBoardObject))
    {
        Result.Reason = TEXT("Target is not a legal action.");
        return Result;
    }

    if (!Attributes || !Attributes->SpendInitiative(GetAttackCost()))
    {
        Result.Reason = TEXT("Not enough initiative.");
        return Result;
    }

    int32 DefenseIgnore = 0;
    int32 Damage = Attributes->BaseStats.Attack;
    if (FMath::FRand() < GetCritChance())
    {
        Damage = FMath::RoundToInt(static_cast<double>(Damage) * GetCritModifier());
    }

    const bool bCanTriggerMarkedAttack = TargetCharacter
        && HasGeomancerMarkOn(TargetCharacter)
        && PassiveCooldownRemainingByTarget.FindRef(TargetCharacter) <= 0;
    if (bCanTriggerMarkedAttack)
    {
        Damage += GetMarkedAttackBonusDamage();
        Damage = FMath::RoundToInt(static_cast<double>(Damage) * (1.0 + FMath::Max(0.0, GetPassiveValueFromKey(TEXT("MarkedAttackDamageMultiplierAdd")))));
        DefenseIgnore += FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("MarkedAttackDefenseIgnore"))));
    }

    int32 DamageDone = 0;
    bool bDidDie = false;
    if (TargetCharacter)
    {
        Damage = ModifyOutgoingDamage(Damage, ERoundDamageType::Physical, TargetCharacter, DefenseIgnore);
        DamageDone = TargetCharacter->ApplyRoundDamage(Damage, ERoundDamageType::Physical, DefenseIgnore, this, bDidDie);

        if (bCanTriggerMarkedAttack)
        {
            if (GetPassiveValueFromKey(TEXT("KeepMarkAfterEmpoweredAttack")) <= 0.0)
            {
                ConsumeGeomancerMarkOn(TargetCharacter);
            }
            PassiveCooldownRemainingByTarget.Add(TargetCharacter, GetPassivePerTargetCooldownInitiative());

            if (Attributes)
            {
                Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("InitiativeGainOnMarkTrigger")))));
            }

            const int32 HealOnTrigger = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("HealOnMarkTrigger"))));
            if (HealOnTrigger > 0)
            {
                Heal(HealOnTrigger);
            }

            const int32 CooldownReduction = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("AbilityCooldownReductionOnMarkTrigger"))));
            if (CooldownReduction > 0)
            {
                for (ARoundAbilityActor* Ability : GrantedAbilities)
                {
                    if (Ability) Ability->ReduceCooldown(CooldownReduction);
                }
            }

            if (ARoundGridTile* RockTile = FindGeomancerTileBehindTarget(RoundGameState, CurrentTile, TargetCharacter->CurrentTile))
            {
                RockTile->SetTerrainType(ERoundTerrainType::Rocks);

                int32 AdditionalRocks = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("AdditionalRocksOnMarkTrigger"))));
                for (ARoundGridTile* Neighbor : RoundGameState->GetNeighborFields(TargetCharacter->CurrentTile))
                {
                    if (AdditionalRocks <= 0) break;
                    if (!Neighbor || Neighbor == RockTile || Neighbor->OccupyingCharacter || Neighbor->GetResolvedOccupyingFieldObject()) continue;
                    Neighbor->SetTerrainType(ERoundTerrainType::Rocks);
                    --AdditionalRocks;
                }
            }
        }
    }
    else if (TargetBoardObject)
    {
        Damage = ModifyOutgoingDamage(Damage, ERoundDamageType::Physical, nullptr, DefenseIgnore);
        const FRoundBoardDamageResult BoardDamageResult = IRoundBoardDamageableInterface::Execute_ApplyBoardDamage(
            TargetBoardObject,
            Damage,
            ERoundDamageType::Physical,
            DefenseIgnore,
            this);
        DamageDone = BoardDamageResult.AppliedDamage;
        bDidDie = BoardDamageResult.bDestroyed;
    }

    Result.bSuccess = true;
    Result.Reason = FString::Printf(TEXT("Attack executed for %d damage."), DamageDone);
    if (TargetCharacter)
    {
        RoundGameState->RecordBasicAttack(this, TargetCharacter, DamageDone);
    }

    RoundGameState->SetVision();
    RoundGameState->OnBoardStateChanged.Broadcast();
    NotifyRoundStateChanged();
    ForceNetUpdate();
    return Result;
}

void ARoundCharacterGeomancer::AdvanceGlobalInitiative(int32 Amount)
{
    Super::AdvanceGlobalInitiative(Amount);

    if (Amount <= 0 || PassiveCooldownRemainingByTarget.Num() <= 0)
    {
        return;
    }

    TArray<TWeakObjectPtr<ARoundCharacter>> KeysToRemove;
    for (TPair<TWeakObjectPtr<ARoundCharacter>, int32>& Pair : PassiveCooldownRemainingByTarget)
    {
        if (!Pair.Key.IsValid())
        {
            KeysToRemove.Add(Pair.Key);
            continue;
        }

        Pair.Value = FMath::Max(0, Pair.Value - Amount);
        if (Pair.Value <= 0)
        {
            KeysToRemove.Add(Pair.Key);
        }
    }

    for (const TWeakObjectPtr<ARoundCharacter>& Key : KeysToRemove)
    {
        PassiveCooldownRemainingByTarget.Remove(Key);
    }
}

void ARoundCharacterGeomancer::ApplyGeomancerMark(ARoundCharacter* TargetCharacter)
{
    if (!HasAuthority() || !GetWorld() || !TargetCharacter || !IsEnemyFor(TargetCharacter))
    {
        return;
    }

    for (ARoundEffectActor* Effect : TargetCharacter->ActiveEffects)
    {
        if (ARoundGeomancerMarkEffect* ExistingMark = Cast<ARoundGeomancerMarkEffect>(Effect))
        {
            if (ExistingMark->CauserCharacter == this)
            {
                if (!MarkedEnemies.Contains(TargetCharacter))
                {
                    MarkedEnemies.Add(TargetCharacter);
                }
                NotifyRoundStateChanged();
                ForceNetUpdate();
                return;
            }
        }
    }

    ARoundGeomancerMarkEffect* MarkEffect = GetWorld()->SpawnActor<ARoundGeomancerMarkEffect>(ARoundGeomancerMarkEffect::StaticClass());
    if (!MarkEffect)
    {
        return;
    }

    MarkEffect->EffectId = TEXT("GeomancerMark");
    MarkEffect->DisplayName = TEXT("Fracture Mark");
    MarkEffect->Description = TEXT("Der naechste Standardangriff des Geomancers verursacht Bonusschaden und erzeugt einen Stein.");
    MarkEffect->CauserCharacter = this;
    TargetCharacter->AddEffect(MarkEffect);
    MarkedEnemies.AddUnique(TargetCharacter);
    NotifyRoundStateChanged();
    ForceNetUpdate();
}

bool ARoundCharacterGeomancer::HasGeomancerMarkOn(const ARoundCharacter* TargetCharacter) const
{
    if (!TargetCharacter)
    {
        return false;
    }

    for (ARoundEffectActor* Effect : TargetCharacter->ActiveEffects)
    {
        const ARoundGeomancerMarkEffect* MarkEffect = Cast<ARoundGeomancerMarkEffect>(Effect);
        if (MarkEffect && MarkEffect->CauserCharacter == this)
        {
            return true;
        }
    }

    return false;
}

bool ARoundCharacterGeomancer::ConsumeGeomancerMarkOn(ARoundCharacter* TargetCharacter)
{
    if (!TargetCharacter)
    {
        return false;
    }

    for (ARoundEffectActor* Effect : TargetCharacter->ActiveEffects)
    {
        if (ARoundGeomancerMarkEffect* MarkEffect = Cast<ARoundGeomancerMarkEffect>(Effect))
        {
            if (MarkEffect->CauserCharacter == this)
            {
                MarkEffect->RemoveEffect();
                MarkedEnemies.Remove(TargetCharacter);
                NotifyRoundStateChanged();
                ForceNetUpdate();
                return true;
            }
        }
    }

    return false;
}

int32 ARoundCharacterGeomancer::GetMarkedEnemyCount() const
{
    int32 Count = 0;
    for (ARoundCharacter* Candidate : MarkedEnemies)
    {
        if (Candidate && HasGeomancerMarkOn(Candidate))
        {
            ++Count;
        }
    }
    return Count;
}

int32 ARoundCharacterGeomancer::GetPassivePerTargetCooldownInitiative() const
{
    const int32 BaseValue = FMath::RoundToInt(GetValueFromKey(TEXT("PassivePerTargetCooldownInitiative")));
    return FMath::Max(0, (BaseValue > 0 ? BaseValue : 70) + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("PassivePerTargetCooldownInitiative"))));
}

int32 ARoundCharacterGeomancer::GetMarkedAttackBonusDamage() const
{
    const int32 BaseValue = FMath::RoundToInt(GetValueFromKey(TEXT("MarkedAttackBonusDamage")));
    return FMath::Max(0, (BaseValue > 0 ? BaseValue : 70) + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("MarkedAttackBonusDamage"))));
}
