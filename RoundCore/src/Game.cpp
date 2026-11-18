#include "round/core/Game.h"

#include <algorithm>
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
        for (const AbilityState& ability : character.abilities)
        {
            HashString(hash, ability.definitionId);
            HashValue(hash, ability.cooldownRemaining);
        }
    }
    return hash;
}
}
