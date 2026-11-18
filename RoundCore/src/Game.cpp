#include "round/core/Game.h"

#include <algorithm>
#include <limits>
#include <unordered_set>
#include <utility>

namespace roundcore
{
namespace
{
ActionResult Failure(std::string code, std::string message)
{
    ActionResult result;
    result.errorCode = std::move(code);
    result.message = std::move(message);
    return result;
}

void HashBytes(std::uint64_t& hash, const void* data, const std::size_t size) noexcept
{
    constexpr std::uint64_t prime = 1099511628211ull;
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t index = 0; index < size; ++index)
    {
        hash ^= bytes[index];
        hash *= prime;
    }
}

template <typename Value>
void HashValue(std::uint64_t& hash, const Value& value) noexcept
{
    HashBytes(hash, &value, sizeof(Value));
}

void HashString(std::uint64_t& hash, const std::string& value) noexcept
{
    HashBytes(hash, value.data(), value.size());
    const std::uint8_t terminator = 0xff;
    HashValue(hash, terminator);
}
}

ActionResult Game::Reset(MatchSetup setup)
{
    if (setup.definitions.schemaVersion == 0 || setup.initialState.schemaVersion == 0)
    {
        return Failure("invalid_schema", "Definition and state schema versions must be non-zero.");
    }

    std::unordered_set<EntityId> tileIds;
    std::unordered_set<EntityId> characterIds;
    std::unordered_set<EntityId> occupiedTileIds;

    for (const TileState& tile : setup.initialState.tiles)
    {
        if (tile.id == InvalidEntityId || !tileIds.insert(tile.id).second)
        {
            return Failure("invalid_tile_id", "Tile IDs must be unique and non-zero.");
        }
        if (setup.definitions.terrains.find(tile.terrainId) == setup.definitions.terrains.end())
        {
            return Failure("missing_terrain_definition", "Every tile must reference an exported terrain definition.");
        }
    }

    for (const CharacterState& character : setup.initialState.characters)
    {
        if (character.id == InvalidEntityId || !characterIds.insert(character.id).second)
        {
            return Failure("invalid_character_id", "Character IDs must be unique and non-zero.");
        }
        if (setup.definitions.characters.find(character.definitionId) == setup.definitions.characters.end())
        {
            return Failure("missing_character_definition", "Every character must reference an exported character definition.");
        }
        if (tileIds.find(character.tileId) == tileIds.end())
        {
            return Failure("missing_character_tile", "Every character must reference a tile in the match board.");
        }
        if (!occupiedTileIds.insert(character.tileId).second)
        {
            return Failure("duplicate_occupancy", "Two characters cannot start on the same tile.");
        }
    }

    for (TileState& tile : setup.initialState.tiles)
    {
        tile.occupantId = InvalidEntityId;
    }
    for (const CharacterState& character : setup.initialState.characters)
    {
        const auto tile = std::find_if(setup.initialState.tiles.begin(), setup.initialState.tiles.end(),
            [&character](const TileState& candidate) { return candidate.id == character.tileId; });
        tile->occupantId = character.id;
    }

    if (setup.initialState.activeCharacterId != InvalidEntityId
        && characterIds.find(setup.initialState.activeCharacterId) == characterIds.end())
    {
        return Failure("invalid_active_character", "The active character must exist in the match state.");
    }

    setup.initialState.status = MatchStatus::Running;
    setup.initialState.winner = Team::Neutral;
    setup.initialState.step = 0;
    definitions_ = std::move(setup.definitions);
    state_ = std::move(setup.initialState);

    ActionResult result;
    result.success = true;
    result.events.push_back(GameEvent{EventType::MatchReset});
    return result;
}

ActionResult Game::ApplyAction(const GameAction& action)
{
    if (action.type != ActionType::Move)
    {
        return Failure("unsupported_action", "This action has not been migrated to RoundCore yet.");
    }

    const ActionResult validation = ValidateMove(action);
    if (!validation.success)
    {
        return validation;
    }

    CharacterState* character = FindCharacter(action.actorId);
    TileState* origin = character ? FindTile(character->tileId) : nullptr;
    TileState* destination = FindTile(action.targetTileId);
    if (!character || !origin || !destination)
    {
        return Failure("invalid_state", "Validated movement references missing state.");
    }

    const std::int32_t cost = MovementCost(*character, *destination);
    origin->occupantId = InvalidEntityId;
    destination->occupantId = character->id;
    character->tileId = destination->id;
    character->initiative -= cost;
    ++state_.step;

    ActionResult result;
    result.success = true;
    result.message = "Movement executed.";
    result.events.push_back(GameEvent{EventType::CharacterMoved, character->id, InvalidEntityId, destination->id, {}, cost});
    result.events.push_back(GameEvent{EventType::InitiativeChanged, character->id, character->id, destination->id, {}, -cost});
    return result;
}

std::vector<GameAction> Game::LegalActions(const EntityId actorId) const
{
    std::vector<GameAction> actions;
    const CharacterState* character = FindCharacter(actorId);
    if (!character)
    {
        return actions;
    }

    const TileState* origin = FindTile(character->tileId);
    if (!origin)
    {
        return actions;
    }

    for (const TileState& tile : state_.tiles)
    {
        if (HexDistance(origin->coord, tile.coord) != 1)
        {
            continue;
        }

        GameAction action;
        action.type = ActionType::Move;
        action.actorId = actorId;
        action.targetTileId = tile.id;
        if (ValidateMove(action).success)
        {
            actions.push_back(std::move(action));
        }
    }

    std::sort(actions.begin(), actions.end(), [](const GameAction& left, const GameAction& right)
    {
        return left.targetTileId < right.targetTileId;
    });
    return actions;
}

const TileState* Game::FindTile(const EntityId tileId) const noexcept
{
    const auto tile = std::find_if(state_.tiles.begin(), state_.tiles.end(),
        [tileId](const TileState& candidate) { return candidate.id == tileId; });
    return tile == state_.tiles.end() ? nullptr : &*tile;
}

TileState* Game::FindTile(const EntityId tileId) noexcept
{
    return const_cast<TileState*>(static_cast<const Game*>(this)->FindTile(tileId));
}

const CharacterState* Game::FindCharacter(const EntityId characterId) const noexcept
{
    const auto character = std::find_if(state_.characters.begin(), state_.characters.end(),
        [characterId](const CharacterState& candidate) { return candidate.id == characterId; });
    return character == state_.characters.end() ? nullptr : &*character;
}

CharacterState* Game::FindCharacter(const EntityId characterId) noexcept
{
    return const_cast<CharacterState*>(static_cast<const Game*>(this)->FindCharacter(characterId));
}

std::int32_t Game::MovementCost(const CharacterState& character, const TileState& destination) const noexcept
{
    const auto characterDefinition = definitions_.characters.find(character.definitionId);
    const auto terrainDefinition = definitions_.terrains.find(destination.terrainId);
    if (characterDefinition == definitions_.characters.end() || terrainDefinition == definitions_.terrains.end())
    {
        return std::numeric_limits<std::int32_t>::max();
    }

    const std::int64_t baseCost = std::max<std::int32_t>(1, characterDefinition->second.stats.moveCost);
    const std::int64_t multiplier = std::max<std::int32_t>(0, terrainDefinition->second.movementCostMultiplierBasisPoints);
    const std::int64_t scaledCost = (baseCost * multiplier + 5000) / 10000;
    const std::int64_t cost = std::max<std::int64_t>(terrainDefinition->second.movementCost, scaledCost);
    return cost > std::numeric_limits<std::int32_t>::max()
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(std::max<std::int64_t>(1, cost));
}

ActionResult Game::ValidateMove(const GameAction& action) const
{
    if (state_.status != MatchStatus::Running)
    {
        return Failure("match_not_running", "Movement requires a running match.");
    }
    if (action.type != ActionType::Move)
    {
        return Failure("wrong_action_type", "Movement validation requires a move action.");
    }

    const CharacterState* character = FindCharacter(action.actorId);
    if (!character || !character->alive)
    {
        return Failure("invalid_actor", "The moving character must exist and be alive.");
    }
    if (state_.activeCharacterId != character->id)
    {
        return Failure("not_active_character", "Only the active character can move.");
    }
    if (character->movementBlocked)
    {
        return Failure("movement_blocked", "The active character cannot move.");
    }

    const TileState* origin = FindTile(character->tileId);
    const TileState* destination = FindTile(action.targetTileId);
    if (!origin || !destination || origin->id == destination->id)
    {
        return Failure("invalid_destination", "Movement requires a different existing destination tile.");
    }
    if (HexDistance(origin->coord, destination->coord) != 1)
    {
        return Failure("destination_not_adjacent", "A move action only allows an adjacent destination.");
    }

    const auto terrain = definitions_.terrains.find(destination->terrainId);
    if (terrain == definitions_.terrains.end() || !terrain->second.walkable || destination->blocksMovement)
    {
        return Failure("destination_blocked", "The destination does not allow character movement.");
    }
    if (destination->occupantId != InvalidEntityId)
    {
        return Failure("destination_occupied", "The destination is occupied by another character.");
    }

    const std::int32_t cost = MovementCost(*character, *destination);
    if (cost == std::numeric_limits<std::int32_t>::max() || character->initiative < cost)
    {
        return Failure("insufficient_initiative", "The character cannot afford this movement.");
    }

    ActionResult result;
    result.success = true;
    return result;
}

const ContentDefinitions& Game::Definitions() const noexcept
{
    return definitions_;
}

const GameState& Game::State() const noexcept
{
    return state_;
}

bool Game::IsTerminal() const noexcept
{
    return state_.status == MatchStatus::Completed;
}

std::uint64_t Game::StateHash() const noexcept
{
    std::uint64_t hash = 14695981039346656037ull;
    HashValue(hash, state_.schemaVersion);
    HashValue(hash, state_.matchId);
    HashValue(hash, state_.seed);
    HashValue(hash, state_.step);
    HashValue(hash, state_.status);
    HashValue(hash, state_.winner);
    HashValue(hash, state_.activeCharacterId);

    for (const TileState& tile : state_.tiles)
    {
        HashValue(hash, tile.id);
        HashValue(hash, tile.coord.q);
        HashValue(hash, tile.coord.r);
        HashString(hash, tile.terrainId);
        HashValue(hash, tile.occupantId);
        HashValue(hash, tile.blocksMovement);
    }
    for (const CharacterState& character : state_.characters)
    {
        HashValue(hash, character.id);
        HashString(hash, character.definitionId);
        HashValue(hash, character.team);
        HashValue(hash, character.tileId);
        HashValue(hash, character.health);
        HashValue(hash, character.initiative);
        HashValue(hash, character.alive);
        HashValue(hash, character.movementBlocked);
        for (const AbilityState& ability : character.abilities)
        {
            HashString(hash, ability.definitionId);
            HashValue(hash, ability.cooldownRemaining);
        }
    }
    return hash;
}
}
