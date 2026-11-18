#pragma once

#include <cstdint>
#include <string>

namespace roundcore
{
using MatchId = std::uint64_t;
using EntityId = std::uint32_t;
using DefinitionId = std::string;

constexpr EntityId InvalidEntityId = 0;

enum class Team : std::uint8_t
{
    Neutral,
    TeamA,
    TeamB
};

enum class ActionType : std::uint8_t
{
    Move,
    BasicAttack,
    UseAbility,
    ClaimObjective,
    Wait,
    EndTurn
};

enum class EventType : std::uint8_t
{
    MatchReset,
    CharacterMoved,
    DamageApplied,
    HealingApplied,
    InitiativeChanged,
    CooldownChanged,
    EffectAdded,
    EffectRemoved,
    TerrainChanged,
    CharacterDied,
    ActiveCharacterChanged,
    ObjectiveChanged,
    MatchEnded
};

enum class MatchStatus : std::uint8_t
{
    NotStarted,
    Running,
    Completed
};

struct HexCoord
{
    std::int32_t q = 0;
    std::int32_t r = 0;

    friend constexpr bool operator==(const HexCoord& left, const HexCoord& right) noexcept
    {
        return left.q == right.q && left.r == right.r;
    }

    friend constexpr bool operator!=(const HexCoord& left, const HexCoord& right) noexcept
    {
        return !(left == right);
    }
};
}
