#pragma once

#include "round/core/Types.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace roundcore
{
struct StatBlock
{
    std::int32_t maxHealth = 0;
    std::int32_t attack = 0;
    std::int32_t defense = 0;
    std::int32_t maxInitiative = 0;
    std::int32_t visionRange = 0;
    std::int32_t attackRange = 0;
    std::int32_t moveCost = 0;
    std::int32_t attackCost = 0;
    std::int32_t turnDelay = 0;
};

struct AbilityDefinition
{
    DefinitionId id;
    AbilityOperation operation = AbilityOperation::Unsupported;
    AbilityTargetRelation targetRelation = AbilityTargetRelation::Enemy;
    std::int32_t initiativeCost = 0;
    std::int32_t range = 0;
    std::int32_t amount = 0;
    std::int32_t defenseIgnore = 0;
    bool requiresVisibility = true;
    std::unordered_map<std::string, double> values;
};

struct CharacterDefinition
{
    DefinitionId id;
    StatBlock stats;
    std::vector<DefinitionId> abilityIds;
    std::unordered_map<std::string, double> passiveValues;
    std::int32_t criticalChanceBasisPoints = 0;
    std::int32_t criticalDamageMultiplierBasisPoints = 15000;
};

struct TerrainDefinition
{
    DefinitionId id;
    bool walkable = true;
    bool blocksVision = false;
    bool stopsMovement = false;
    // Minimum cost of entering this terrain after applying its multiplier.
    std::int32_t movementCost = 1;
    // Fixed point keeps movement deterministic across compilers and platforms.
    std::int32_t movementCostMultiplierBasisPoints = 10000;
    std::unordered_map<std::string, double> values;
    std::int32_t visionCost = 1;
    std::int32_t visionRangeModifier = 0;
};

struct ContentDefinitions
{
    std::uint32_t schemaVersion = 1;
    std::string contentVersion;
    std::unordered_map<DefinitionId, CharacterDefinition> characters;
    std::unordered_map<DefinitionId, AbilityDefinition> abilities;
    std::unordered_map<DefinitionId, TerrainDefinition> terrains;
};
}
