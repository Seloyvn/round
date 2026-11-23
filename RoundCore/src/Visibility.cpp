#include "round/core/Game.h"

#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace roundcore
{
namespace
{
struct CubeCoord
{
    std::int64_t x;
    std::int64_t y;
    std::int64_t z;
};

std::int64_t RoundRatio(const std::int64_t numerator, const std::int64_t denominator)
{
    if (numerator >= 0)
    {
        return (numerator + denominator / 2) / denominator;
    }
    return -((-numerator + denominator / 2) / denominator);
}

HexCoord LineCoord(const HexCoord from, const HexCoord to, const std::int32_t step, const std::int32_t distance)
{
    const CubeCoord first{from.q, -static_cast<std::int64_t>(from.q) - from.r, from.r};
    const CubeCoord last{to.q, -static_cast<std::int64_t>(to.q) - to.r, to.r};
    const std::int64_t inverseStep = distance - step;
    const std::int64_t xNumerator = first.x * inverseStep + last.x * step;
    const std::int64_t yNumerator = first.y * inverseStep + last.y * step;
    const std::int64_t zNumerator = first.z * inverseStep + last.z * step;
    std::int64_t x = RoundRatio(xNumerator, distance);
    std::int64_t y = RoundRatio(yNumerator, distance);
    std::int64_t z = RoundRatio(zNumerator, distance);
    const std::int64_t xDiff = std::abs(x * distance - xNumerator);
    const std::int64_t yDiff = std::abs(y * distance - yNumerator);
    const std::int64_t zDiff = std::abs(z * distance - zNumerator);
    if (xDiff > yDiff && xDiff > zDiff)
    {
        x = -y - z;
    }
    else if (yDiff > zDiff)
    {
        y = -x - z;
    }
    else
    {
        z = -x - y;
    }
    return {static_cast<std::int32_t>(x), static_cast<std::int32_t>(z)};
}
}

std::vector<EntityId> Game::VisibleTileIds(const EntityId characterId) const
{
    const CharacterState* character = FindCharacter(characterId);
    if (!character || !character->alive)
    {
        return {};
    }
    const TileState* source = FindTile(character->tileId);
    const auto characterDefinition = definitions_.characters.find(character->definitionId);
    if (!source || characterDefinition == definitions_.characters.end())
    {
        return {};
    }
    const auto sourceTerrain = definitions_.terrains.find(source->terrainId);
    const std::int32_t rangeModifier = sourceTerrain == definitions_.terrains.end()
        ? 0
        : sourceTerrain->second.visionRangeModifier;
    const std::int32_t visionRange = std::max(0, characterDefinition->second.stats.visionRange + rangeModifier);

    std::vector<EntityId> visible;
    for (const TileState& candidate : state_.tiles)
    {
        const std::int32_t distance = HexDistance(source->coord, candidate.coord);
        if (distance > visionRange)
        {
            continue;
        }
        bool occluded = false;
        std::int32_t visionCost = 0;
        for (std::int32_t step = 1; step <= distance; ++step)
        {
            const HexCoord coord = LineCoord(source->coord, candidate.coord, step, distance);
            const auto lineTile = std::find_if(state_.tiles.begin(), state_.tiles.end(), [coord](const TileState& tile)
            {
                return tile.coord == coord;
            });
            if (lineTile == state_.tiles.end())
            {
                continue;
            }
            const auto terrain = definitions_.terrains.find(lineTile->terrainId);
            if (terrain != definitions_.terrains.end())
            {
                visionCost += std::max(0, terrain->second.visionCost);
                if (visionCost > visionRange || (step < distance && terrain->second.blocksVision))
                {
                    occluded = true;
                    break;
                }
            }
        }
        if (!occluded)
        {
            visible.push_back(candidate.id);
        }
    }
    std::sort(visible.begin(), visible.end());
    return visible;
}

std::vector<EntityId> Game::TeamVisibleTileIds(const Team team) const
{
    std::unordered_set<EntityId> unique;
    for (const CharacterState& character : state_.characters)
    {
        if (!character.alive || character.team != team)
        {
            continue;
        }
        for (const EntityId tileId : VisibleTileIds(character.id))
        {
            unique.insert(tileId);
        }
    }
    for (const CharacterState& character : state_.characters)
    {
        if (!character.alive) continue;
        const bool revealed = std::any_of(character.modifierEffects.begin(), character.modifierEffects.end(),
            [](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::SuppressingFire
                && (effect.flags & 4u) != 0u; });
        if (revealed) unique.insert(character.tileId);
    }
    for (const DeviceState& device : state_.devices)
    {
        const CharacterState* owner = FindCharacter(device.ownerId);
        const TileState* center = FindTile(device.tileId);
        if (!owner || owner->team != team || device.type != DeviceType::SpotterBeacon || !center) continue;
        for (const TileState& tile : state_.tiles)
            if (HexDistance(center->coord, tile.coord) <= device.range) unique.insert(tile.id);
    }
    std::vector<EntityId> result(unique.begin(), unique.end());
    std::sort(result.begin(), result.end());
    return result;
}
}
