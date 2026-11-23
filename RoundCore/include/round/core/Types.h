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

enum class AbilityOperation : std::uint8_t
{
    Unsupported,
    DirectDamage,
    DirectHeal,
    SelfModifier,
    TargetModifier,
    GroundAura,
    Knockback,
    DelayStrike,
    Shackle,
    Bind,
    Taunt,
    Snipe,
    Concentrate,
    Aim,
    PiercingShot,
    SuppressingFire,
    Shadowstrike,
    Armorpen,
    Execute,
    HookShot,
    DeathMark,
    Fireball,
    Icebeam,
    Lightningstrike,
    Rockblast,
    ArcaneShift
};

enum class ModifierEffectKind : std::uint8_t
{
    Generic,
    Concentrate,
    AimField,
    SuppressingFire,
    PiercingMark,
    TrackingMark,
    SnipeMark,
    AimMark,
    AimWeakness,
    Invisibility,
    AssassinHuntMark,
    ShadowWound,
    DeathMark,
    SharedBreach,
    BasicAttackWeakness,
    Bleed,
    AssassinFollowup,
    HookAnchor,
    FireballBurn,
    StaticCharge,
    DelayedArea,
    RockEcho,
    OneHitProtection,
    IcebeamSlow
};

enum class AbilityTargetRelation : std::uint8_t
{
    Self,
    Ally,
    Enemy,
    AnyCharacter
};

enum class DamageType : std::uint8_t
{
    Physical,
    Magical,
    Fire,
    Ice,
    Lightning,
    Holy,
    Ground,
    TrueDamage,
    Nature
};

enum class EventType : std::uint8_t
{
    MatchReset,
    CharacterMoved,
    DamageApplied,
    HealingApplied,
    InitiativeChanged,
    CooldownChanged,
    AbilityChargesChanged,
    ResourceChanged,
    EffectAdded,
    EffectRemoved,
    TerrainChanged,
    GlobalInitiativeAdvanced,
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

constexpr std::int32_t HexDistance(const HexCoord& left, const HexCoord& right) noexcept
{
    const std::int32_t dq = left.q - right.q;
    const std::int32_t dr = left.r - right.r;
    const std::int32_t ds = (left.q + left.r) - (right.q + right.r);
    const auto absolute = [](const std::int32_t value) constexpr { return value < 0 ? -value : value; };
    return (absolute(dq) + absolute(dr) + absolute(ds)) / 2;
}
}
