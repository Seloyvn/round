#include "round/core/Package.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace roundcore
{
namespace
{
constexpr std::uint8_t Magic[] = {'R', 'N', 'D', 'C'};
constexpr std::uint32_t MaximumCollectionSize = 1000000;

template <typename T, bool IsEnum = std::is_enum_v<T>>
struct IntegerRawType { using Type = T; };
template <typename T>
struct IntegerRawType<T, true> { using Type = std::underlying_type_t<T>; };
template <>
struct IntegerRawType<bool, false> { using Type = std::uint8_t; };

class Writer
{
public:
    template <typename T>
    void Integer(const T value)
    {
        static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
        using Raw = typename IntegerRawType<T>::Type;
        using Unsigned = std::make_unsigned_t<Raw>;
        Unsigned raw = static_cast<Unsigned>(value);
        for (std::size_t index = 0; index < sizeof(Unsigned); ++index)
        {
            bytes_.push_back(static_cast<std::uint8_t>((raw >> (index * 8)) & 0xffu));
        }
    }

    void Double(const double value)
    {
        std::uint64_t raw = 0;
        static_assert(sizeof(raw) == sizeof(value));
        std::memcpy(&raw, &value, sizeof(raw));
        Integer(raw);
    }

    void String(const std::string& value)
    {
        if (value.size() > std::numeric_limits<std::uint32_t>::max())
        {
            throw std::length_error("Match package string is too large.");
        }
        Integer(static_cast<std::uint32_t>(value.size()));
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }

    std::vector<std::uint8_t> Finish() { return std::move(bytes_); }

private:
    std::vector<std::uint8_t> bytes_;
};

class Reader
{
public:
    explicit Reader(const std::vector<std::uint8_t>& bytes) : bytes_(bytes) {}

    template <typename T>
    T Integer()
    {
        static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
        using Raw = typename IntegerRawType<T>::Type;
        using Unsigned = std::make_unsigned_t<Raw>;
        Require(sizeof(Unsigned));
        Unsigned raw = 0;
        for (std::size_t index = 0; index < sizeof(Unsigned); ++index)
        {
            raw |= static_cast<Unsigned>(bytes_[offset_++]) << (index * 8);
        }
        return static_cast<T>(static_cast<Raw>(raw));
    }

    double Double()
    {
        const std::uint64_t raw = Integer<std::uint64_t>();
        double value = 0;
        std::memcpy(&value, &raw, sizeof(value));
        return value;
    }

    std::string String()
    {
        const std::uint32_t size = Count();
        Require(size);
        std::string result(bytes_.begin() + static_cast<std::ptrdiff_t>(offset_),
            bytes_.begin() + static_cast<std::ptrdiff_t>(offset_ + size));
        offset_ += size;
        return result;
    }

    std::uint32_t Count()
    {
        const std::size_t countOffset = offset_;
        const std::uint32_t count = Integer<std::uint32_t>();
        if (count > MaximumCollectionSize)
        {
            throw std::invalid_argument(
                "Match package collection exceeds its safety limit at byte "
                + std::to_string(countOffset) + ": " + std::to_string(count) + ".");
        }
        return count;
    }

    bool Finished() const noexcept { return offset_ == bytes_.size(); }

private:
    void Require(const std::size_t size)
    {
        if (size > bytes_.size() - std::min(offset_, bytes_.size()))
        {
            throw std::invalid_argument("Match package ended unexpectedly.");
        }
    }

    const std::vector<std::uint8_t>& bytes_;
    std::size_t offset_ = 0;
};

template <typename Map>
std::vector<typename Map::key_type> SortedKeys(const Map& values)
{
    std::vector<typename Map::key_type> keys;
    keys.reserve(values.size());
    for (const auto& value : values) keys.push_back(value.first);
    std::sort(keys.begin(), keys.end());
    return keys;
}

void WriteValues(Writer& writer, const std::unordered_map<std::string, double>& values)
{
    const auto keys = SortedKeys(values);
    writer.Integer(static_cast<std::uint32_t>(keys.size()));
    for (const std::string& key : keys)
    {
        writer.String(key);
        writer.Double(values.at(key));
    }
}

std::unordered_map<std::string, double> ReadValues(Reader& reader)
{
    std::unordered_map<std::string, double> result;
    for (std::uint32_t index = 0, count = reader.Count(); index < count; ++index)
    {
        // Function-argument evaluation order is not guaranteed. The binary
        // format always stores the key before the value, so read explicitly.
        std::string key = reader.String();
        const double value = reader.Double();
        result.emplace(std::move(key), value);
    }
    return result;
}

void WriteDefinitions(Writer& writer, const ContentDefinitions& definitions)
{
    writer.Integer(definitions.schemaVersion);
    writer.String(definitions.contentVersion);
    const auto characterKeys = SortedKeys(definitions.characters);
    writer.Integer(static_cast<std::uint32_t>(characterKeys.size()));
    for (const DefinitionId& key : characterKeys)
    {
        const CharacterDefinition& value = definitions.characters.at(key);
        writer.String(key);
        writer.String(value.id);
        writer.Integer(value.stats.maxHealth); writer.Integer(value.stats.attack); writer.Integer(value.stats.defense);
        writer.Integer(value.stats.maxInitiative); writer.Integer(value.stats.visionRange);
        writer.Integer(value.stats.attackRange); writer.Integer(value.stats.moveCost);
        writer.Integer(value.stats.attackCost); writer.Integer(value.stats.turnDelay);
        writer.Integer(static_cast<std::uint32_t>(value.abilityIds.size()));
        for (const DefinitionId& abilityId : value.abilityIds) writer.String(abilityId);
        WriteValues(writer, value.passiveValues);
        writer.Integer(value.criticalChanceBasisPoints);
        writer.Integer(value.criticalDamageMultiplierBasisPoints);
    }
    const auto abilityKeys = SortedKeys(definitions.abilities);
    writer.Integer(static_cast<std::uint32_t>(abilityKeys.size()));
    for (const DefinitionId& key : abilityKeys)
    {
        const AbilityDefinition& value = definitions.abilities.at(key);
        writer.String(key); writer.String(value.id); writer.Integer(value.operation); writer.Integer(value.targetRelation);
        writer.Integer(value.initiativeCost); writer.Integer(value.range); writer.Integer(value.amount);
        writer.Integer(value.defenseIgnore); writer.Integer(value.requiresVisibility); WriteValues(writer, value.values);
    }
    const auto terrainKeys = SortedKeys(definitions.terrains);
    writer.Integer(static_cast<std::uint32_t>(terrainKeys.size()));
    for (const DefinitionId& key : terrainKeys)
    {
        const TerrainDefinition& value = definitions.terrains.at(key);
        writer.String(key); writer.String(value.id); writer.Integer(value.walkable); writer.Integer(value.blocksVision);
        writer.Integer(value.stopsMovement); writer.Integer(value.movementCost);
        writer.Integer(value.movementCostMultiplierBasisPoints); WriteValues(writer, value.values);
        writer.Integer(value.visionCost); writer.Integer(value.visionRangeModifier);
    }
}

ContentDefinitions ReadDefinitions(Reader& reader)
{
    ContentDefinitions result;
    result.schemaVersion = reader.Integer<std::uint32_t>();
    result.contentVersion = reader.String();
    for (std::uint32_t index = 0, count = reader.Count(); index < count; ++index)
    {
        const DefinitionId key = reader.String(); CharacterDefinition value; value.id = reader.String();
        value.stats.maxHealth = reader.Integer<std::int32_t>(); value.stats.attack = reader.Integer<std::int32_t>();
        value.stats.defense = reader.Integer<std::int32_t>(); value.stats.maxInitiative = reader.Integer<std::int32_t>();
        value.stats.visionRange = reader.Integer<std::int32_t>(); value.stats.attackRange = reader.Integer<std::int32_t>();
        value.stats.moveCost = reader.Integer<std::int32_t>(); value.stats.attackCost = reader.Integer<std::int32_t>();
        value.stats.turnDelay = reader.Integer<std::int32_t>();
        for (std::uint32_t ability = 0, abilityCount = reader.Count(); ability < abilityCount; ++ability)
            value.abilityIds.push_back(reader.String());
        value.passiveValues = ReadValues(reader); value.criticalChanceBasisPoints = reader.Integer<std::int32_t>();
        value.criticalDamageMultiplierBasisPoints = reader.Integer<std::int32_t>();
        result.characters.emplace(key, std::move(value));
    }
    for (std::uint32_t index = 0, count = reader.Count(); index < count; ++index)
    {
        const DefinitionId key = reader.String(); AbilityDefinition value; value.id = reader.String();
        value.operation = reader.Integer<AbilityOperation>(); value.targetRelation = reader.Integer<AbilityTargetRelation>();
        value.initiativeCost = reader.Integer<std::int32_t>(); value.range = reader.Integer<std::int32_t>();
        value.amount = reader.Integer<std::int32_t>(); value.defenseIgnore = reader.Integer<std::int32_t>();
        value.requiresVisibility = reader.Integer<bool>(); value.values = ReadValues(reader);
        result.abilities.emplace(key, std::move(value));
    }
    for (std::uint32_t index = 0, count = reader.Count(); index < count; ++index)
    {
        const DefinitionId key = reader.String(); TerrainDefinition value; value.id = reader.String();
        value.walkable = reader.Integer<bool>(); value.blocksVision = reader.Integer<bool>();
        value.stopsMovement = reader.Integer<bool>(); value.movementCost = reader.Integer<std::int32_t>();
        value.movementCostMultiplierBasisPoints = reader.Integer<std::int32_t>(); value.values = ReadValues(reader);
        value.visionCost = reader.Integer<std::int32_t>(); value.visionRangeModifier = reader.Integer<std::int32_t>();
        result.terrains.emplace(key, std::move(value));
    }
    return result;
}

void WriteState(Writer& writer, const GameState& state)
{
    writer.Integer(state.schemaVersion); writer.Integer(state.matchId); writer.Integer(state.seed);
    writer.Integer(state.randomState); writer.Integer(state.step); writer.Integer(state.status); writer.Integer(state.winner);
    writer.Integer(state.activeCharacterId); writer.Integer(state.globalInitiative);
    writer.Integer(static_cast<std::uint32_t>(state.tiles.size()));
    for (const TileState& tile : state.tiles)
    {
        writer.Integer(tile.id); writer.Integer(tile.coord.q); writer.Integer(tile.coord.r); writer.String(tile.terrainId);
        writer.Integer(tile.occupantId); writer.Integer(tile.blocksMovement);
    }
    writer.Integer(static_cast<std::uint32_t>(state.characters.size()));
    for (const CharacterState& character : state.characters)
    {
        writer.Integer(character.id); writer.String(character.definitionId); writer.Integer(character.team);
        writer.Integer(character.tileId); writer.Integer(character.health); writer.Integer(character.initiative);
        writer.Integer(character.alive); writer.Integer(character.movementBlocked); writer.Integer(character.canTakeTurn);
        writer.Integer(character.initiativeGainPerTick); writer.Integer(character.delayedActionInitiative);
        writer.Integer(static_cast<std::uint32_t>(character.abilities.size()));
        for (const AbilityState& ability : character.abilities)
        {
            writer.String(ability.definitionId); writer.Integer(ability.cooldownRemaining);
            writer.Integer(ability.cooldownDuration); writer.Integer(ability.currentCharges);
            writer.Integer(ability.maxCharges); writer.Integer(ability.usesCharges);
        }
        writer.Integer(static_cast<std::uint32_t>(character.modifierEffects.size()));
        for (const ModifierEffectState& effect : character.modifierEffects)
        {
            writer.String(effect.id); writer.Integer(effect.causerId); writer.Integer(effect.remainingInitiative);
            writer.Integer(effect.remainingTurns); writer.Integer(effect.incomingDamageMultiplierBasisPoints);
            writer.Integer(effect.outgoingDamageMultiplierBasisPoints); writer.Integer(effect.defenseIgnoreAdd);
            writer.Integer(effect.healFractionFromDamageDealtBasisPoints);
            writer.Integer(effect.onlyAffectCauserDamage); writer.Integer(effect.onlyAgainstRangedSources);
            writer.Integer(effect.expireOnTurnEnd);
        }
    }
}

GameState ReadState(Reader& reader)
{
    GameState state;
    state.schemaVersion = reader.Integer<std::uint32_t>(); state.matchId = reader.Integer<MatchId>();
    state.seed = reader.Integer<std::uint64_t>(); state.randomState = reader.Integer<std::uint64_t>();
    state.step = reader.Integer<std::uint64_t>(); state.status = reader.Integer<MatchStatus>();
    state.winner = reader.Integer<Team>(); state.activeCharacterId = reader.Integer<EntityId>();
    state.globalInitiative = reader.Integer<std::uint64_t>();
    for (std::uint32_t index = 0, count = reader.Count(); index < count; ++index)
    {
        TileState tile; tile.id = reader.Integer<EntityId>(); tile.coord.q = reader.Integer<std::int32_t>();
        tile.coord.r = reader.Integer<std::int32_t>(); tile.terrainId = reader.String();
        tile.occupantId = reader.Integer<EntityId>(); tile.blocksMovement = reader.Integer<bool>();
        state.tiles.push_back(std::move(tile));
    }
    for (std::uint32_t index = 0, count = reader.Count(); index < count; ++index)
    {
        CharacterState character; character.id = reader.Integer<EntityId>(); character.definitionId = reader.String();
        character.team = reader.Integer<Team>(); character.tileId = reader.Integer<EntityId>();
        character.health = reader.Integer<std::int32_t>(); character.initiative = reader.Integer<std::int32_t>();
        character.alive = reader.Integer<bool>(); character.movementBlocked = reader.Integer<bool>();
        character.canTakeTurn = reader.Integer<bool>(); character.initiativeGainPerTick = reader.Integer<std::int32_t>();
        character.delayedActionInitiative = reader.Integer<std::int32_t>();
        for (std::uint32_t ability = 0, abilityCount = reader.Count(); ability < abilityCount; ++ability)
        {
            AbilityState value; value.definitionId = reader.String(); value.cooldownRemaining = reader.Integer<std::int32_t>();
            value.cooldownDuration = reader.Integer<std::int32_t>(); value.currentCharges = reader.Integer<std::int32_t>();
            value.maxCharges = reader.Integer<std::int32_t>(); value.usesCharges = reader.Integer<bool>();
            character.abilities.push_back(std::move(value));
        }
        for (std::uint32_t effect = 0, effectCount = reader.Count(); effect < effectCount; ++effect)
        {
            ModifierEffectState value; value.id = reader.String(); value.causerId = reader.Integer<EntityId>();
            value.remainingInitiative = reader.Integer<std::int32_t>(); value.remainingTurns = reader.Integer<std::int32_t>();
            value.incomingDamageMultiplierBasisPoints = reader.Integer<std::int32_t>();
            value.outgoingDamageMultiplierBasisPoints = reader.Integer<std::int32_t>();
            value.defenseIgnoreAdd = reader.Integer<std::int32_t>();
            value.healFractionFromDamageDealtBasisPoints = reader.Integer<std::int32_t>();
            value.onlyAffectCauserDamage = reader.Integer<bool>(); value.onlyAgainstRangedSources = reader.Integer<bool>();
            value.expireOnTurnEnd = reader.Integer<bool>();
            character.modifierEffects.push_back(std::move(value));
        }
        state.characters.push_back(std::move(character));
    }
    return state;
}
}

std::vector<std::uint8_t> SerializeMatchSetup(const MatchSetup& setup)
{
    Writer writer;
    for (const std::uint8_t byte : Magic) writer.Integer(byte);
    writer.Integer(MatchPackageVersion);
    WriteDefinitions(writer, setup.definitions);
    WriteState(writer, setup.initialState);
    return writer.Finish();
}

MatchSetup DeserializeMatchSetup(const std::vector<std::uint8_t>& bytes)
{
    Reader reader(bytes);
    for (const std::uint8_t expected : Magic)
    {
        if (reader.Integer<std::uint8_t>() != expected) throw std::invalid_argument("Invalid RoundCore match package magic.");
    }
    const std::uint32_t version = reader.Integer<std::uint32_t>();
    if (version != MatchPackageVersion) throw std::invalid_argument("Unsupported RoundCore match package version.");
    MatchSetup setup; setup.definitions = ReadDefinitions(reader); setup.initialState = ReadState(reader);
    if (!reader.Finished()) throw std::invalid_argument("RoundCore match package contains trailing data.");
    return setup;
}
}
