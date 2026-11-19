#pragma once

#include "round/core/Actions.h"

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

class ROUNDCORE_API Game
{
public:
    ActionResult Reset(MatchSetup setup);
    ActionResult ValidateAction(const GameAction& action) const;
    ActionResult ApplyAction(const GameAction& action);
    ActionResult ApplyDecision(const GameAction& action, std::uint32_t maximumInitiativeTicks = 10000);
    std::vector<GameAction> LegalActions(EntityId actorId) const;

    const ContentDefinitions& Definitions() const noexcept;
    const GameState& State() const noexcept;
    bool IsTerminal() const noexcept;
    std::uint64_t StateHash() const noexcept;

private:
    const TileState* FindTile(EntityId tileId) const noexcept;
    TileState* FindTile(EntityId tileId) noexcept;
    const CharacterState* FindCharacter(EntityId characterId) const noexcept;
    CharacterState* FindCharacter(EntityId characterId) noexcept;
    std::int32_t MovementCost(const CharacterState& character, const TileState& destination) const noexcept;
    ActionResult ValidateMove(const GameAction& action) const;
    ActionResult ApplyBasicAttack(const GameAction& action);
    ActionResult ValidateBasicAttack(const GameAction& action) const;
    ActionResult ApplyEndTurn(const GameAction& action);
    ActionResult ValidateEndTurn(const GameAction& action) const;
    std::uint64_t NextRandom() noexcept;

    ContentDefinitions definitions_;
    GameState state_;
};
}
