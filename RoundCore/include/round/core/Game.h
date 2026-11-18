#pragma once

#include "round/core/Actions.h"

#include <cstdint>
#include <vector>

namespace roundcore
{
struct MatchSetup
{
    ContentDefinitions definitions;
    GameState initialState;
};

class Game
{
public:
    ActionResult Reset(MatchSetup setup);
    ActionResult ApplyAction(const GameAction& action);
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

    ContentDefinitions definitions_;
    GameState state_;
};
}
