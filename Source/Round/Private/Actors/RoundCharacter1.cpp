#include "Actors/RoundCharacter1.h"

#include "Actors/RoundEffectActor.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundCharacterData.h"
#include "Game/RoundGameState.h"
#include "Net/UnrealNetwork.h"

ARoundCharacter1::ARoundCharacter1()
{
    CharacterDataAsset = TSoftObjectPtr<URoundCharacterData>(FSoftObjectPath(TEXT("/Game/TopDown/Characters/Character_1/Character1Details.Character1Details")));
}

void ARoundCharacter1::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundCharacter1, NativeChainCount);
    DOREPLIFETIME(ARoundCharacter1, NativeChainRechargeProgress);
}

void ARoundCharacter1::OnRep_ChainState()
{
    NotifyRoundStateChanged();
}

int32 ARoundCharacter1::GetChainLimit() const
{
    const int32 Limit = FMath::RoundToInt(GetValueFromKey(TEXT("ChainLimit"))) + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("ChainLimitAdd")));
    return FMath::Max(1, Limit);
}

int32 ARoundCharacter1::GetChainRechargeTime() const
{
    const int32 RechargeTime = FMath::RoundToInt(GetValueFromKey(TEXT("ChainRechargeTime"))) + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("ChainRechargeTimeAdd")));
    return FMath::Max(1, RechargeTime);
}

int32 ARoundCharacter1::GetCurrentChainCount() const
{
    return FMath::Clamp(NativeChainCount, 0, GetChainLimit());
}

bool ARoundCharacter1::TryHasChain(int32 Amount) const
{
    return Amount > 0 && GetCurrentChainCount() >= Amount;
}

void ARoundCharacter1::InitializeCharacter()
{
    Super::InitializeCharacter();

    if (!bChainStateInitialized)
    {
        NativeChainCount = GetChainLimit();
        bChainStateInitialized = true;
    }

    NativeChainCount = FMath::Clamp(NativeChainCount, 0, GetChainLimit());
    NativeChainRechargeProgress = FMath::Max(0, NativeChainRechargeProgress);
    NativeDamageRechargeCooldownRemaining = FMath::Max(0, NativeDamageRechargeCooldownRemaining);
}

void ARoundCharacter1::BeginTurn()
{
    if (!bCoreTurnDelayAlreadyApplied)
    {
        bGrantedChainSpendRefundThisTurn = false;
        bMovedThisTurn = false;
        bStationaryGuardActive = false;

        if (GetCurrentChainCount() <= 0)
        {
            const int32 ChainGain = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("ZeroChainTurnStartGainChain"))));
            if (ChainGain > 0)
            {
                GainChains(ChainGain);
            }

            const int32 InitiativePenalty = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("ZeroChainTurnStartInitiativePenalty"))));
            if (InitiativePenalty > 0)
            {
                Super::ApplyDelay(InitiativePenalty);
            }
        }
    }

    Super::BeginTurn();
}

void ARoundCharacter1::AdvanceGlobalInitiative(int32 Amount)
{
    Super::AdvanceGlobalInitiative(Amount);

    if (Amount <= 0)
    {
        return;
    }

    const ARoundGameState* GameState = GetRoundGameState();
    if (GameState && GameState->IsApplyingCoreInitiativeAdvance())
    {
        return;
    }

    NativeDamageRechargeCooldownRemaining = FMath::Max(0, NativeDamageRechargeCooldownRemaining - Amount);
    HandleChainRechargeTick(Amount);
}

void ARoundCharacter1::EndTurn()
{
    bStationaryGuardActive = !bMovedThisTurn;
    Super::EndTurn();
}

FRoundActionResult ARoundCharacter1::MoveAlongPath(const TArray<ARoundGridTile*>& Path)
{
    const FRoundActionResult Result = Super::MoveAlongPath(Path);
    if (Result.bSuccess && Path.Num() > 0)
    {
        bMovedThisTurn = true;
        bStationaryGuardActive = false;
    }

    return Result;
}

void ARoundCharacter1::ApplyDelay(int32 Amount)
{
    const double DelayMultiplier = GetPassiveValueFromKey(TEXT("DelayReceivedMultiplier"));
    const int32 AdjustedAmount = DelayMultiplier > 0.0
        ? FMath::RoundToInt(static_cast<double>(FMath::Max(0, Amount)) * DelayMultiplier)
        : Amount;
    Super::ApplyDelay(AdjustedAmount);
}

int32 ARoundCharacter1::ApplyRoundDamage(int32 Amount, ERoundDamageType DamageType, int32 DefenseIgnore, ARoundCharacter* SourceCharacter, bool& bDidDie)
{
    int32 AdjustedAmount = Amount;
    if (SourceCharacter && HasDebuffAppliedToCharacter(SourceCharacter))
    {
        const double Multiplier = GetPassiveValueFromKey(TEXT("IncomingDamageMultiplierFromDebuffedEnemies"));
        if (Multiplier > 0.0)
        {
            AdjustedAmount = FMath::RoundToInt(static_cast<double>(AdjustedAmount) * Multiplier);
        }
    }

    return Super::ApplyRoundDamage(AdjustedAmount, DamageType, DefenseIgnore, SourceCharacter, bDidDie);
}

double ARoundCharacter1::GetIncomingDamageMultiplier(ERoundDamageType DamageType) const
{
    double Multiplier = Super::GetIncomingDamageMultiplier(DamageType);

    const double ChainMultiplier = GetPassiveValueFromKey(TEXT("DamageMultiplierWhileHasChain"));
    if (GetCurrentChainCount() > 0 && ChainMultiplier > 0.0)
    {
        Multiplier *= ChainMultiplier;
    }

    const double StationaryMultiplier = GetPassiveValueFromKey(TEXT("StationaryIncomingDamageMultiplier"));
    if (bStationaryGuardActive && StationaryMultiplier > 0.0)
    {
        Multiplier *= StationaryMultiplier;
    }

    return Multiplier;
}

int32 ARoundCharacter1::GetIncomingBonusDefense(ERoundDamageType DamageType) const
{
    const int32 BonusDefense = Super::GetIncomingBonusDefense(DamageType);
    if (GetCurrentChainCount() > 0)
    {
        return BonusDefense;
    }

    return BonusDefense + FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("DefenseWhileNoChain"))));
}

bool ARoundCharacter1::TryPreventDeathFromPassive(int32 AppliedDamage, ARoundCharacter* SourceCharacter, ERoundDamageType DamageType)
{
    if (!Attributes || NativeDamageRechargeCooldownRemaining > 0)
    {
        return false;
    }

    const int32 HealPerChain = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("CheatDeathHealPerConsumedChain"))));
    if (HealPerChain <= 0)
    {
        return false;
    }

    const int32 ConsumedChains = GetCurrentChainCount();
    if (ConsumedChains <= 0)
    {
        return false;
    }

    const int32 CooldownInitiative = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("CheatDeathCooldownInitiative"))));
    NativeChainCount = 0;
    NativeDamageRechargeCooldownRemaining = CooldownInitiative;
    Attributes->CurrentHealth = FMath::Clamp(HealPerChain * ConsumedChains, 1, Attributes->BaseStats.MaxHealth);
    NotifyRoundStateChanged();
    return true;
}

bool ARoundCharacter1::TryForcedSetField(ARoundGridTile* NewTile, ARoundCharacter* SourceCharacter, bool bUpdateUI)
{
    if (!NewTile || NewTile == CurrentTile)
    {
        return false;
    }

    if (SourceCharacter && IsEnemyFor(SourceCharacter))
    {
        const double IgnoreChance = GetPassiveValueFromKey(TEXT("IgnoreEnemyForcedMovementChance"));
        if (IgnoreChance > 0.0 && FMath::FRand() <= IgnoreChance)
        {
            NotifyRoundStateChanged();
            return false;
        }
    }

    return Super::TryForcedSetField(NewTile, SourceCharacter, bUpdateUI);
}

bool ARoundCharacter1::ConsumeChains(int32 Amount, bool bAllowRefund)
{
    const int32 SafeAmount = FMath::Max(0, Amount);
    if (SafeAmount <= 0 || GetCurrentChainCount() < SafeAmount)
    {
        return false;
    }

    NativeChainCount = FMath::Max(0, NativeChainCount - SafeAmount);
    if (bAllowRefund && !bGrantedChainSpendRefundThisTurn && Attributes)
    {
        const int32 Refund = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("FirstChainSpendInitiativeRefund"))));
        if (Refund > 0)
        {
            Attributes->GainInitiative(Refund);
            bGrantedChainSpendRefundThisTurn = true;
        }
    }

    NotifyRoundStateChanged();
    return true;
}

int32 ARoundCharacter1::AdjustDebuffDurationInitiative(int32 BaseDuration) const
{
    const double Multiplier = GetPassiveValueFromKey(TEXT("DebuffDurationMultiplier"));
    if (BaseDuration <= 0 || Multiplier <= 0.0)
    {
        return BaseDuration;
    }

    return FMath::Max(1, FMath::RoundToInt(static_cast<double>(BaseDuration) * Multiplier));
}

bool ARoundCharacter1::HasDebuffAppliedToCharacter(const ARoundCharacter* TargetCharacter) const
{
    if (!TargetCharacter)
    {
        return false;
    }

    for (ARoundEffectActor* Effect : TargetCharacter->ActiveEffects)
    {
        if (Effect && Effect->CauserCharacter == this && Effect->IsDebuff())
        {
            return true;
        }
    }

    return false;
}

void ARoundCharacter1::GainChains(int32 Amount)
{
    if (Amount <= 0)
    {
        return;
    }

    NativeChainCount = FMath::Clamp(NativeChainCount + Amount, 0, GetChainLimit());
    NotifyRoundStateChanged();
}

void ARoundCharacter1::HandleChainRechargeTick(int32 InitiativeAmount)
{
    const int32 RechargeTime = GetChainRechargeTime();
    if (InitiativeAmount <= 0 || RechargeTime <= 0)
    {
        return;
    }

    NativeChainRechargeProgress += InitiativeAmount;
    while (NativeChainRechargeProgress >= RechargeTime)
    {
        NativeChainRechargeProgress -= RechargeTime;
        if (GetCurrentChainCount() < GetChainLimit())
        {
            ++NativeChainCount;
        }
        else if (Attributes)
        {
            const int32 OverflowInitiative = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("ChainOverflowInitiativeGain"))));
            if (OverflowInitiative > 0)
            {
                Attributes->GainInitiative(OverflowInitiative);
            }
        }
    }

    NotifyRoundStateChanged();
}
