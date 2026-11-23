#pragma once

#include "round/core/Definitions.h"

#include <cstdint>
#include <string>
#include <vector>

namespace roundcore
{
struct TileState
{
    EntityId id = InvalidEntityId;
    HexCoord coord;
    DefinitionId terrainId;
    EntityId occupantId = InvalidEntityId;
    bool blocksMovement = false;
    std::int32_t fieldObjectHealth = 0;
    std::int32_t terrainObjectHealth = 0;
};

struct AbilityState
{
    DefinitionId definitionId;
    std::int32_t cooldownRemaining = 0;
    std::int32_t cooldownDuration = 0;
    std::int32_t currentCharges = 1;
    std::int32_t maxCharges = 1;
    bool usesCharges = false;
    std::int32_t readyInitiativeElapsed = 0;
};

struct ModifierEffectState
{
    DefinitionId id;
    DefinitionId sourceAbilityId;
    ModifierEffectKind kind = ModifierEffectKind::Generic;
    EntityId causerId = InvalidEntityId;
    EntityId preferredTargetId = InvalidEntityId;
    std::int32_t remainingInitiative = 0;
    std::int32_t remainingTurns = 0;
    std::int32_t incomingDamageMultiplierBasisPoints = 10000;
    std::int32_t outgoingDamageMultiplierBasisPoints = 10000;
    std::int32_t defenseIgnoreAdd = 0;
    std::int32_t healFractionFromDamageDealtBasisPoints = 0;
    std::int32_t moveCostAdd = 0;
    std::int32_t moveCostMultiplierBasisPoints = 10000;
    std::int32_t initiativeLossOnMove = 0;
    std::int32_t firstMoveInitiativeLoss = 0;
    std::int32_t maximumMovedFields = 0;
    std::int32_t movedFields = 0;
    std::int32_t frostbreakDamage = 0;
    std::int32_t basicAttackCostAdd = 0;
    std::int32_t basicAttackDamageMultiplierBasisPoints = 10000;
    std::int32_t incomingDamageMultiplierFromCauserBasisPoints = 10000;
    std::int32_t initiativeLossOnExpire = 0;
    std::int32_t incomingEffectDurationAdd = 0;
    std::int32_t outgoingDamageMultiplierAgainstOtherTargetsBasisPoints = 10000;
    std::int32_t initiativeLossWhenIgnoringPreferredTarget = 0;
    std::int32_t initiativeGainForCauserWhenAttacked = 0;
    std::int32_t triggerDamageOnMove = 0;
    std::int32_t triggerDelayOnMove = 0;
    std::int32_t triggerOutgoingDamageMultiplierBasisPoints = 10000;
    std::int32_t triggerSlowMultiplierBasisPoints = 10000;
    std::int32_t triggerSlowDurationInitiative = 0;
    std::int32_t triggerAreaRadius = 0;
    std::int32_t triggerAreaInitiativeLoss = 0;
    std::int32_t triggerInitiativeGainForCauser = 0;
    std::int32_t redirectPreMitigationBasisPoints = 0;
    std::int32_t redirectedDamageMultiplierBasisPoints = 10000;
    std::int32_t protectorHealFractionBasisPoints = 0;
    std::int32_t protectedInitiativePerHit = 0;
    EntityId areaCenterTileId = InvalidEntityId;
    std::int32_t areaRadius = 0;
    std::int32_t areaDamage = 0;
    DamageType areaDamageType = DamageType::Physical;
    std::int32_t areaInitiativeLossPerPulse = 0;
    bool areaFollowsCauser = false;
    bool onlyAffectCauserDamage = false;
    bool onlyAgainstRangedSources = false;
    bool blockMovementBonuses = false;
    bool basicAttackCostAvailable = false;
    bool consumeOnBasicAttack = false;
    bool consumeOnOutgoingDamage = false;
    bool preventsAbilityUse = false;
    bool preventsMovement = false;
    bool firstMovePenaltyAvailable = false;
    bool expireOnTurnEnd = false;
    bool isDebuff = false;
    bool triggerOnMove = false;
    bool consumeOnDamageFromCauser = false;
    std::int32_t counterA = 0;
    std::int32_t counterB = 0;
    std::int32_t counterC = 0;
    std::int32_t valueA = 0;
    std::int32_t valueB = 0;
    std::int32_t valueC = 0;
    std::uint32_t flags = 0;
};

struct RecentAuraPreventionState
{
    EntityId sourceId = InvalidEntityId;
    std::uint64_t globalInitiative = 0;
};

struct CharacterState
{
    EntityId id = InvalidEntityId;
    DefinitionId definitionId;
    Team team = Team::Neutral;
    EntityId tileId = InvalidEntityId;
    std::int32_t health = 0;
    std::int32_t initiative = 0;
    bool alive = true;
    bool movementBlocked = false;
    bool canTakeTurn = true;
    std::int32_t initiativeGainPerTick = 1;
    std::int32_t delayedActionInitiative = 0;
    std::int32_t resourceCurrent = 0;
    std::int32_t resourceMaximum = 0;
    std::int32_t resourceRechargeProgress = 0;
    std::int32_t resourceRechargeInterval = 0;
    std::int32_t specialCooldownRemaining = 0;
    bool firstResourceSpendRefundAvailable = true;
    bool movedSinceLastTurn = false;
    bool stationaryGuardActive = false;
    bool attackedThisTurn = false;
    bool spentResourceThisTurn = false;
    bool startedTurnWithFullResource = false;
    bool heldPositionFromPreviousTurn = false;
    bool usedFreeBasicAttackThisTurn = false;
    bool triggeredEmptyResourceThisTurn = false;
    bool refundedLastResourceKillThisTurn = false;
    bool usedRangeRefundThisTurn = false;
    bool usedKillCooldownReductionThisTurn = false;
    std::int32_t stationaryShotStacks = 0;
    bool hasFreePreparedStep = false;
    std::int32_t movedFieldsThisTurn = 0;
    std::int32_t resourceSpentThisTurnCount = 0;
    std::int32_t secondaryCooldownRemaining = 0;
    std::int32_t turnSequence = 0;
    bool openingRewardUsedThisTurn = false;
    bool overflowRewardUsedThisTurn = false;
    bool freeAdjacentMoveAvailable = false;
    bool freeAdjacentMoveUsedThisTurn = false;
    bool nextDamageAfterMoveReady = false;
    bool nextReductionAfterMoveReady = false;
    bool startInvisibilityApplied = false;
    std::vector<AbilityState> abilities;
    std::vector<ModifierEffectState> modifierEffects;
    std::vector<RecentAuraPreventionState> recentAuraPreventions;
};

struct GameState
{
    std::uint32_t schemaVersion = 1;
    MatchId matchId = 0;
    std::uint64_t seed = 0;
    std::uint64_t randomState = 0;
    std::uint64_t step = 0;
    MatchStatus status = MatchStatus::NotStarted;
    Team winner = Team::Neutral;
    EntityId activeCharacterId = InvalidEntityId;
    std::uint64_t globalInitiative = 0;
    std::vector<TileState> tiles;
    std::vector<CharacterState> characters;
};
}
