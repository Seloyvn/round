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
};

struct AbilityState
{
    DefinitionId definitionId;
    std::int32_t cooldownRemaining = 0;
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
    std::vector<AbilityState> abilities;
};

struct GameState
{
    std::uint32_t schemaVersion = 1;
    MatchId matchId = 0;
    std::uint64_t seed = 0;
    std::uint64_t step = 0;
    MatchStatus status = MatchStatus::NotStarted;
    Team winner = Team::Neutral;
    EntityId activeCharacterId = InvalidEntityId;
    std::vector<TileState> tiles;
    std::vector<CharacterState> characters;
};
}
