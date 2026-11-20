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
};

struct AbilityState
{
    DefinitionId definitionId;
    std::int32_t cooldownRemaining = 0;
    std::int32_t cooldownDuration = 0;
    std::int32_t currentCharges = 1;
    std::int32_t maxCharges = 1;
    bool usesCharges = false;
};

struct ModifierEffectState
{
    DefinitionId id;
    EntityId causerId = InvalidEntityId;
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
    bool onlyAffectCauserDamage = false;
    bool onlyAgainstRangedSources = false;
    bool blockMovementBonuses = false;
    bool basicAttackCostAvailable = false;
    bool consumeOnBasicAttack = false;
    bool preventsAbilityUse = false;
    bool preventsMovement = false;
    bool firstMovePenaltyAvailable = false;
    bool expireOnTurnEnd = false;
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
    std::vector<AbilityState> abilities;
    std::vector<ModifierEffectState> modifierEffects;
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
