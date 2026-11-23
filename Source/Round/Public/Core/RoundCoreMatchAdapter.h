#pragma once

#include "CoreMinimal.h"
#include "round/core/Game.h"

class ARoundCharacter;
class ARoundGameState;
class ARoundGridTile;
class ARoundAbilityActor;
enum class ERoundDamageType : uint8;

struct FRoundCoreAbilityRuntimeUpdate
{
    ARoundAbilityActor* Ability = nullptr;
    int32 Cooldown = 0;
    int32 Charges = 0;
    int32 ReadyInitiativeElapsed = 0;
};

struct FRoundCoreBasicAttackPreview
{
    int32 InitiativeCost = 0;
    int32 Damage = 0;
    bool bTargetDied = false;
    bool bMatchEnded = false;
    bool bCritical = false;
};

struct FRoundCoreCharacterRuntimeUpdate
{
    ARoundCharacter* Character = nullptr;
    ARoundGridTile* Tile = nullptr;
    int32 Health = 0;
    int32 Initiative = 0;
    bool bAlive = false;
    int32 ResourceCurrent = 0;
    int32 ResourceRechargeProgress = 0;
    int32 SpecialCooldownRemaining = 0;
    bool bFirstResourceSpendRefundAvailable = true;
    bool bMovedSinceLastTurn = false;
    bool bStationaryGuardActive = false;
    bool bAttackedThisTurn = false;
    bool bSpentResourceThisTurn = false;
    bool bStartedTurnWithFullResource = false;
    bool bHeldPositionFromPreviousTurn = false;
    bool bUsedFreeBasicAttackThisTurn = false;
    bool bTriggeredEmptyResourceThisTurn = false;
    bool bRefundedLastResourceKillThisTurn = false;
    bool bUsedRangeRefundThisTurn = false;
    bool bUsedKillCooldownReductionThisTurn = false;
    int32 StationaryShotStacks = 0;
    bool bHasFreePreparedStep = false;
    int32 MovedFieldsThisTurn = 0;
    int32 ResourceSpentThisTurnCount = 0;
    int32 SecondaryCooldownRemaining = 0;
    int32 TurnSequence = 0;
    bool bOpeningRewardUsedThisTurn = false;
    bool bOverflowRewardUsedThisTurn = false;
    bool bFreeAdjacentMoveAvailable = false;
    bool bFreeAdjacentMoveUsedThisTurn = false;
    bool bNextDamageAfterMoveReady = false;
    bool bNextReductionAfterMoveReady = false;
    bool bStartInvisibilityApplied = false;
    int32 ActiveElement = 0;
    uint32 UsedElementMask = 0;
    int32 LastCastElement = 0;
    int32 FirstCastElement = 0;
    int32 ElementSwapCooldownRemaining = 0;
    int32 RockDefenseBonus = 0;
    int32 RockDefenseRemainingInitiative = 0;
    int32 ProtectionRemainingInitiative = 0;
    int32 EchoElement = 0;
    uint32 ElementCycleMask = 0;
    bool bElementCostReductionUsed = false;
    bool bElementSwapRefundUsed = false;
    bool bStoredElementEmpowered = false;
    bool bProtectionReady = false;
};

struct FRoundCoreEffectRemoval
{
    ARoundCharacter* Character = nullptr;
    FString EffectId;
};

struct FRoundCoreHealingEvent
{
    ARoundCharacter* Character = nullptr;
    int32 Amount = 0;
};

struct FRoundCoreDamageEvent
{
    ARoundCharacter* Source = nullptr;
    ARoundCharacter* Target = nullptr;
    int32 Amount = 0;
    bool bTargetDied = false;
    ERoundDamageType DamageType = static_cast<ERoundDamageType>(0);
};

struct FRoundCoreModifierEffectAddition
{
    ARoundCharacter* Character = nullptr;
    ARoundCharacter* Causer = nullptr;
    ARoundCharacter* PreferredTarget = nullptr;
    FString EffectId;
    int32 RemainingInitiative = 0;
    int32 IncomingDamageMultiplierBasisPoints = 10000;
    int32 OutgoingDamageMultiplierBasisPoints = 10000;
    int32 DefenseIgnoreAdd = 0;
    int32 HealFractionFromDamageDealtBasisPoints = 0;
    int32 MoveCostAdd = 0;
    bool bPreventsAbilityUse = false;
    int32 RedirectPreMitigationBasisPoints = 0;
    int32 RedirectedDamageMultiplierBasisPoints = 10000;
    int32 ProtectorHealFractionBasisPoints = 0;
    int32 ProtectedInitiativePerHit = 0;
    ARoundGridTile* AreaCenterTile = nullptr;
    int32 AreaRadius = 0;
    int32 AreaDamage = 0;
    ERoundDamageType AreaDamageType = static_cast<ERoundDamageType>(0);
    int32 AreaInitiativeLossPerPulse = 0;
    bool bAreaFollowsCauser = false;
    bool bIsDebuff = false;
    bool bIsBind = false;
    bool bIsTaunt = false;
    int32 MoveCostMultiplierBasisPoints = 10000;
    int32 InitiativeLossOnMove = 0;
    int32 InitiativeLossOnExpire = 0;
    bool bBlockMovementBonuses = false;
    bool bOnlyAffectCauserDamage = false;
    int32 IncomingDamageMultiplierFromCauserBasisPoints = 10000;
    int32 IncomingEffectDurationAdd = 0;
    int32 TriggerDamage = 0;
    int32 TriggerDelay = 0;
    int32 TriggerOutgoingDamageMultiplierBasisPoints = 10000;
    int32 TriggerSlowMultiplierBasisPoints = 10000;
    int32 TriggerSlowDurationInitiative = 0;
    int32 TriggerAreaRadius = 0;
    int32 TriggerAreaInitiativeLoss = 0;
    int32 TriggerInitiativeGainForCauser = 0;
    int32 InitiativeLossWhenIgnoringCauser = 0;
    int32 InitiativeGainForCauserWhenAttacked = 0;
    int32 OutgoingDamageMultiplierAgainstOtherTargetsBasisPoints = 10000;
    FString SourceAbilityId;
    int32 CoreEffectKind = 0;
    int32 CounterA = 0;
    int32 CounterB = 0;
    int32 CounterC = 0;
    int32 ValueA = 0;
    int32 ValueB = 0;
    int32 ValueC = 0;
    int32 Flags = 0;
};

struct FRoundCoreTurnExecution
{
    TArray<FRoundCoreCharacterRuntimeUpdate> CharacterUpdates;
    TArray<FRoundCoreAbilityRuntimeUpdate> AbilityUpdates;
    TArray<FRoundCoreEffectRemoval> RemovedEffects;
    TArray<FRoundCoreDamageEvent> DamageEvents;
    TArray<TPair<ARoundCharacter*, int32>> AppliedDelays;
    ARoundCharacter* ActiveCharacter = nullptr;
    int32 GlobalInitiativeAdvance = 0;
};

struct FRoundCoreBasicAttackExecution
{
    TArray<FRoundCoreCharacterRuntimeUpdate> CharacterUpdates;
    TArray<FRoundCoreAbilityRuntimeUpdate> AbilityUpdates;
    TArray<FRoundCoreEffectRemoval> RemovedEffects;
    TArray<FRoundCoreDamageEvent> DamageEvents;
    TArray<FRoundCoreModifierEffectAddition> AddedModifierEffects;
    int32 AppliedDamage = 0;
    bool bCritical = false;
    bool bTargetDied = false;
};

struct FRoundCoreBoardDamageEvent
{
    ARoundGridTile* Tile = nullptr;
    int32 Amount = 0;
    bool bTerrainObject = false;
};

struct FRoundCoreAbilityExecution
{
    TArray<FRoundCoreCharacterRuntimeUpdate> CharacterUpdates;
    TArray<FRoundCoreAbilityRuntimeUpdate> AbilityUpdates;
    TArray<FRoundCoreEffectRemoval> RemovedEffects;
    TArray<FRoundCoreHealingEvent> HealingEvents;
    TArray<FRoundCoreDamageEvent> DamageEvents;
    TArray<FRoundCoreBoardDamageEvent> BoardDamageEvents;
    TArray<FRoundCoreModifierEffectAddition> AddedModifierEffects;
    TArray<TPair<ARoundGridTile*, int32>> TerrainUpdates;
    int32 AppliedDamage = 0;
    int32 AppliedHealing = 0;
    int32 TargetInitiativeLoss = 0;
    bool bTargetDied = false;
};

struct FRoundCoreMoveExecution
{
    TArray<FRoundCoreCharacterRuntimeUpdate> CharacterUpdates;
    TArray<FRoundCoreEffectRemoval> RemovedEffects;
    TArray<FRoundCoreDamageEvent> DamageEvents;
    TArray<FRoundCoreModifierEffectAddition> AddedModifierEffects;
    int32 MovementCost = 0;
    int32 Initiative = 0;
};

// Converts the live Unreal board to the standard-C++ state used by gameplay,
// training and deterministic tests. This adapter contains no gameplay rules.
class ROUND_API FRoundCoreMatchAdapter
{
public:
    bool Rebuild(const ARoundGameState& GameState, FString& OutReason);
    bool IsMoveLegal(const ARoundCharacter* Character, const ARoundGridTile* TargetTile, FString& OutReason) const;
    bool IsBasicAttackLegal(const ARoundCharacter* Character, const ARoundCharacter* TargetCharacter, FString& OutReason) const;
    bool IsAbilityLegal(const ARoundCharacter* Character, const ARoundAbilityActor* Ability,
        const ARoundCharacter* TargetCharacter, const ARoundGridTile* TargetTile, FString& OutReason) const;
    bool PreviewBasicAttack(const ARoundCharacter* Character, const ARoundCharacter* TargetCharacter,
        FRoundCoreBasicAttackPreview& OutPreview, FString& OutReason) const;
    bool ResolveBasicAttackCritical(const ARoundCharacter* Character, bool& bOutCritical, FString& OutReason);
    bool ExecuteBasicAttack(
        const ARoundCharacter* Character,
        const ARoundCharacter* TargetCharacter,
        FRoundCoreBasicAttackExecution& OutExecution,
        FString& OutReason);
    bool ExecuteAbility(
        const ARoundCharacter* Character,
        const ARoundAbilityActor* Ability,
        const ARoundCharacter* TargetCharacter,
        const ARoundGridTile* TargetTile,
        FRoundCoreAbilityExecution& OutExecution,
        FString& OutReason);
    bool GetVisibleTiles(
        const ARoundCharacter* Character,
        TArray<ARoundGridTile*>& OutVisibleTiles,
        FString& OutReason) const;
    bool ExecuteMove(
        const ARoundCharacter* Character,
        const ARoundGridTile* TargetTile,
        FRoundCoreMoveExecution& OutExecution,
        FString& OutReason);
    bool ExecuteTurnStep(
        const ARoundCharacter* EndingCharacter,
        FRoundCoreTurnExecution& OutExecution,
        FString& OutReason);

    const roundcore::Game& GetGame() const { return CoreGame; }

private:
    roundcore::Game CoreGame;
    TMap<const ARoundGridTile*, roundcore::EntityId> TileIds;
    TMap<roundcore::EntityId, ARoundGridTile*> TileActors;
    TMap<const ARoundCharacter*, roundcore::EntityId> CharacterIds;
    TMap<roundcore::EntityId, ARoundCharacter*> CharacterActors;
    TMap<FString, ARoundAbilityActor*> AbilityActors;
    TMap<const ARoundCharacter*, TMap<const ARoundCharacter*, uint64>> PreservedAuraPreventions;
    int32 LastMatchSeed = 0;
    std::uint64_t PreservedRandomState = 0;
};
