#include "Abilities/RoundSnipeAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter2.h"
#include "Actors/RoundEffectActor.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundModifierEffect.h"
#include "Effects/RoundSnipeMarkEffect.h"
#include "Game/RoundGameState.h"
#include "Net/UnrealNetwork.h"

namespace
{
ARoundGridTile* FindTileBehindTarget(ARoundGameState* GameState, ARoundGridTile* SourceTile, ARoundGridTile* TargetTile)
{
    if (!GameState || !SourceTile || !TargetTile) return nullptr;
    const FVector2D Direction = TargetTile->GridPosition - SourceTile->GridPosition;
    ARoundGridTile* BestTile = nullptr;
    double BestDot = -DBL_MAX;
    for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(TargetTile))
    {
        if (!Neighbor) continue;
        const double Dot = FVector2D::DotProduct(Direction, Neighbor->GridPosition - TargetTile->GridPosition);
        if (Dot > BestDot)
        {
            BestDot = Dot;
            BestTile = Neighbor;
        }
    }
    return BestTile;
}
}

void ARoundSnipeAbility::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundSnipeAbility, ReadyInitiativeElapsed);
}

void ARoundSnipeAbility::AdvanceCooldown()
{
    const bool bWasReady = CurrentCooldown <= 0;
    Super::AdvanceCooldown();
    if (bWasReady && GetAbilityValueFromKey(TEXT("ReadyDamagePercentPerInterval")) > 0.0)
    {
        ++ReadyInitiativeElapsed;
        ForceNetUpdate();
    }
}

FRoundActionResult ARoundSnipeAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 11)
    {
        return ARoundAbilityActor::ExecuteAbility(TargetTile);
    }
    FRoundActionResult Result;
    ARoundCharacter2* AmmoCharacter = Cast<ARoundCharacter2>(OwningCharacter);
    if (!CanTargetTile(TargetTile) || !AmmoCharacter || !OwningCharacter->Attributes || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }
    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    if (!TargetCharacter || !TargetCharacter->Attributes)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }
    if (!AmmoCharacter->HasAmmo())
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
    int32 Damage = GetAbilityDamage();
    const int32 MissingInitiative = FMath::Max(0, TargetCharacter->Attributes->BaseStats.MaxInitiative - TargetCharacter->Attributes->CurrentInitiative);
    Damage += FMath::RoundToInt(MissingInitiative * GetAbilityValueFromKey(TEXT("InitativeScaling")));

    if (const double IsolatedMultiplier = GetAbilityValueFromKey(TEXT("DamageMultiplierVsIsolated")); IsolatedMultiplier > 0.0)
    {
        bool bHasAdjacentAlly = false;
        if (ARoundGameState* GameState = OwningCharacter->GetRoundGameState())
        {
            for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(TargetTile))
            {
                if (Neighbor && Neighbor->OccupyingCharacter && !TargetCharacter->IsEnemyFor(Neighbor->OccupyingCharacter))
                {
                    bHasAdjacentAlly = true;
                    break;
                }
            }
        }
        if (!bHasAdjacentAlly) Damage = FMath::RoundToInt(Damage * IsolatedMultiplier);
    }

    const int32 ReadyInterval = FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ReadyDamageInitiativeInterval"))));
    const double ReadyPercent = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("ReadyDamagePercentPerInterval")));
    const double ReadyCap = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("ReadyDamagePercentCap")));
    if (ReadyPercent > 0.0)
    {
        const double AccumulatedPercent = FMath::Min(ReadyCap, (ReadyInitiativeElapsed / ReadyInterval) * ReadyPercent);
        Damage = FMath::RoundToInt(Damage * (1.0 + AccumulatedPercent));
    }

    int32 DefenseIgnore = 0;
    const int32 RequiredMissing = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RequiredMissingInitiativeForDefenseIgnore")));
    if (RequiredMissing > 0 && MissingInitiative >= RequiredMissing)
    {
        DefenseIgnore += FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ConditionalDefenseIgnore")));
    }
    const double LowHealthThreshold = GetAbilityValueFromKey(TEXT("LowHealthThresholdPercent"));
    if (LowHealthThreshold > 0.0 && TargetCharacter->Attributes->CurrentHealth * 100.0 <= TargetCharacter->Attributes->BaseStats.MaxHealth * LowHealthThreshold)
    {
        Damage = FMath::RoundToInt(Damage * GetAbilityValueFromKey(TEXT("DamageMultiplierVsLowHealth")));
    }

    const int32 AmmoBeforeShot = AmmoCharacter->Ammo;
    ARoundEffectActor* DebuffToExtend = nullptr;
    int32 LowestDebuffDuration = MAX_int32;
    if (GetAbilityValueFromKey(TEXT("ExtendDebuffDurationInitiative")) > 0.0)
    {
        for (ARoundEffectActor* Effect : TargetCharacter->ActiveEffects)
        {
            if (Effect && Effect->IsDebuff() && Effect->IsRemovable() && Effect->GetRemainingDurationInitiative() < LowestDebuffDuration)
            {
                LowestDebuffDuration = Effect->GetRemainingDurationInitiative();
                DebuffToExtend = Effect;
            }
        }
    }
    bool bDidDie = false;
    const int32 FinalDamage = OwningCharacter->ModifyOutgoingDamage(Damage, GetAbilityDamageType(), TargetCharacter, DefenseIgnore);
    TargetCharacter->ApplyRoundDamage(FinalDamage, GetAbilityDamageType(), DefenseIgnore, OwningCharacter, bDidDie);

    if (!bDidDie && GetAbilityValueFromKey(TEXT("DefenseReductionOnHit")) > 0.0)
    {
        if (ARoundModifierEffect* Effect = GetWorld()->SpawnActor<ARoundModifierEffect>())
        {
            Effect->EffectId = TEXT("SnipeDefenseBreak");
            Effect->DisplayName = TEXT("Aufgerissene Deckung");
            Effect->DefenseIgnoreAdd = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseReductionOnHit")));
            Effect->bTreatAsDebuff = true;
            Effect->CauserCharacter = OwningCharacter;
            Effect->SetDurationInitiative(FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseReductionDurationInitiative"))), false);
            TargetCharacter->AddEffect(Effect);
        }
    }
    if (!bDidDie && GetAbilityValueFromKey(TEXT("MarkNextHitDamageMultiplier")) > 0.0)
    {
        if (ARoundSnipeMarkEffect* Effect = GetWorld()->SpawnActor<ARoundSnipeMarkEffect>())
        {
            Effect->EffectId = TEXT("SnipeMark");
            Effect->DisplayName = TEXT("Zielmarkierung");
            Effect->Amount = GetAbilityValueFromKey(TEXT("MarkNextHitDamageMultiplier"));
            Effect->CauserCharacter = OwningCharacter;
            Effect->SetDurationInitiative(FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MarkDurationInitiative"))), false);
            TargetCharacter->AddEffect(Effect);
        }
    }

    if (!bDidDie && DebuffToExtend)
    {
        if (DebuffToExtend->RemainingInitiative > 0) DebuffToExtend->RemainingInitiative += FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ExtendDebuffDurationInitiative")));
        else if (DebuffToExtend->RemainingTurns > 0) ++DebuffToExtend->RemainingTurns;
    }

    if (GetAbilityValueFromKey(TEXT("SecondaryTargetDamageMultiplier")) > 0.0)
    {
        if (ARoundGameState* GameState = OwningCharacter->GetRoundGameState())
        {
            ARoundGridTile* BehindTile = FindTileBehindTarget(GameState, OwningCharacter->CurrentTile, TargetTile);
            ARoundCharacter* BehindCharacter = BehindTile ? BehindTile->OccupyingCharacter : nullptr;
            if (BehindCharacter && OwningCharacter->IsEnemyFor(BehindCharacter))
            {
                bool bSecondaryDied = false;
                ApplyAbilityDamageToTarget(BehindCharacter, FMath::RoundToInt(Damage * GetAbilityValueFromKey(TEXT("SecondaryTargetDamageMultiplier"))), DefenseIgnore, bSecondaryDied);
            }
        }
    }
    if (!bDidDie && AmmoBeforeShot == 1 && GetAbilityValueFromKey(TEXT("LastAmmoKnockbackDistance")) > 0.0)
    {
        if (ARoundGameState* GameState = OwningCharacter->GetRoundGameState())
        {
            if (ARoundGridTile* PushTile = FindTileBehindTarget(GameState, OwningCharacter->CurrentTile, TargetCharacter->CurrentTile))
            {
                TargetCharacter->TryForcedSetField(PushTile, OwningCharacter, true);
            }
        }
    }

    const bool bMaxRange = OwningCharacter->CurrentTile && OwningCharacter->CurrentTile->GetDistance(TargetTile) >= GetAbilityRange();
    const bool bRangeRefund = bMaxRange && !AmmoCharacter->bUsedSnipeRangeRefundThisTurn && GetAbilityValueFromKey(TEXT("RefundAmmoAtMaxRange")) > 0.0;
    const bool bKillRefund = bDidDie && GetAbilityValueFromKey(TEXT("RefundAmmoOnKill")) > 0.0;
    AmmoCharacter->ResolveAmmoShot(bDidDie, bRangeRefund || bKillRefund);
    AmmoCharacter->ResolveConcentrateHit(TargetCharacter, this, bDidDie, false);
    AmmoCharacter->bUsedSnipeRangeRefundThisTurn |= bRangeRefund;
    if (bDidDie && !AmmoCharacter->bUsedSnipeKillCooldownReductionThisTurn)
    {
        const int32 CooldownReduction = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionOnKill")));
        if (CooldownReduction > 0)
        {
            ReduceCooldown(CooldownReduction);
            AmmoCharacter->bUsedSnipeKillCooldownReductionThisTurn = true;
        }
    }

    ReadyInitiativeElapsed = 0;
    ForceNetUpdate();
    Result.bSuccess = true;
    Result.Reason = TEXT("Snipe executed.");
    return Result;
}
