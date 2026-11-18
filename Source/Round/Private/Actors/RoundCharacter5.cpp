#include "Actors/RoundCharacter5.h"

#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundCharacterData.h"
#include "Game/RoundGameState.h"
#include "Actors/RoundAbilityActor.h"
#include "Net/UnrealNetwork.h"

ARoundCharacter5::ARoundCharacter5()
{
    CharacterDataAsset = TSoftObjectPtr<URoundCharacterData>(FSoftObjectPath(TEXT("/Game/TopDown/Characters/Character_5/Character5Details.Character5Details")));
}

void ARoundCharacter5::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundCharacter5, NativeBlood);
    DOREPLIFETIME(ARoundCharacter5, bFirstDamageReductionAvailableThisTurn);
    DOREPLIFETIME(ARoundCharacter5, BloodCheatDeathCooldownTurnsRemaining);
    DOREPLIFETIME(ARoundCharacter5, bBasicAttackBloodAvailableThisTurn);
    DOREPLIFETIME(ARoundCharacter5, bBloodCapTriggerAvailable);
}

int32 ARoundCharacter5::GetCurrentBlood() const
{
    return FMath::Max(0, NativeBlood);
}

int32 ARoundCharacter5::GetBloodCap() const
{
    return FMath::Max(0, FMath::RoundToInt(GetValueFromKey(TEXT("BloodCap")) + GetPassiveValueFromKey(TEXT("BloodCapAdd"))));
}

double ARoundCharacter5::GetBloodGainFraction() const
{
    double Fraction = GetValueFromKey(TEXT("BloodGainFraction"));
    if (Fraction <= 0.0)
    {
        Fraction = GetValueFromKey(TEXT("BloodGainPercent"));
        if (Fraction > 0.0)
        {
            Fraction /= 100.0;
        }
    }

    if (Fraction <= 0.0)
    {
        Fraction = 0.1;
    }

    const double Multiplier = GetPassiveValueFromKey(TEXT("BloodGainMultiplier"));
    if (Multiplier > 0.0)
    {
        Fraction *= Multiplier;
    }

    return Fraction;
}

void ARoundCharacter5::GainBlood(int32 Amount)
{
    if (Amount <= 0)
    {
        return;
    }

    GainRawBlood(FMath::Max(0, FMath::RoundToInt(static_cast<double>(Amount) * GetBloodGainFraction())));
}

void ARoundCharacter5::GainRawBlood(int32 Amount)
{
    const int32 BloodCap = GetBloodCap();
    if (Amount <= 0 || BloodCap <= 0)
    {
        return;
    }

    const int32 PreviousBlood = NativeBlood;
    const int32 Overflow = FMath::Max(0, PreviousBlood + Amount - BloodCap);
    NativeBlood = FMath::Clamp(PreviousBlood + Amount, 0, BloodCap);

    if (Overflow > 0)
    {
        const double OverflowHealFraction = GetPassiveValueFromKey(TEXT("BloodOverflowHealFraction"));
        if (OverflowHealFraction > 0.0)
        {
            Heal(FMath::RoundToInt(static_cast<double>(Overflow) * OverflowHealFraction));
        }
    }

    if (PreviousBlood < BloodCap && NativeBlood >= BloodCap && bBloodCapTriggerAvailable)
    {
        const int32 CooldownReduction = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("CooldownReductionOnBloodCap"))));
        if (CooldownReduction > 0)
        {
            for (ARoundAbilityActor* Ability : GrantedAbilities)
            {
                if (Ability)
                {
                    Ability->ReduceCooldown(CooldownReduction);
                }
            }
            bBloodCapTriggerAvailable = false;
        }
    }
    NotifyRoundStateChanged();
}

int32 ARoundCharacter5::TryConsumeBlood(int32 MaximumAmount)
{
    if (NativeBlood <= 0)
    {
        return 0;
    }

    const int32 ConsumedBlood = MaximumAmount > 0 ? FMath::Min(NativeBlood, MaximumAmount) : NativeBlood;
    NativeBlood -= ConsumedBlood;
    if (Attributes)
    {
        const int32 InitiativeGain = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("InitiativeGainOnBloodSpend"))));
        if (InitiativeGain > 0)
        {
            Attributes->GainInitiative(InitiativeGain);
        }
        const int32 SpendThreshold = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("BloodSpendThresholdForInitiative"))));
        if (SpendThreshold > 0 && ConsumedBlood >= SpendThreshold)
        {
            Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("InitiativeGainOnLargeBloodSpend")))));
        }
    }
    NotifyRoundStateChanged();
    return ConsumedBlood;
}

void ARoundCharacter5::BeginTurn()
{
    bFirstDamageReductionAvailableThisTurn = true;
    bBasicAttackBloodAvailableThisTurn = true;
    bBloodCapTriggerAvailable = true;
    Super::BeginTurn();
}

void ARoundCharacter5::AdvanceGlobalInitiative(int32 Amount)
{
    Super::AdvanceGlobalInitiative(Amount);

    if (Amount > 0 && BloodCheatDeathCooldownTurnsRemaining > 0)
    {
        BloodCheatDeathCooldownTurnsRemaining = FMath::Max(0, BloodCheatDeathCooldownTurnsRemaining - Amount);
    }
}

int32 ARoundCharacter5::ModifyOutgoingDamage(int32 Amount, ERoundDamageType DamageType, ARoundCharacter* TargetCharacter, int32& DefenseIgnore) const
{
    int32 Damage = Super::ModifyOutgoingDamage(Amount, DamageType, TargetCharacter, DefenseIgnore);

    if (!Attributes || Attributes->BaseStats.MaxHealth <= 0)
    {
        return Damage;
    }

    const double LowHealthThresholdPercent = GetPassiveValueFromKey(TEXT("LowHealthThresholdPercent"));
    const double DamageMultiplierWhileLowHealth = GetPassiveValueFromKey(TEXT("DamageMultiplierWhileLowHealth"));
    if (LowHealthThresholdPercent > 0.0 && DamageMultiplierWhileLowHealth > 0.0)
    {
        const double HealthPercent = static_cast<double>(Attributes->CurrentHealth) / static_cast<double>(Attributes->BaseStats.MaxHealth);
        if (HealthPercent <= (LowHealthThresholdPercent / 100.0))
        {
            Damage = FMath::Max(0, FMath::RoundToInt(static_cast<double>(Damage) * DamageMultiplierWhileLowHealth));
        }
    }

    const int32 HighBloodThreshold = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("HighBloodDamageThreshold"))));
    const double HighBloodMultiplier = GetPassiveValueFromKey(TEXT("HighBloodDamageMultiplier"));
    if (HighBloodThreshold > 0 && NativeBlood >= HighBloodThreshold && HighBloodMultiplier > 0.0)
    {
        Damage = FMath::Max(0, FMath::RoundToInt(static_cast<double>(Damage) * HighBloodMultiplier));
    }

    return Damage;
}

double ARoundCharacter5::GetIncomingDamageMultiplier(ERoundDamageType DamageType) const
{
    double Multiplier = Super::GetIncomingDamageMultiplier(DamageType);
    if (bFirstDamageReductionAvailableThisTurn)
    {
        const double FirstHitMultiplier = GetPassiveValueFromKey(TEXT("FirstIncomingDamageMultiplierPerTurn"));
        if (FirstHitMultiplier > 0.0)
        {
            Multiplier *= FirstHitMultiplier;
        }
    }
    if (Attributes && Attributes->BaseStats.MaxHealth > 0)
    {
        const double Threshold = GetPassiveValueFromKey(TEXT("LowHealthThresholdPercent")) / 100.0;
        const double LowHealthMultiplier = GetPassiveValueFromKey(TEXT("IncomingDamageMultiplierWhileLowHealth"));
        const double HealthFraction = static_cast<double>(Attributes->CurrentHealth) / static_cast<double>(Attributes->BaseStats.MaxHealth);
        if (Threshold > 0.0 && HealthFraction <= Threshold && LowHealthMultiplier > 0.0)
        {
            Multiplier *= LowHealthMultiplier;
        }
    }
    return Multiplier;
}

int32 ARoundCharacter5::GetIncomingBonusDefense(ERoundDamageType DamageType) const
{
    const int32 Threshold = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("BloodDefenseThreshold"))));
    return Super::GetIncomingBonusDefense(DamageType)
        + ((Threshold > 0 && NativeBlood >= Threshold) ? FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("DefenseWhileHighBlood")))) : 0);
}

void ARoundCharacter5::HandlePassiveDamageTaken(int32 AppliedDamage, ARoundCharacter* SourceCharacter, ERoundDamageType DamageType)
{
    if (AppliedDamage <= 0 || !Attributes || Attributes->BaseStats.MaxHealth <= 0)
    {
        return;
    }

    const double TakenDamageBloodFraction = GetPassiveValueFromKey(TEXT("BloodGainFractionFromDamageTaken"));
    if (TakenDamageBloodFraction > 0.0)
    {
        GainRawBlood(FMath::RoundToInt(static_cast<double>(AppliedDamage) * TakenDamageBloodFraction));
    }

    const double LowHealthThresholdPercent = GetPassiveValueFromKey(TEXT("LowHealthThresholdPercent"));
    const double IncomingDamageMultiplierWhileLowHealth = GetPassiveValueFromKey(TEXT("IncomingDamageMultiplierWhileLowHealth"));
    if (bFirstDamageReductionAvailableThisTurn)
    {
        const double FirstHitMultiplier = GetPassiveValueFromKey(TEXT("FirstIncomingDamageMultiplierPerTurn"));
        if (FirstHitMultiplier > 0.0 && FirstHitMultiplier < 1.0)
        {
            bFirstDamageReductionAvailableThisTurn = false;
        }
    }

    if (LowHealthThresholdPercent > 0.0 && IncomingDamageMultiplierWhileLowHealth > 0.0)
    {
        const double HealthPercent = static_cast<double>(Attributes->CurrentHealth) / static_cast<double>(Attributes->BaseStats.MaxHealth);
        if (HealthPercent <= (LowHealthThresholdPercent / 100.0))
        {
            // Passive is accounted for in prevention/cheat-death flow; no extra action needed here.
        }
    }
}

void ARoundCharacter5::HandleEnemyHitByCharacter(ARoundCharacter* EnemyCharacter, int32 AppliedDamage, ERoundDamageType DamageType)
{
    Super::HandleEnemyHitByCharacter(EnemyCharacter, AppliedDamage, DamageType);
    if (!bBasicAttackBloodAvailableThisTurn || AppliedDamage <= 0)
    {
        return;
    }
    const int32 BonusBlood = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("FirstBasicAttackBloodGain"))));
    if (BonusBlood > 0)
    {
        GainRawBlood(BonusBlood);
        bBasicAttackBloodAvailableThisTurn = false;
    }
}

bool ARoundCharacter5::TryPreventDeathFromPassive(int32 AppliedDamage, ARoundCharacter* SourceCharacter, ERoundDamageType DamageType)
{
    if (!Attributes)
    {
        return false;
    }

    const int32 BloodCost = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("BloodCheatDeathCost"))));
    const int32 HealAmount = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("BloodCheatDeathHeal"))));
    const int32 CooldownInitiative = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("BloodCheatDeathCooldownInitiative"))));
    if (BloodCost <= 0 || HealAmount <= 0 || NativeBlood < BloodCost || BloodCheatDeathCooldownTurnsRemaining > 0)
    {
        return false;
    }

    NativeBlood = FMath::Max(0, NativeBlood - BloodCost);
    BloodCheatDeathCooldownTurnsRemaining = CooldownInitiative;
    Attributes->CurrentHealth = FMath::Max(1, HealAmount);
    NotifyRoundStateChanged();
    return true;
}
