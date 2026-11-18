#pragma once

#include "round/core/State.h"

#include <cstdint>
#include <string>
#include <vector>

namespace roundcore
{
struct GameAction
{
    ActionType type = ActionType::EndTurn;
    EntityId actorId = InvalidEntityId;
    EntityId targetEntityId = InvalidEntityId;
    EntityId targetTileId = InvalidEntityId;
    DefinitionId abilityId;
    std::vector<std::int32_t> selections;
};

struct GameEvent
{
    EventType type = EventType::MatchReset;
    EntityId sourceId = InvalidEntityId;
    EntityId targetId = InvalidEntityId;
    EntityId tileId = InvalidEntityId;
    DefinitionId definitionId;
    std::int64_t amount = 0;
};

struct ActionResult
{
    bool success = false;
    std::string errorCode;
    std::string message;
    std::vector<GameEvent> events;
};
}
