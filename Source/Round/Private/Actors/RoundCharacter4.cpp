#include "Actors/RoundCharacter4.h"

#include "Actors/RoundAbilityActor.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundAbilityData.h"
#include "Data/RoundCharacterData.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogRoundCharacter4Passive, Log, All);

namespace
{
double GetConfiguredMultiplier(const ARoundCharacter4* Character, const TCHAR* Key, const double DefaultValue)
{
    if (!Character)
    {
        return DefaultValue;
    }

    double ConfiguredValue = Character->GetPassiveValueFromKey(Key);
    if (ConfiguredValue <= 0.0)
    {
        ConfiguredValue = Character->GetValueFromKey(Key);
    }
    return ConfiguredValue > 0.0 ? ConfiguredValue : DefaultValue;
}

int32 GetConfiguredInt(const ARoundCharacter4* Character, const TCHAR* Key, const int32 DefaultValue)
{
    if (!Character)
    {
        return DefaultValue;
    }

    double ConfiguredValue = Character->GetPassiveValueFromKey(Key);
    if (ConfiguredValue <= 0.0)
    {
        ConfiguredValue = Character->GetValueFromKey(Key);
    }
    return ConfiguredValue > 0.0 ? FMath::RoundToInt(ConfiguredValue) : DefaultValue;
}
}

ARoundCharacter4::ARoundCharacter4()
{
    CharacterDataAsset = TSoftObjectPtr<URoundCharacterData>(FSoftObjectPath(TEXT("/Game/TopDown/Characters/Character_4/Character4Details.Character4Details")));
}

void ARoundCharacter4::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundCharacter4, ActiveElementBonuses);
    DOREPLIFETIME(ARoundCharacter4, UsedElementBonusesThisTurn);
    DOREPLIFETIME(ARoundCharacter4, bMovedThisTurn);
    DOREPLIFETIME(ARoundCharacter4, LastCastElementBonus);
    DOREPLIFETIME(ARoundCharacter4, FirstCastElementBonusThisTurn);
    DOREPLIFETIME(ARoundCharacter4, ElementSwapCooldownTurnsRemaining);
    DOREPLIFETIME(ARoundCharacter4, CurrentAbilityDamageMultiplier);
    DOREPLIFETIME(ARoundCharacter4, ElementalRockDefenseBonus);
    DOREPLIFETIME(ARoundCharacter4, ElementalRockDefenseRemainingInitiative);
}

void ARoundCharacter4::OnRep_ElementBonusState()
{
    UE_LOG(LogRoundCharacter4Passive, Log, TEXT("OnRep_ElementBonusState - Character=%s ActiveBonuses=%d UsedThisTurn=%d"),
        *GetName(),
        ActiveElementBonuses.Num(),
        UsedElementBonusesThisTurn.Num());
    NotifyRoundStateChanged();
}

void ARoundCharacter4::InitializeCharacter()
{
    Super::InitializeCharacter();
    ActiveElementBonuses.Reset();
    UsedElementBonusesThisTurn.Reset();
    ElementalRockDefenseBonus = 0;
    ElementalRockDefenseRemainingInitiative = 0;
    bElementCostReductionUsedThisTurn = false;
    bElementSwapRefundUsedThisTurn = false;
    bStoredElementEmpowered = false;
    bProtectionReady = false;
    ProtectionRemainingInitiative = 0;
    EchoElement = ERoundMageElementBonus::None;
    ElementCycle.Reset();
}

void ARoundCharacter4::ApplySimMageState(
    const TArray<ERoundMageElementBonus>& InActiveElementBonuses,
    const TArray<ERoundMageElementBonus>& InUsedElementBonusesThisTurn,
    const bool bInMovedThisTurn,
    const ERoundMageElementBonus InLastCastElementBonus,
    const ERoundMageElementBonus InFirstCastElementBonusThisTurn,
    const int32 InElementSwapCooldownTurnsRemaining,
    const double InCurrentAbilityDamageMultiplier)
{
    ActiveElementBonuses = InActiveElementBonuses;
    UsedElementBonusesThisTurn = InUsedElementBonusesThisTurn;
    bMovedThisTurn = bInMovedThisTurn;
    LastCastElementBonus = InLastCastElementBonus;
    FirstCastElementBonusThisTurn = InFirstCastElementBonusThisTurn;
    ElementSwapCooldownTurnsRemaining = FMath::Max(0, InElementSwapCooldownTurnsRemaining);
    CurrentAbilityDamageMultiplier = InCurrentAbilityDamageMultiplier > 0.0 ? InCurrentAbilityDamageMultiplier : 1.0;
    NotifyRoundStateChanged();
}

void ARoundCharacter4::ApplyCoreMageState(const int32 InActiveElement, const uint32 InUsedElementMask,
    const int32 InLastCastElement, const int32 InFirstCastElement, const int32 InElementSwapCooldownRemaining,
    const int32 InRockDefenseBonus, const int32 InRockDefenseRemainingInitiative,
    const int32 InProtectionRemainingInitiative, const int32 InEchoElement, const uint32 InElementCycleMask,
    const bool bInElementCostReductionUsed, const bool bInElementSwapRefundUsed,
    const bool bInStoredElementEmpowered, const bool bInProtectionReady)
{
    ActiveElementBonuses.Reset();
    if (InActiveElement > 0 && InActiveElement <= static_cast<int32>(ERoundMageElementBonus::RockGuard))
        ActiveElementBonuses.Add(static_cast<ERoundMageElementBonus>(InActiveElement));
    UsedElementBonusesThisTurn.Reset();
    for (int32 Element = 1; Element <= static_cast<int32>(ERoundMageElementBonus::RockGuard); ++Element)
        if ((InUsedElementMask & (1u << Element)) != 0u) UsedElementBonusesThisTurn.Add(static_cast<ERoundMageElementBonus>(Element));
    LastCastElementBonus = static_cast<ERoundMageElementBonus>(FMath::Clamp(InLastCastElement, 0, 4));
    FirstCastElementBonusThisTurn = static_cast<ERoundMageElementBonus>(FMath::Clamp(InFirstCastElement, 0, 4));
    ElementSwapCooldownTurnsRemaining = FMath::Max(0, InElementSwapCooldownRemaining);
    ElementalRockDefenseBonus = FMath::Max(0, InRockDefenseBonus);
    ElementalRockDefenseRemainingInitiative = FMath::Max(0, InRockDefenseRemainingInitiative);
    ProtectionRemainingInitiative = FMath::Max(0, InProtectionRemainingInitiative);
    EchoElement = static_cast<ERoundMageElementBonus>(FMath::Clamp(InEchoElement, 0, 4));
    ElementCycle.Reset();
    for (int32 Element = 1; Element <= 4; ++Element)
        if ((InElementCycleMask & (1u << Element)) != 0u) ElementCycle.Add(static_cast<ERoundMageElementBonus>(Element));
    bElementCostReductionUsedThisTurn = bInElementCostReductionUsed;
    bElementSwapRefundUsedThisTurn = bInElementSwapRefundUsed;
    bStoredElementEmpowered = bInStoredElementEmpowered;
    bProtectionReady = bInProtectionReady;
    NotifyRoundStateChanged();
}

void ARoundCharacter4::BeginTurn()
{
    Super::BeginTurn();
    bElementCostReductionUsedThisTurn = false;
    bElementSwapRefundUsedThisTurn = false;
}

void ARoundCharacter4::AdvanceGlobalInitiative(int32 Amount)
{
    Super::AdvanceGlobalInitiative(Amount);
    if (ElementalRockDefenseRemainingInitiative > 0)
    {
        ElementalRockDefenseRemainingInitiative = FMath::Max(0, ElementalRockDefenseRemainingInitiative - FMath::Max(0, Amount));
        if (ElementalRockDefenseRemainingInitiative == 0)
        {
            ElementalRockDefenseBonus = 0;
        }
        NotifyRoundStateChanged();
    }

    if (ProtectionRemainingInitiative > 0)
    {
        ProtectionRemainingInitiative = FMath::Max(0, ProtectionRemainingInitiative - FMath::Max(0, Amount));
        if (ProtectionRemainingInitiative == 0)
        {
            bProtectionReady = false;
        }
    }
}

FRoundActionResult ARoundCharacter4::MoveAlongPath(const TArray<ARoundGridTile*>& Path)
{
    return Super::MoveAlongPath(Path);
}

void ARoundCharacter4::EndTurn()
{
    Super::EndTurn();
}

void ARoundCharacter4::RegisterAbilityDamageType(ERoundDamageType DamageType)
{
    const ERoundMageElementBonus NewBonus = DamageTypeToElementBonus(DamageType);
    if (NewBonus == ERoundMageElementBonus::None)
    {
        UE_LOG(LogRoundCharacter4Passive, Warning, TEXT("RegisterAbilityDamageType ignored - Character=%s DamageType=%d PreviousBonus=%d"),
            *GetName(),
            static_cast<int32>(DamageType),
            static_cast<int32>(GetActiveElementBonus()));
        return;
    }

    ActiveElementBonuses.Reset();
    ActiveElementBonuses.Add(NewBonus);
    UsedElementBonusesThisTurn.Remove(NewBonus);

    UE_LOG(LogRoundCharacter4Passive, Log, TEXT("RegisterAbilityDamageType applied - Character=%s DamageType=%d NewBonus=%d ActiveCount=%d"),
        *GetName(),
        static_cast<int32>(DamageType),
        static_cast<int32>(NewBonus),
        ActiveElementBonuses.Num());
    NotifyRoundStateChanged();
}

ERoundMageElementBonus ARoundCharacter4::GetActiveElementBonus() const
{
    return ActiveElementBonuses.Num() > 0 ? ActiveElementBonuses[0] : ERoundMageElementBonus::None;
}

bool ARoundCharacter4::IsElementBonusActive(const ERoundMageElementBonus Bonus) const
{
    return ActiveElementBonuses.Contains(Bonus);
}

bool ARoundCharacter4::WasElementBonusUsedThisTurn(const ERoundMageElementBonus Bonus) const
{
    return UsedElementBonusesThisTurn.Contains(Bonus);
}

void ARoundCharacter4::ClearElementBonus()
{
    if (ActiveElementBonuses.Num() == 0)
    {
        return;
    }

    UE_LOG(LogRoundCharacter4Passive, Log, TEXT("ClearElementBonus - Character=%s ClearedAll=%d UsedThisTurnBefore=%d"),
        *GetName(),
        ActiveElementBonuses.Num(),
        UsedElementBonusesThisTurn.Num());
    ActiveElementBonuses.Reset();
    NotifyRoundStateChanged();
}

void ARoundCharacter4::ClearSpecificElementBonus(const ERoundMageElementBonus Bonus)
{
    if (!ActiveElementBonuses.Contains(Bonus))
    {
        return;
    }

    UE_LOG(LogRoundCharacter4Passive, Log, TEXT("ClearSpecificElementBonus - Character=%s Bonus=%d"),
        *GetName(),
        static_cast<int32>(Bonus));
    ActiveElementBonuses.RemoveSingle(Bonus);
    NotifyRoundStateChanged();
}

void ARoundCharacter4::ConsumeElementBonus(const ERoundMageElementBonus Bonus)
{
    if (!ActiveElementBonuses.Contains(Bonus))
    {
        return;
    }

    UE_LOG(LogRoundCharacter4Passive, Log, TEXT("ConsumeElementBonus - Character=%s ConsumedBonus=%d"),
        *GetName(),
        static_cast<int32>(Bonus));
    UsedElementBonusesThisTurn.AddUnique(Bonus);
    ActiveElementBonuses.RemoveSingle(Bonus);
    if (Attributes)
    {
        const int32 InitiativeGain = FMath::Max(0, GetConfiguredInt(this, TEXT("InitiativeGainOnBonusConsumed"), 0));
        if (InitiativeGain > 0)
        {
            Attributes->GainInitiative(InitiativeGain);
        }
    }
    NotifyRoundStateChanged();
}

int32 ARoundCharacter4::AdjustEffectDuration(int32 Duration)
{
    if (IsElementBonusActive(ERoundMageElementBonus::IceControl))
    {
        const int32 BonusDuration = FMath::Max(0, GetConfiguredInt(this, TEXT("PassiveIceBonusDuration"), 1));
        UE_LOG(LogRoundCharacter4Passive, Log, TEXT("AdjustEffectDuration - Character=%s BaseDuration=%d BonusDuration=%d"),
            *GetName(),
            Duration,
            BonusDuration);
        ConsumeElementBonus(ERoundMageElementBonus::IceControl);
        return Duration + BonusDuration;
    }

    return Duration;
}

int32 ARoundCharacter4::GetAttackCost() const
{
    int32 Cost = Super::GetAttackCost();
    if (ActiveElementBonuses.Num() > 0)
    {
        Cost = FMath::Max(1, Cost - FMath::Max(0, GetConfiguredInt(this, TEXT("AttackCostReductionWhileElementBonus"), 0)));
    }

    return Cost;
}

FRoundActionResult ARoundCharacter4::UseAbilityIndex(int32 AbilityIndex, ARoundGridTile* TargetTile)
{
    if (!GrantedAbilities.IsValidIndex(AbilityIndex) || !GrantedAbilities[AbilityIndex])
    {
        return Super::UseAbilityIndex(AbilityIndex, TargetTile);
    }

    ARoundAbilityActor* Ability = GrantedAbilities[AbilityIndex];
    const ERoundMageElementBonus StoredElement = GetActiveElementBonus();
    CurrentCastElement = Ability && Ability->AbilityData
        ? DamageTypeToElementBonus(Ability->AbilityData->DamageType)
        : ERoundMageElementBonus::None;

    bAllElementsActiveForCast = false;
    if (GetConfiguredInt(this, TEXT("AllElementsOnCompletedCycle"), 0) > 0)
    {
        if (CurrentCastElement == ERoundMageElementBonus::None)
        {
            ElementCycle.Reset();
        }
        else if (ElementCycle.Num() == 3 && !ElementCycle.Contains(CurrentCastElement))
        {
            bAllElementsActiveForCast = true;
        }
    }

    bResolvingElementalAbility = true;
    bCountRockElementHits = HasElementForCurrentCast(ERoundMageElementBonus::RockGuard);
    RockElementEnemyHits = 0;
    CurrentAbilityEnemyHits = 0;
    CurrentAbilityDamageMultiplier = HasElementForCurrentCast(ERoundMageElementBonus::FirePower)
        ? 1.0 + (GetConfiguredMultiplier(this, TEXT("ElementFireDamageMultiplier"), 1.25) - 1.0) * GetElementStrength(ERoundMageElementBonus::FirePower)
        : 1.0;

    FRoundActionResult Result = Super::UseAbilityIndex(AbilityIndex, TargetTile);
    bResolvingElementalAbility = false;
    bCountRockElementHits = false;
    CurrentAbilityDamageMultiplier = 1.0;

    if (!Result.bSuccess)
    {
        RockElementEnemyHits = 0;
        CurrentAbilityEnemyHits = 0;
        CurrentCastElement = ERoundMageElementBonus::None;
        bAllElementsActiveForCast = false;
        return Result;
    }

    if (GetConfiguredInt(this, TEXT("FirstElementUseCostReduction"), 0) > 0 && StoredElement != ERoundMageElementBonus::None)
    {
        bElementCostReductionUsedThisTurn = true;
    }

    if (!bElementSwapRefundUsedThisTurn
        && StoredElement != ERoundMageElementBonus::None
        && CurrentCastElement != ERoundMageElementBonus::None
        && StoredElement != CurrentCastElement)
    {
        const int32 Refund = GetConfiguredInt(this, TEXT("DifferentElementInitiativeRefund"), 0);
        if (Refund > 0 && Attributes)
        {
            Attributes->GainInitiative(Refund);
            bElementSwapRefundUsedThisTurn = true;
        }
    }

    if (HasElementForCurrentCast(ERoundMageElementBonus::RockGuard) && RockElementEnemyHits > 0)
    {
        const int32 DefensePerHit = FMath::Max(0, FMath::RoundToInt(
            GetConfiguredInt(this, TEXT("ElementRockDefensePerEnemyHit"), 10) * GetElementStrength(ERoundMageElementBonus::RockGuard)));
        const int32 MaxStacks = FMath::Max(1, GetConfiguredInt(this, TEXT("ElementRockMaxDefenseStacks"), 4));
        ElementalRockDefenseBonus = FMath::Min(MaxStacks * DefensePerHit, ElementalRockDefenseBonus + RockElementEnemyHits * DefensePerHit);
        ElementalRockDefenseRemainingInitiative = FMath::Max(1, GetConfiguredInt(this, TEXT("ElementRockDefenseDurationInitiative"), 80));
    }

    if (StoredElement == ERoundMageElementBonus::None && EchoElement == ERoundMageElementBonus::None && !bAllElementsActiveForCast)
    {
        const int32 CooldownReduction = GetConfiguredInt(this, TEXT("CooldownReductionWithoutElement"), 0);
        if (CooldownReduction > 0)
        {
            Ability->ReduceCooldown(CooldownReduction);
        }
    }

    if (StoredElement != ERoundMageElementBonus::None)
    {
        const double ProtectionMultiplier = GetConfiguredMultiplier(this, TEXT("ElementProtectionIncomingDamageMultiplier"), 1.0);
        if (ProtectionMultiplier < 1.0)
        {
            bProtectionReady = true;
            ProtectionRemainingInitiative = FMath::Max(1, GetConfiguredInt(this, TEXT("ElementProtectionDurationInitiative"), 80));
        }
    }

    const bool bIsArcaneShift = Ability && Ability->AbilityData && Ability->AbilityData->AbilityId == FName(TEXT("ArcaneShift"));
    const bool bPreserveStoredElement = bIsArcaneShift
        && StoredElement != ERoundMageElementBonus::None
        && GetConfiguredInt(this, TEXT("ArcaneShiftPreservesElement"), 0) > 0;

    EchoElement = GetConfiguredInt(this, TEXT("ElementEcho"), 0) > 0
        ? StoredElement
        : ERoundMageElementBonus::None;

    if (!bPreserveStoredElement)
    {
        if (CurrentCastElement == ERoundMageElementBonus::None)
        {
            ClearElementBonus();
        }
        else
        {
            RegisterAbilityDamageType(Ability->AbilityData->DamageType);
        }
    }

    bStoredElementEmpowered = CurrentCastElement != ERoundMageElementBonus::None
        && CurrentAbilityEnemyHits >= 2
        && GetConfiguredInt(this, TEXT("EmpowerStoredElementOnMultiHit"), 0) > 0;

    if (CurrentCastElement == ERoundMageElementBonus::None)
    {
        ElementCycle.Reset();
    }
    else if (bAllElementsActiveForCast)
    {
        ElementCycle.Reset();
    }
    else if (ElementCycle.Contains(CurrentCastElement))
    {
        ElementCycle.Reset();
        ElementCycle.Add(CurrentCastElement);
    }
    else
    {
        ElementCycle.Add(CurrentCastElement);
    }

    RockElementEnemyHits = 0;
    CurrentAbilityEnemyHits = 0;
    CurrentCastElement = ERoundMageElementBonus::None;
    bAllElementsActiveForCast = false;
    NotifyRoundStateChanged();
    return Result;
}

int32 ARoundCharacter4::ApplyRoundDamage(int32 Amount, ERoundDamageType DamageType, int32 DefenseIgnore, ARoundCharacter* SourceCharacter, bool& bDidDie)
{
    if (Amount > 0 && bProtectionReady)
    {
        Amount = FMath::RoundToInt(Amount * FMath::Clamp(
            GetConfiguredMultiplier(this, TEXT("ElementProtectionIncomingDamageMultiplier"), 0.8), 0.0, 1.0));
        bProtectionReady = false;
        ProtectionRemainingInitiative = 0;
    }
    return Super::ApplyRoundDamage(Amount, DamageType, DefenseIgnore, SourceCharacter, bDidDie);
}

int32 ARoundCharacter4::ModifyOutgoingDamage(int32 Amount, ERoundDamageType DamageType, ARoundCharacter* TargetCharacter, int32& DefenseIgnore) const
{
    int32 Damage = Super::ModifyOutgoingDamage(Amount, DamageType, TargetCharacter, DefenseIgnore);
    if (bResolvingElementalAbility && CurrentAbilityDamageMultiplier > 0.0)
    {
        Damage = FMath::RoundToInt(static_cast<double>(Damage) * CurrentAbilityDamageMultiplier);
    }
    return FMath::Max(0, Damage);
}

double ARoundCharacter4::GetStoredElementDamageMultiplier() const
{
    return HasElementForCurrentCast(ERoundMageElementBonus::FirePower)
        ? 1.0 + (GetConfiguredMultiplier(this, TEXT("ElementFireDamageMultiplier"), 1.25) - 1.0) * GetElementStrength(ERoundMageElementBonus::FirePower)
        : 1.0;
}

int32 ARoundCharacter4::GetStoredElementRangeAdd() const
{
    return HasElementForCurrentCast(ERoundMageElementBonus::IceControl)
        ? FMath::Max(0, FMath::RoundToInt(GetConfiguredInt(this, TEXT("ElementIceRangeAdd"), 2) * GetElementStrength(ERoundMageElementBonus::IceControl)))
        : 0;
}

double ARoundCharacter4::GetStoredElementCooldownMultiplier() const
{
    return HasElementForCurrentCast(ERoundMageElementBonus::LightningTempo)
        ? FMath::Clamp(1.0 - (1.0 - GetConfiguredMultiplier(this, TEXT("ElementLightningCooldownMultiplier"), 0.75))
            * GetElementStrength(ERoundMageElementBonus::LightningTempo), 0.0, 1.0)
        : 1.0;
}

int32 ARoundCharacter4::GetStoredElementAbilityCostReduction() const
{
    return GetActiveElementBonus() != ERoundMageElementBonus::None && !bElementCostReductionUsedThisTurn
        ? FMath::Max(0, GetConfiguredInt(this, TEXT("FirstElementUseCostReduction"), 0))
        : 0;
}

int32 ARoundCharacter4::GetIncomingBonusDefense(ERoundDamageType DamageType) const
{
    const int32 StoredElementDefense = GetActiveElementBonus() != ERoundMageElementBonus::None
        ? FMath::Max(0, GetConfiguredInt(this, TEXT("DefenseWhileElementStored"), 0))
        : 0;
    return Super::GetIncomingBonusDefense(DamageType) + FMath::Max(0, ElementalRockDefenseBonus) + StoredElementDefense;
}

void ARoundCharacter4::HandleDamageDealt(ARoundCharacter* EnemyCharacter, int32 AppliedDamage, ERoundDamageType DamageType)
{
    Super::HandleDamageDealt(EnemyCharacter, AppliedDamage, DamageType);
    if (bResolvingElementalAbility && EnemyCharacter && AppliedDamage > 0)
    {
        ++CurrentAbilityEnemyHits;
        if (bCountRockElementHits)
        {
            ++RockElementEnemyHits;
        }
    }
}

double ARoundCharacter4::GetElementStrength(const ERoundMageElementBonus Element) const
{
    double Strength = GetConfiguredMultiplier(this, TEXT("ElementMasteryMultiplier"), 1.0);
    if (bStoredElementEmpowered && GetActiveElementBonus() == Element)
    {
        Strength *= GetConfiguredMultiplier(this, TEXT("EmpoweredStoredElementMultiplier"), 1.25);
    }
    if (GetActiveElementBonus() == Element && CurrentCastElement == Element)
    {
        Strength *= GetConfiguredMultiplier(this, TEXT("SameElementEffectMultiplier"), 1.5);
    }
    return Strength;
}

bool ARoundCharacter4::HasElementForCurrentCast(const ERoundMageElementBonus Element) const
{
    return bAllElementsActiveForCast || GetActiveElementBonus() == Element || EchoElement == Element;
}

int32 ARoundCharacter4::GetActiveElementBonusCount() const
{
    return ActiveElementBonuses.Num();
}

ERoundMageElementBonus ARoundCharacter4::DamageTypeToElementBonus(const ERoundDamageType DamageType) const
{
    switch (DamageType)
    {
    case ERoundDamageType::Fire:
        return ERoundMageElementBonus::FirePower;
    case ERoundDamageType::Ice:
        return ERoundMageElementBonus::IceControl;
    case ERoundDamageType::Lightning:
        return ERoundMageElementBonus::LightningTempo;
    case ERoundDamageType::Ground:
        return ERoundMageElementBonus::RockGuard;
    default:
        return ERoundMageElementBonus::None;
    }
}

void ARoundCharacter4::MaybeGrantElementSwapInitiative(ERoundMageElementBonus NewBonus)
{
    if (!Attributes || NewBonus == ERoundMageElementBonus::None)
    {
        return;
    }

    const int32 InitiativeGain = FMath::Max(0, GetConfiguredInt(this, TEXT("InitiativeGainOnElementSwap"), 0));
    const int32 CooldownTurns = FMath::Max(0, GetConfiguredInt(this, TEXT("ElementSwapInitiativeCooldownInitiative"), 0));
    if (InitiativeGain <= 0 || LastCastElementBonus == ERoundMageElementBonus::None || LastCastElementBonus == NewBonus || ElementSwapCooldownTurnsRemaining > 0)
    {
        return;
    }

    Attributes->GainInitiative(InitiativeGain);
    ElementSwapCooldownTurnsRemaining = CooldownTurns;
}
