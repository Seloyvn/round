#pragma once

#include "round/core/Actions.h"

#include <cstdint>

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

    const ContentDefinitions& Definitions() const noexcept;
    const GameState& State() const noexcept;
    bool IsTerminal() const noexcept;
    std::uint64_t StateHash() const noexcept;

private:
    ContentDefinitions definitions_;
    GameState state_;
};
}
