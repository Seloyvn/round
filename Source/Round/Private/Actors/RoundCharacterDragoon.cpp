#include "Actors/RoundCharacterDragoon.h"

#include "Components/RoundAttributeSetComponent.h"
#include "Net/UnrealNetwork.h"

void ARoundCharacterDragoon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundCharacterDragoon, MomentumStacks);
}

void ARoundCharacterDragoon::BeginTurn()
{
    bHitEnemyThisTurn = false;
    Super::BeginTurn();
    if (Attributes && GetMomentumStacks() >= GetMomentumMaxStacks())
    {
        Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("InitiativeGainAtMaxMomentumTurnStart")))));
    }
}

void ARoundCharacterDragoon::EndTurn()
{
    if (HasAuthority() && !bHitEnemyThisTurn)
    {
        if (GetPassiveValueFromKey(TEXT("KeepAllMomentumOnMiss")) <= 0.0)
        {
            const int32 Retained = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("MomentumRetainedOnMiss"))));
            MomentumStacks = FMath::Min(MomentumStacks, Retained);
            NotifyRoundStateChanged();
            ForceNetUpdate();
        }
    }

    Super::EndTurn();
}

int32 ARoundCharacterDragoon::ModifyOutgoingDamage(int32 Amount, ERoundDamageType DamageType, ARoundCharacter* TargetCharacter, int32& DefenseIgnore) const
{
    const int32 BaseDamage = Super::ModifyOutgoingDamage(Amount, DamageType, TargetCharacter, DefenseIgnore);
    DefenseIgnore += GetMomentumStacks() * FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("DefenseIgnorePerMomentum"))));
    double Damage = BaseDamage + (GetMomentumStacks() * GetDamagePerStack());
    if (GetMomentumStacks() >= GetMomentumMaxStacks()) Damage *= FMath::Max(0.0, 1.0 + GetPassiveValueFromKey(TEXT("DamageMultiplierAddAtMaxMomentum")));
    return FMath::Max(0, FMath::RoundToInt(Damage));
}

int32 ARoundCharacterDragoon::GetIncomingBonusDefense(ERoundDamageType DamageType) const
{
    return Super::GetIncomingBonusDefense(DamageType)
        + GetMomentumStacks() * FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("DefensePerMomentum"))));
}

void ARoundCharacterDragoon::HandleEnemyHitByCharacter(ARoundCharacter* EnemyCharacter, int32 AppliedDamage, ERoundDamageType DamageType)
{
    if (!EnemyCharacter || !IsEnemyFor(EnemyCharacter))
    {
        return;
    }

    const bool bWasFirstHitThisTurn = !bHitEnemyThisTurn;
    bHitEnemyThisTurn = true;
    int32 Gain = 1;
    if (bWasFirstHitThisTurn) Gain += FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("BonusMomentumOnFirstHit"))));
    GainMomentumStacks(Gain);
    if (GetMomentumStacks() >= GetMomentumMaxStacks())
    {
        Heal(FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("HealOnHitAtMaxMomentum")))));
    }
}

int32 ARoundCharacterDragoon::GetMomentumStacks() const
{
    return FMath::Max(0, MomentumStacks);
}

int32 ARoundCharacterDragoon::GetMomentumMaxStacks() const
{
    const int32 BaseMax = FMath::RoundToInt(GetValueFromKey(TEXT("MomentumMaxStacks")));
    const int32 BonusMax = FMath::RoundToInt(GetPassiveValueFromKey(TEXT("MomentumMaxStacks")));
    const int32 GenericBonus = FMath::RoundToInt(GetPassiveValueFromKey(TEXT("MaxStacks")));
    return FMath::Max(1, (BaseMax > 0 ? BaseMax : 4) + BonusMax + GenericBonus);
}

int32 ARoundCharacterDragoon::GainMomentumStacks(int32 Amount)
{
    if (!HasAuthority() || Amount <= 0)
    {
        return GetMomentumStacks();
    }

    MomentumStacks = FMath::Clamp(MomentumStacks + Amount, 0, GetMomentumMaxStacks());
    NotifyRoundStateChanged();
    ForceNetUpdate();
    return MomentumStacks;
}

void ARoundCharacterDragoon::RecordEnemyHit(ARoundCharacter* EnemyCharacter, int32 AppliedDamage, ERoundDamageType DamageType)
{
    HandleEnemyHitByCharacter(EnemyCharacter, AppliedDamage, DamageType);
}

void ARoundCharacterDragoon::ResetMomentumStacks()
{
    if (!HasAuthority())
    {
        return;
    }

    if (MomentumStacks == 0)
    {
        return;
    }

    MomentumStacks = 0;
    NotifyRoundStateChanged();
    ForceNetUpdate();
}

int32 ARoundCharacterDragoon::GetDamagePerStack() const
{
    const int32 BaseDamage = FMath::RoundToInt(GetValueFromKey(TEXT("DamagePerStack")));
    const int32 BonusDamage = FMath::RoundToInt(GetPassiveValueFromKey(TEXT("DamagePerStack")));
    const int32 AlternateBonus = FMath::RoundToInt(GetPassiveValueFromKey(TEXT("MomentumDamagePerStack")));
    return FMath::Max(0, (BaseDamage > 0 ? BaseDamage : 25) + BonusDamage + AlternateBonus);
}
