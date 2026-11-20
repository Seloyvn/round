#pragma once

#include "round/core/Actions.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace roundcore
{
struct ContentDefinitions;

struct ObservationSchema
{
    std::uint32_t schemaVersion = 1;
    std::string contentVersion;
    std::vector<DefinitionId> characterDefinitions;
    std::vector<DefinitionId> terrainDefinitions;
    std::vector<DefinitionId> abilityDefinitions;
    std::unordered_map<DefinitionId, std::int32_t> characterIndices;
    std::unordered_map<DefinitionId, std::int32_t> terrainIndices;
    std::unordered_map<DefinitionId, std::int32_t> abilityIndices;
};

struct NumericObservation
{
    static constexpr std::size_t GlobalStride = 7;
    static constexpr std::size_t TileStride = 7;
    static constexpr std::size_t CharacterStride = 11;
    static constexpr std::size_t AbilityStride = 8;
    static constexpr std::size_t ModifierEffectStride = 19;
    static constexpr std::size_t ActionStride = 6;
    static constexpr std::size_t ActionSelectionStride = 3;

    std::uint32_t schemaVersion = 1;
    std::string contentVersion;
    // The caller chooses an omniscient or team-filtered Game snapshot before encoding.
    std::vector<std::int64_t> global;
    std::vector<std::int64_t> tiles;
    std::vector<std::int64_t> characters;
    std::vector<std::int64_t> abilities;
    std::vector<std::int64_t> modifierEffects;
    std::vector<std::int64_t> legalActions;
    std::vector<std::int64_t> actionSelections;
};

ObservationSchema BuildObservationSchema(const ContentDefinitions& definitions);
NumericObservation EncodeOmniscientObservation(
    const GameState& state,
    const std::vector<GameAction>& legalActions,
    const ObservationSchema& schema);
}
