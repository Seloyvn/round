#pragma once

#include "round/core/Actions.h"
#include "round/core/Observation.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#ifndef ROUNDCORE_API
#define ROUNDCORE_API
#endif

namespace roundcore
{
struct MatchSetup
{
    ContentDefinitions definitions;
    GameState initialState;
};

struct DecisionSnapshot
{
    GameState state;
    std::vector<GameAction> legalActions;
};

class ROUNDCORE_API Game
{
public:
    ActionResult Reset(MatchSetup setup);
    ActionResult ValidateAction(const GameAction& action) const;
    ActionResult ApplyAction(const GameAction& action);
    ActionResult ApplyDecision(const GameAction& action, std::uint32_t maximumInitiativeTicks = 10000);
    ActionResult ApplyDecisionIndex(std::size_t legalActionIndex, std::uint32_t maximumInitiativeTicks = 10000);
    std::vector<GameAction> LegalActions(EntityId actorId) const;
    std::vector<EntityId> VisibleTileIds(EntityId characterId) const;
    std::vector<EntityId> TeamVisibleTileIds(Team team) const;
    DecisionSnapshot ObserveDecision() const;
    DecisionSnapshot ObserveTeamDecision(Team perspective) const;
    NumericObservation ObserveNumeric(const ObservationSchema& schema) const;
    NumericObservation ObserveTeamNumeric(Team perspective, const ObservationSchema& schema) const;
    Game Clone() const;

    const ContentDefinitions& Definitions() const noexcept;
    const GameState& State() const noexcept;
    bool IsTerminal() const noexcept;
    std::uint64_t StateHash() const noexcept;
    bool RollBasisPoints(std::int32_t chanceBasisPoints) noexcept;

private:
    const TileState* FindTile(EntityId tileId) const noexcept;
    TileState* FindTile(EntityId tileId) noexcept;
    const CharacterState* FindCharacter(EntityId characterId) const noexcept;
    CharacterState* FindCharacter(EntityId characterId) noexcept;
    std::int32_t MovementCost(const CharacterState& character, const TileState& destination) const noexcept;
    std::int32_t BasicAttackCost(const CharacterState& character) const noexcept;
    std::int32_t ResolveDamage(CharacterState& source, CharacterState& target, std::int64_t damage,
        std::int32_t defenseIgnore, std::vector<GameEvent>& auraEvents,
        std::vector<GameEvent>& modifierEvents);
    ActionResult ValidateMove(const GameAction& action) const;
    ActionResult ApplyBasicAttack(const GameAction& action);
    ActionResult ValidateBasicAttack(const GameAction& action) const;
    ActionResult ApplyAbility(const GameAction& action);
    ActionResult ValidateAbility(const GameAction& action) const;
    ActionResult ApplyEndTurn(const GameAction& action);
    ActionResult ValidateEndTurn(const GameAction& action) const;
    std::uint64_t NextRandom() noexcept;

    ContentDefinitions definitions_;
    GameState state_;
};
}
