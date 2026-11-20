#include "round/core/Observation.h"

#include "round/core/Definitions.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace roundcore
{
namespace
{
template <typename Map>
std::vector<DefinitionId> SortedIds(const Map& definitions)
{
    std::vector<DefinitionId> ids;
    ids.reserve(definitions.size());
    for (const auto& entry : definitions)
    {
        ids.push_back(entry.first);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

std::unordered_map<DefinitionId, std::int32_t> BuildIndices(const std::vector<DefinitionId>& ids)
{
    std::unordered_map<DefinitionId, std::int32_t> indices;
    for (std::size_t index = 0; index < ids.size(); ++index)
    {
        indices.emplace(ids[index], static_cast<std::int32_t>(index));
    }
    return indices;
}

std::int32_t RequireIndex(
    const std::unordered_map<DefinitionId, std::int32_t>& indices,
    const DefinitionId& id,
    const char* category)
{
    const auto found = indices.find(id);
    if (found == indices.end())
    {
        throw std::invalid_argument(std::string("Observation schema is missing ") + category + " definition: " + id);
    }
    return found->second;
}
}

ObservationSchema BuildObservationSchema(const ContentDefinitions& definitions)
{
    ObservationSchema schema;
    schema.schemaVersion = definitions.schemaVersion;
    schema.contentVersion = definitions.contentVersion;
    schema.characterDefinitions = SortedIds(definitions.characters);
    schema.terrainDefinitions = SortedIds(definitions.terrains);
    schema.abilityDefinitions = SortedIds(definitions.abilities);
    schema.characterIndices = BuildIndices(schema.characterDefinitions);
    schema.terrainIndices = BuildIndices(schema.terrainDefinitions);
    schema.abilityIndices = BuildIndices(schema.abilityDefinitions);
    return schema;
}

NumericObservation EncodeOmniscientObservation(
    const GameState& state,
    const std::vector<GameAction>& legalActions,
    const ObservationSchema& schema)
{
    NumericObservation result;
    result.schemaVersion = schema.schemaVersion;
    result.contentVersion = schema.contentVersion;
    result.global = {
        static_cast<std::int64_t>(state.matchId),
        static_cast<std::int64_t>(state.step),
        static_cast<std::int64_t>(state.status),
        static_cast<std::int64_t>(state.winner),
        static_cast<std::int64_t>(state.activeCharacterId),
        static_cast<std::int64_t>(state.globalInitiative),
        static_cast<std::int64_t>(state.seed)};

    std::vector<const TileState*> tiles;
    tiles.reserve(state.tiles.size());
    for (const TileState& tile : state.tiles)
    {
        tiles.push_back(&tile);
    }
    std::sort(tiles.begin(), tiles.end(), [](const TileState* left, const TileState* right)
    {
        return left->id < right->id;
    });
    result.tiles.reserve(tiles.size() * NumericObservation::TileStride);
    for (const TileState* tile : tiles)
    {
        result.tiles.insert(result.tiles.end(), {
            tile->id,
            tile->coord.q,
            tile->coord.r,
            RequireIndex(schema.terrainIndices, tile->terrainId, "terrain"),
            tile->occupantId,
            tile->blocksMovement ? 1 : 0,
            tile->occupantId == InvalidEntityId ? 0 : 1});
    }

    std::vector<const CharacterState*> characters;
    characters.reserve(state.characters.size());
    for (const CharacterState& character : state.characters)
    {
        characters.push_back(&character);
    }
    std::sort(characters.begin(), characters.end(), [](const CharacterState* left, const CharacterState* right)
    {
        return left->id < right->id;
    });
    result.characters.reserve(characters.size() * NumericObservation::CharacterStride);
    for (const CharacterState* character : characters)
    {
        result.characters.insert(result.characters.end(), {
            character->id,
            RequireIndex(schema.characterIndices, character->definitionId, "character"),
            static_cast<std::int64_t>(character->team),
            character->tileId,
            character->health,
            character->initiative,
            character->alive ? 1 : 0,
            character->movementBlocked ? 1 : 0,
            character->canTakeTurn ? 1 : 0,
            character->initiativeGainPerTick,
            character->delayedActionInitiative});

        for (std::size_t abilityIndex = 0; abilityIndex < character->abilities.size(); ++abilityIndex)
        {
            const AbilityState& ability = character->abilities[abilityIndex];
            result.abilities.insert(result.abilities.end(), {
                character->id,
                static_cast<std::int64_t>(abilityIndex),
                RequireIndex(schema.abilityIndices, ability.definitionId, "ability"),
                ability.cooldownRemaining,
                ability.cooldownDuration,
                ability.currentCharges,
                ability.maxCharges,
                ability.usesCharges ? 1 : 0});
        }
        for (std::size_t effectIndex = 0; effectIndex < character->modifierEffects.size(); ++effectIndex)
        {
            const ModifierEffectState& effect = character->modifierEffects[effectIndex];
            result.modifierEffects.insert(result.modifierEffects.end(), {
                character->id, static_cast<std::int64_t>(effectIndex), effect.causerId,
                effect.remainingInitiative, effect.remainingTurns,
                effect.incomingDamageMultiplierBasisPoints, effect.outgoingDamageMultiplierBasisPoints,
                effect.defenseIgnoreAdd, effect.healFractionFromDamageDealtBasisPoints,
                effect.moveCostAdd, effect.moveCostMultiplierBasisPoints, effect.initiativeLossOnMove,
                effect.firstMoveInitiativeLoss, effect.maximumMovedFields, effect.movedFields,
                effect.basicAttackCostAdd, effect.basicAttackDamageMultiplierBasisPoints,
                effect.incomingDamageMultiplierFromCauserBasisPoints, effect.initiativeLossOnExpire,
                effect.onlyAffectCauserDamage ? 1 : 0,
                effect.onlyAgainstRangedSources ? 1 : 0, effect.blockMovementBonuses ? 1 : 0,
                effect.basicAttackCostAvailable ? 1 : 0, effect.consumeOnBasicAttack ? 1 : 0,
                effect.preventsAbilityUse ? 1 : 0, effect.preventsMovement ? 1 : 0,
                effect.firstMovePenaltyAvailable ? 1 : 0,
                effect.expireOnTurnEnd ? 1 : 0});
        }
    }

    result.legalActions.reserve(legalActions.size() * NumericObservation::ActionStride);
    for (std::size_t actionIndex = 0; actionIndex < legalActions.size(); ++actionIndex)
    {
        const GameAction& action = legalActions[actionIndex];
        const std::int64_t abilityIndex = action.abilityId.empty()
            ? -1
            : RequireIndex(schema.abilityIndices, action.abilityId, "ability");
        result.legalActions.insert(result.legalActions.end(), {
            static_cast<std::int64_t>(action.type),
            action.actorId,
            action.targetEntityId,
            action.targetTileId,
            abilityIndex,
            static_cast<std::int64_t>(action.selections.size())});
        for (std::size_t selectionIndex = 0; selectionIndex < action.selections.size(); ++selectionIndex)
        {
            result.actionSelections.insert(result.actionSelections.end(), {
                static_cast<std::int64_t>(actionIndex),
                static_cast<std::int64_t>(selectionIndex),
                action.selections[selectionIndex]});
        }
    }
    return result;
}
}
