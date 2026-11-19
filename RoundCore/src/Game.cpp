#include "round/core/Game.h"

#include <algorithm>
#include <iterator>
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
    if (setup.initialState.randomState == 0)
    {
        setup.initialState.randomState = setup.initialState.seed != 0
            ? setup.initialState.seed
            : 0x9e3779b97f4a7c15ull;
    }
    definitions_ = std::move(setup.definitions);
    state_ = std::move(setup.initialState);

    ActionResult result;
    result.success = true;
    result.events.push_back(GameEvent{EventType::MatchReset});
    return result;
}

ActionResult Game::ApplyAction(const GameAction& action)
{
    const ActionResult validation = ValidateAction(action);
    if (!validation.success)
    {
        return validation;
    }

    if (action.type == ActionType::EndTurn)
    {
        return ApplyEndTurn(action);
    }
    if (action.type == ActionType::BasicAttack)
    {
        return ApplyBasicAttack(action);
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

ActionResult Game::ValidateAction(const GameAction& action) const
{
    if (action.type == ActionType::Move)
    {
        return ValidateMove(action);
    }
    if (action.type == ActionType::EndTurn)
    {
        return ValidateEndTurn(action);
    }
    if (action.type == ActionType::BasicAttack)
    {
        return ValidateBasicAttack(action);
    }
    return Failure("unsupported_action", "This action has not been migrated to RoundCore yet.");
}

ActionResult Game::ApplyDecision(const GameAction& action, const std::uint32_t maximumInitiativeTicks)
{
    ActionResult combined = ApplyAction(action);
    if (!combined.success || action.type != ActionType::EndTurn)
    {
        return combined;
    }

    std::uint32_t ticks = 0;
    while (!IsTerminal() && state_.activeCharacterId == InvalidEntityId)
    {
        if (ticks++ >= maximumInitiativeTicks)
        {
            combined.success = false;
            combined.errorCode = "initiative_safety_limit";
            combined.message = "No character became active before the initiative safety limit.";
            return combined;
        }

        GameAction continuation;
        continuation.type = ActionType::EndTurn;
        ActionResult step = ApplyAction(continuation);
        if (!step.success)
        {
            return step;
        }
        combined.events.insert(
            combined.events.end(),
            std::make_move_iterator(step.events.begin()),
            std::make_move_iterator(step.events.end()));
        combined.message = std::move(step.message);
    }
    return combined;
}

ActionResult Game::ApplyDecisionIndex(const std::size_t legalActionIndex,
    const std::uint32_t maximumInitiativeTicks)
{
    const std::vector<GameAction> actions = LegalActions(state_.activeCharacterId);
    if (legalActionIndex >= actions.size())
    {
        return Failure("legal_action_index_out_of_range", "The selected legal action index does not exist.");
    }
    return ApplyDecision(actions[legalActionIndex], maximumInitiativeTicks);
}

DecisionSnapshot Game::ObserveDecision() const
{
    DecisionSnapshot snapshot;
    snapshot.state = state_;
    if (!IsTerminal() && state_.activeCharacterId != InvalidEntityId)
    {
        snapshot.legalActions = LegalActions(state_.activeCharacterId);
    }
    return snapshot;
}

ActionResult Game::ApplyBasicAttack(const GameAction& action)
{
    CharacterState* attacker = FindCharacter(action.actorId);
    CharacterState* target = FindCharacter(action.targetEntityId);
    if (!attacker || !target)
    {
        return Failure("invalid_state", "Validated attack references missing character state.");
    }

    const CharacterDefinition& attackerDefinition = definitions_.characters.at(attacker->definitionId);
    const CharacterDefinition& targetDefinition = definitions_.characters.at(target->definitionId);
    const std::int32_t attackCost = std::max(0, attackerDefinition.stats.attackCost);
    attacker->initiative -= attackCost;

    std::int64_t damage = std::max(0, attackerDefinition.stats.attack);
    const std::int32_t criticalChance = std::clamp(attackerDefinition.criticalChanceBasisPoints, 0, 10000);
    const bool critical = criticalChance > 0
        && static_cast<std::int32_t>(NextRandom() % 10000ull) < criticalChance;
    if (critical)
    {
        const std::int64_t multiplier = std::max(0, attackerDefinition.criticalDamageMultiplierBasisPoints);
        damage = (damage * multiplier + 5000) / 10000;
    }

    const std::int64_t defense = std::max(0, targetDefinition.stats.defense);
    const std::int64_t denominator = 100 + defense;
    const std::int32_t finalDamage = static_cast<std::int32_t>(std::min<std::int64_t>(
        std::numeric_limits<std::int32_t>::max(),
        (damage * 100 + denominator / 2) / denominator));
    const std::int32_t appliedDamage = std::min(target->health, finalDamage);
    target->health -= appliedDamage;

    ActionResult result;
    result.success = true;
    result.message = critical ? "Critical basic attack executed." : "Basic attack executed.";
    result.events.push_back(GameEvent{
        EventType::InitiativeChanged, attacker->id, attacker->id, attacker->tileId, {}, -attackCost});
    result.events.push_back(GameEvent{
        EventType::DamageApplied, attacker->id, target->id, target->tileId, "Physical", appliedDamage});

    if (target->health <= 0)
    {
        target->health = 0;
        target->alive = false;
        if (TileState* targetTile = FindTile(target->tileId))
        {
            targetTile->occupantId = InvalidEntityId;
        }
        result.events.push_back(GameEvent{
            EventType::CharacterDied, attacker->id, target->id, target->tileId, {}, 0});

        Team soleTeam = Team::Neutral;
        bool multipleTeams = false;
        for (const CharacterState& character : state_.characters)
        {
            if (!character.alive || character.team == Team::Neutral)
            {
                continue;
            }
            if (soleTeam == Team::Neutral)
            {
                soleTeam = character.team;
            }
            else if (soleTeam != character.team)
            {
                multipleTeams = true;
                break;
            }
        }
        if (!multipleTeams && soleTeam != Team::Neutral)
        {
            state_.status = MatchStatus::Completed;
            state_.winner = soleTeam;
            state_.activeCharacterId = InvalidEntityId;
            result.events.push_back(GameEvent{
                EventType::MatchEnded, attacker->id, InvalidEntityId, InvalidEntityId, {}, static_cast<std::int64_t>(soleTeam)});
        }
    }

    ++state_.step;
    return result;
}

ActionResult Game::ValidateBasicAttack(const GameAction& action) const
{
    if (state_.status != MatchStatus::Running)
    {
        return Failure("match_not_running", "Basic attacks require a running match.");
    }
    const CharacterState* attacker = FindCharacter(action.actorId);
    const CharacterState* target = FindCharacter(action.targetEntityId);
    if (!attacker || !attacker->alive || !target || !target->alive)
    {
        return Failure("invalid_attack_characters", "Attacker and target must exist and be alive.");
    }
    if (state_.activeCharacterId != attacker->id)
    {
        return Failure("not_active_character", "Only the active character can attack.");
    }
    if (attacker->team == Team::Neutral || attacker->team == target->team)
    {
        return Failure("target_not_enemy", "A basic attack requires an enemy character.");
    }
    const TileState* attackerTile = FindTile(attacker->tileId);
    const TileState* targetTile = FindTile(target->tileId);
    if (!attackerTile || !targetTile)
    {
        return Failure("missing_attack_tile", "Attacker and target must occupy board tiles.");
    }
    const CharacterDefinition& definition = definitions_.characters.at(attacker->definitionId);
    if (HexDistance(attackerTile->coord, targetTile->coord) > std::max(0, definition.stats.attackRange))
    {
        return Failure("target_out_of_range", "The target is outside basic attack range.");
    }
    if (attacker->initiative < std::max(0, definition.stats.attackCost))
    {
        return Failure("insufficient_initiative", "The character cannot afford this basic attack.");
    }
    return ActionResult{true};
}

std::uint64_t Game::NextRandom() noexcept
{
    std::uint64_t value = state_.randomState;
    value ^= value >> 12;
    value ^= value << 25;
    value ^= value >> 27;
    state_.randomState = value;
    return value * 2685821657736338717ull;
}

ActionResult Game::ApplyEndTurn(const GameAction&)
{
    state_.activeCharacterId = InvalidEntityId;

    CharacterState* selected = nullptr;
    std::int32_t highestInitiative = std::numeric_limits<std::int32_t>::min();
    for (CharacterState& character : state_.characters)
    {
        if (!character.alive || !character.canTakeTurn)
        {
            continue;
        }
        const auto definition = definitions_.characters.find(character.definitionId);
        if (definition == definitions_.characters.end())
        {
            continue;
        }

        const std::int32_t effectiveInitiative = character.initiative - character.delayedActionInitiative;
        if (effectiveInitiative < definition->second.stats.maxInitiative)
        {
            continue;
        }
        if (!selected || effectiveInitiative > highestInitiative
            || (effectiveInitiative == highestInitiative && character.id < selected->id))
        {
            selected = &character;
            highestInitiative = effectiveInitiative;
        }
    }

    ActionResult result;
    result.success = true;
    if (selected)
    {
        const CharacterDefinition& definition = definitions_.characters.at(selected->definitionId);
        const std::int32_t delay = std::max(0, definition.stats.turnDelay);
        if (delay > 0)
        {
            selected->initiative = std::max(0, selected->initiative - delay);
            result.events.push_back(GameEvent{
                EventType::InitiativeChanged, selected->id, selected->id, selected->tileId, {}, -delay});
        }
        state_.activeCharacterId = selected->id;
        result.events.push_back(GameEvent{
            EventType::ActiveCharacterChanged, selected->id, selected->id, selected->tileId, {}, 0});
        result.message = "Next active character selected.";
    }
    else
    {
        for (CharacterState& character : state_.characters)
        {
            if (!character.alive)
            {
                continue;
            }
            const std::int32_t gain = std::max(0, character.initiativeGainPerTick);
            character.initiative += gain;
            if (gain > 0)
            {
                result.events.push_back(GameEvent{
                    EventType::InitiativeChanged, character.id, character.id, character.tileId, {}, gain});
            }


            for (AbilityState& ability : character.abilities)
            {
                const std::int32_t previousCooldown = ability.cooldownRemaining;
                const std::int32_t previousCharges = ability.currentCharges;
                ability.cooldownRemaining = std::max(0, ability.cooldownRemaining - 1);
                if (ability.usesCharges && ability.cooldownRemaining <= 0 && ability.currentCharges < ability.maxCharges)
                {
                    ability.currentCharges = std::min(ability.maxCharges, ability.currentCharges + 1);
                    if (ability.currentCharges < ability.maxCharges)
                    {
                        ability.cooldownRemaining = std::max(0, ability.cooldownDuration);
                    }
                }
                if (ability.cooldownRemaining != previousCooldown)
                {
                    result.events.push_back(GameEvent{
                        EventType::CooldownChanged,
                        character.id,
                        character.id,
                        character.tileId,
                        ability.definitionId,
                        ability.cooldownRemaining - previousCooldown});
                }
                if (ability.currentCharges != previousCharges)
                {
                    result.events.push_back(GameEvent{
                        EventType::AbilityChargesChanged,
                        character.id,
                        character.id,
                        character.tileId,
                        ability.definitionId,
                        ability.currentCharges - previousCharges});
                }
            }
        }
        ++state_.globalInitiative;
        result.events.push_back(GameEvent{
            EventType::GlobalInitiativeAdvanced, InvalidEntityId, InvalidEntityId, InvalidEntityId, {}, 1});
        result.message = "Global initiative advanced.";
    }
    ++state_.step;
    return result;
}

ActionResult Game::ValidateEndTurn(const GameAction& action) const
{
    if (state_.status != MatchStatus::Running)
    {
        return Failure("match_not_running", "Turn advancement requires a running match.");
    }
    if (state_.activeCharacterId == InvalidEntityId)
    {
        if (action.actorId != InvalidEntityId)
        {
            return Failure("unexpected_actor", "Turn continuation without an active character must not specify an actor.");
        }
        return ActionResult{true};
    }

    const CharacterState* character = FindCharacter(action.actorId);
    if (!character || !character->alive)
    {
        return Failure("invalid_actor", "The ending character must exist and be alive.");
    }
    if (state_.activeCharacterId != character->id)
    {
        return Failure("not_active_character", "Only the active character can end its turn.");
    }
    return ActionResult{true};
}

std::vector<GameAction> Game::LegalActions(const EntityId actorId) const
{
    std::vector<GameAction> actions;
    if (actorId == InvalidEntityId)
    {
        GameAction continuation;
        continuation.type = ActionType::EndTurn;
        if (ValidateEndTurn(continuation).success)
        {
            actions.push_back(continuation);
        }
        return actions;
    }

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

    for (const CharacterState& target : state_.characters)
    {
        GameAction action;
        action.type = ActionType::BasicAttack;
        action.actorId = actorId;
        action.targetEntityId = target.id;
        if (ValidateBasicAttack(action).success)
        {
            actions.push_back(std::move(action));
        }
    }

    GameAction endTurn;
    endTurn.type = ActionType::EndTurn;
    endTurn.actorId = actorId;
    if (ValidateEndTurn(endTurn).success)
    {
        actions.push_back(endTurn);
    }

    std::sort(actions.begin(), actions.end(), [](const GameAction& left, const GameAction& right)
    {
        if (left.type != right.type)
        {
            return left.type < right.type;
        }
        if (left.targetEntityId != right.targetEntityId)
        {
            return left.targetEntityId < right.targetEntityId;
        }
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
    HashValue(hash, state_.randomState);
    HashValue(hash, state_.step);
    HashValue(hash, state_.status);
    HashValue(hash, state_.winner);
    HashValue(hash, state_.activeCharacterId);
    HashValue(hash, state_.globalInitiative);

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
        HashValue(hash, character.canTakeTurn);
        HashValue(hash, character.initiativeGainPerTick);
        HashValue(hash, character.delayedActionInitiative);
        for (const AbilityState& ability : character.abilities)
        {
            HashString(hash, ability.definitionId);
            HashValue(hash, ability.cooldownRemaining);
            HashValue(hash, ability.cooldownDuration);
            HashValue(hash, ability.currentCharges);
            HashValue(hash, ability.maxCharges);
            HashValue(hash, ability.usesCharges);
        }
    }
    return hash;
}
}
