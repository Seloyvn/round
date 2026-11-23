#include "round/core/Game.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>
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

double PassiveValue(const CharacterDefinition& definition, const char* key) noexcept
{
    const auto found = definition.passiveValues.find(key);
    return found == definition.passiveValues.end() ? 0.0 : found->second;
}

double AbilityValue(const AbilityDefinition& definition, const char* key) noexcept
{
    const auto found = definition.values.find(key);
    return found == definition.values.end() ? 0.0 : found->second;
}

std::int64_t ApplyBasisPoints(const std::int64_t value, const std::int32_t basisPoints) noexcept
{
    return (value * std::max(0, basisPoints) + 5000) / 10000;
}
}

ActionResult Game::Reset(MatchSetup setup)
{
    if (setup.definitions.schemaVersion == 0 || setup.initialState.schemaVersion == 0)
    {
        return Failure("invalid_schema", "Definition and state schema versions must be non-zero.");
    }
    if (setup.definitions.schemaVersion != setup.initialState.schemaVersion)
    {
        return Failure("schema_mismatch", "Definition and state schema versions must match.");
    }
    if (setup.definitions.contentVersion.empty())
    {
        return Failure("missing_content_version", "A match package must identify its exported content version.");
    }

    std::unordered_set<EntityId> tileIds;
    std::unordered_set<EntityId> characterIds;
    std::unordered_set<EntityId> occupiedTileIds;

    for (const auto& entry : setup.definitions.abilities)
    {
        const AbilityDefinition& ability = entry.second;
        if (entry.first.empty() || ability.id != entry.first)
        {
            return Failure("invalid_ability_definition_id", "Ability definition keys and IDs must be equal and non-empty.");
        }
        if (ability.operation > AbilityOperation::OverloadDevices
            || ability.targetRelation > AbilityTargetRelation::AnyCharacter)
        {
            return Failure("invalid_ability_contract", "Ability operation and target relation must be recognized.");
        }
        if (ability.initiativeCost < 0 || ability.range < 0 || ability.amount < 0 || ability.defenseIgnore < 0)
        {
            return Failure("invalid_ability_values", "Executable ability values must be non-negative.");
        }
    }
    for (const auto& entry : setup.definitions.characters)
    {
        if (entry.first.empty() || entry.second.id != entry.first)
        {
            return Failure("invalid_character_definition_id", "Character definition keys and IDs must be equal and non-empty.");
        }
        std::unordered_set<DefinitionId> abilityIds;
        for (const DefinitionId& abilityId : entry.second.abilityIds)
        {
            if (!abilityIds.insert(abilityId).second)
            {
                return Failure("duplicate_character_ability", "Character ability definition IDs must be unique.");
            }
            if (setup.definitions.abilities.find(abilityId) == setup.definitions.abilities.end())
            {
                return Failure("missing_declared_ability", "Every declared character ability must have a definition.");
            }
        }
    }

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
        if (character.resourceMaximum < 0 || character.resourceCurrent < 0
            || character.resourceCurrent > character.resourceMaximum
            || character.resourceRechargeProgress < 0 || character.resourceRechargeInterval < 0
            || character.specialCooldownRemaining < 0)
        {
            return Failure("invalid_character_resource", "Character resource state must be within non-negative bounds.");
        }
        if (setup.definitions.characters.find(character.definitionId) == setup.definitions.characters.end())
        {
            return Failure("missing_character_definition", "Every character must reference an exported character definition.");
        }
        const CharacterDefinition& characterDefinition = setup.definitions.characters.at(character.definitionId);
        std::unordered_set<DefinitionId> declaredAbilityIds(
            characterDefinition.abilityIds.begin(), characterDefinition.abilityIds.end());
        std::unordered_set<DefinitionId> runtimeAbilityIds;
        for (const AbilityState& ability : character.abilities)
        {
            if (ability.definitionId.empty() || !runtimeAbilityIds.insert(ability.definitionId).second)
            {
                return Failure("invalid_character_ability", "A character's runtime ability IDs must be unique and non-empty.");
            }
            if (setup.definitions.abilities.find(ability.definitionId) == setup.definitions.abilities.end())
            {
                return Failure("missing_ability_definition", "Every runtime ability must reference an exported ability definition.");
            }
            if (declaredAbilityIds.find(ability.definitionId) == declaredAbilityIds.end())
            {
                return Failure("undeclared_character_ability", "Every runtime ability must be declared by its character definition.");
            }
            if (ability.maxCharges < 1 || ability.currentCharges < 0 || ability.currentCharges > ability.maxCharges
                || ability.cooldownDuration < 0 || ability.cooldownRemaining < 0)
            {
                return Failure("invalid_ability_state", "Ability charges and cooldowns must be within valid non-negative bounds.");
            }
        }
        for (const ModifierEffectState& effect : character.modifierEffects)
        {
            if (effect.id.empty() || effect.remainingInitiative < 0 || effect.remainingTurns < 0
                || effect.incomingDamageMultiplierBasisPoints < 0 || effect.outgoingDamageMultiplierBasisPoints < 0
                || effect.healFractionFromDamageDealtBasisPoints < 0
                || effect.basicAttackDamageMultiplierBasisPoints < 0
                || effect.moveCostMultiplierBasisPoints < 0
                || effect.initiativeLossOnMove < 0
                || effect.firstMoveInitiativeLoss < 0
                || effect.maximumMovedFields < 0 || effect.movedFields < 0
                || effect.frostbreakDamage < 0
                || effect.incomingDamageMultiplierFromCauserBasisPoints < 0
                || effect.initiativeLossOnExpire < 0
                || effect.redirectPreMitigationBasisPoints < 0
                || effect.redirectedDamageMultiplierBasisPoints < 0
                || effect.protectorHealFractionBasisPoints < 0
                || effect.protectedInitiativePerHit < 0
                || effect.areaRadius < 0 || effect.areaDamage < 0
                || effect.areaInitiativeLossPerPulse < 0)
            {
                return Failure("invalid_modifier_effect", "Modifier effects require an ID and non-negative duration and multiplier values.");
            }
            if ((effect.areaDamage > 0 || effect.areaInitiativeLossPerPulse > 0)
                && !effect.areaFollowsCauser && tileIds.find(effect.areaCenterTileId) == tileIds.end())
            {
                return Failure("missing_area_center_tile", "Fixed ground auras must reference a board tile.");
            }
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

    for (const CharacterState& character : setup.initialState.characters)
    {
        for (const ModifierEffectState& effect : character.modifierEffects)
        {
            if (effect.causerId != InvalidEntityId && characterIds.find(effect.causerId) == characterIds.end())
            {
                return Failure("missing_effect_causer", "A modifier effect causer must reference a character in the match.");
            }
        }
        for (const RecentAuraPreventionState& prevention : character.recentAuraPreventions)
        {
            if (prevention.sourceId == InvalidEntityId || characterIds.find(prevention.sourceId) == characterIds.end())
            {
                return Failure("missing_aura_prevention_source",
                    "Recent aura prevention sources must reference a character in the match.");
            }
        }
    }
    std::unordered_set<EntityId> deviceIds;
    std::unordered_set<EntityId> deviceTileIds;
    for (const DeviceState& device : setup.initialState.devices)
    {
        if (device.id == InvalidEntityId || characterIds.find(device.id) != characterIds.end()
            || tileIds.find(device.id) != tileIds.end() || !deviceIds.insert(device.id).second)
            return Failure("invalid_device_id", "Device IDs must be globally unique and non-zero.");
        if (characterIds.find(device.ownerId) == characterIds.end())
            return Failure("missing_device_owner", "Every device must reference an existing owner.");
        if (tileIds.find(device.tileId) == tileIds.end() || !deviceTileIds.insert(device.tileId).second)
            return Failure("invalid_device_tile", "Every device must occupy one unique board tile.");
        if (setup.definitions.abilities.find(device.sourceAbilityId) == setup.definitions.abilities.end())
            return Failure("missing_device_ability", "Every device must reference its source ability.");
        if (device.health <= 0 || device.maximumHealth <= 0 || device.health > device.maximumHealth
            || device.remainingInitiative <= 0)
            return Failure("invalid_device_state", "Device health and lifetime must be positive and bounded.");
    }

    for (TileState& tile : setup.initialState.tiles)
    {
        tile.occupantId = InvalidEntityId;
        tile.fieldObjectId = InvalidEntityId;
    }
    for (const CharacterState& character : setup.initialState.characters)
    {
        const auto tile = std::find_if(setup.initialState.tiles.begin(), setup.initialState.tiles.end(),
            [&character](const TileState& candidate) { return candidate.id == character.tileId; });
        tile->occupantId = character.id;
    }
    for (const DeviceState& device : setup.initialState.devices)
    {
        const auto tile = std::find_if(setup.initialState.tiles.begin(), setup.initialState.tiles.end(),
            [&device](const TileState& candidate) { return candidate.id == device.tileId; });
        tile->fieldObjectId = device.id;
        tile->fieldObjectHealth = device.health;
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

    for (CharacterState& character : state_.characters)
    {
        const CharacterDefinition& definition = definitions_.characters.at(character.definitionId);
        if (PassiveValue(definition, "CoreComboCharacter") <= 0.0) continue;
        character.resourceMaximum = std::max(1, static_cast<std::int32_t>(
            PassiveValue(definition, "BaseComboMaximum") + PassiveValue(definition, "MaxComboPointsAdd")));
        if (character.resourceCurrent <= 0)
            character.resourceCurrent = std::clamp(static_cast<std::int32_t>(
                PassiveValue(definition, "BaseComboStart") + PassiveValue(definition, "ComboPointsStartAdd")),
                0, character.resourceMaximum);
        const std::int32_t invisibility = std::max(0, static_cast<std::int32_t>(
            PassiveValue(definition, "StartInvisibilityDurationInitiative")));
        if (!character.startInvisibilityApplied && invisibility > 0)
        {
            ModifierEffectState effect; effect.id = "Character3StartInvisibility";
            effect.kind = ModifierEffectKind::Invisibility; effect.causerId = character.id;
            effect.remainingInitiative = invisibility; character.modifierEffects.push_back(effect);
            character.startInvisibilityApplied = true;
        }
    }

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
    if (action.type == ActionType::UseAbility)
    {
        return ApplyAbility(action);
    }

    CharacterState* character = FindCharacter(action.actorId);
    TileState* origin = character ? FindTile(character->tileId) : nullptr;
    TileState* destination = FindTile(action.targetTileId);
    if (!character || !origin || !destination)
    {
        return Failure("invalid_state", "Validated movement references missing state.");
    }

    const std::int32_t calculatedCost = MovementCost(*character, *destination);
    const bool assassinFreeMove = character->freeAdjacentMoveAvailable && !character->freeAdjacentMoveUsedThisTurn;
    const std::int32_t cost = (character->hasFreePreparedStep || assassinFreeMove) ? 0 : calculatedCost;
    std::int64_t initiativeLossOnMove = 0;
    for (const ModifierEffectState& effect : character->modifierEffects)
    {
        initiativeLossOnMove = std::min<std::int64_t>(
            std::numeric_limits<std::int32_t>::max(),
            initiativeLossOnMove + std::max(0, effect.initiativeLossOnMove));
        if (effect.firstMovePenaltyAvailable)
        {
            initiativeLossOnMove = std::min<std::int64_t>(
                std::numeric_limits<std::int32_t>::max(),
                initiativeLossOnMove + std::max(0, effect.firstMoveInitiativeLoss));
        }
    }
    origin->occupantId = InvalidEntityId;
    destination->occupantId = character->id;
    character->tileId = destination->id;
    character->movedSinceLastTurn = true;
    character->stationaryGuardActive = false;
    character->stationaryShotStacks = 0;
    if (character->hasFreePreparedStep) character->hasFreePreparedStep = false;
    if (assassinFreeMove)
    {
        character->freeAdjacentMoveAvailable = false;
        character->freeAdjacentMoveUsedThisTurn = true;
    }
    if (PassiveValue(definitions_.characters.at(character->definitionId), "CoreComboCharacter") > 0.0)
    {
        ++character->movedFieldsThisTurn;
        const CharacterDefinition& assassinDefinition = definitions_.characters.at(character->definitionId);
        const std::int32_t threshold = std::max(1, static_cast<std::int32_t>(
            PassiveValue(assassinDefinition, "MoveDistanceForBonuses")));
        if (character->movedFieldsThisTurn >= threshold)
        {
            character->nextDamageAfterMoveReady = PassiveValue(
                assassinDefinition, "NextHitDamageMultiplierAfterMove") > 0.0;
            character->nextReductionAfterMoveReady = PassiveValue(
                assassinDefinition, "NextIncomingDamageMultiplierAfterMove") > 0.0;
        }
        for (ModifierEffectState& mark : character->modifierEffects)
        {
            if (mark.kind == ModifierEffectKind::DeathMark && (mark.flags & 8u) == 0u && mark.valueA > 0)
            {
                mark.remainingInitiative += mark.valueA;
                mark.flags |= 8u;
            }
        }
    }
    for (auto effect = character->modifierEffects.begin(); effect != character->modifierEffects.end();)
    {
        if (effect->kind == ModifierEffectKind::Concentrate && (effect->flags & 1u) != 0u)
        {
            effect = character->modifierEffects.erase(effect);
        }
        else ++effect;
    }
    const CharacterDefinition& movingDefinition = definitions_.characters.at(character->definitionId);
    const double receivedDelayMultiplier = PassiveValue(movingDefinition, "DelayReceivedMultiplier");
    if (initiativeLossOnMove > 0 && receivedDelayMultiplier > 0.0)
    {
        initiativeLossOnMove = static_cast<std::int64_t>(initiativeLossOnMove * receivedDelayMultiplier + 0.5);
    }
    character->initiative = static_cast<std::int32_t>(std::max<std::int64_t>(
        0, static_cast<std::int64_t>(character->initiative) - cost - initiativeLossOnMove));
    ++state_.step;

    ActionResult result;
    result.success = true;
    result.message = "Movement executed.";
    result.events.push_back(GameEvent{EventType::CharacterMoved, character->id, InvalidEntityId, destination->id, {}, cost});
    result.events.push_back(GameEvent{EventType::InitiativeChanged, character->id, character->id, destination->id, {}, -cost});
    if (initiativeLossOnMove > 0)
    {
        result.events.push_back(GameEvent{EventType::InitiativeChanged, character->id, character->id,
            destination->id, "Slow", -static_cast<std::int32_t>(initiativeLossOnMove)});
    }
    if (character->alive && destination->fieldObjectId != InvalidEntityId)
    {
        const DeviceState* enteredDevice = FindDevice(destination->fieldObjectId);
        if (enteredDevice && enteredDevice->type == DeviceType::Mine && enteredDevice->ownerId != character->id)
        {
            const CharacterState* mineOwner = FindCharacter(enteredDevice->ownerId);
            if (mineOwner && mineOwner->team != character->team)
                TriggerDevice(enteredDevice->id, false, result.events);
        }
    }
    for (ModifierEffectState& suppression : character->modifierEffects)
    {
        if (suppression.kind != ModifierEffectKind::SuppressingFire || suppression.counterC != 0) continue;
        CharacterState* sniper = FindCharacter(suppression.causerId);
        if (!sniper || !sniper->alive) continue;
        suppression.counterC = 1;
        const AbilityDefinition& suppressing = definitions_.abilities.at(suppression.sourceAbilityId);
        for (AbilityState& ability : sniper->abilities)
        {
            if (ability.definitionId != suppression.sourceAbilityId) continue;
            const std::int32_t before = ability.cooldownRemaining;
            ability.cooldownRemaining = std::max(0, before - suppression.counterA);
            if (before != ability.cooldownRemaining) result.events.push_back(GameEvent{EventType::CooldownChanged,
                sniper->id, sniper->id, sniper->tileId, ability.definitionId, ability.cooldownRemaining - before});
        }
        if (suppression.valueC > 0 && sniper->resourceCurrent > 0)
        {
            const CharacterDefinition& sniperDefinition = definitions_.characters.at(sniper->definitionId);
            std::vector<GameEvent> aura, modifiers;
            std::int32_t reactionBase = static_cast<std::int32_t>(
                sniperDefinition.stats.attack * (suppression.valueC / 10000.0) + 0.5);
            if (RollBasisPoints(sniperDefinition.criticalChanceBasisPoints))
                reactionBase = static_cast<std::int32_t>(ApplyBasisPoints(
                    reactionBase, sniperDefinition.criticalDamageMultiplierBasisPoints));
            const std::int32_t damage = ResolveDamage(*sniper, *character, reactionBase, 0, aura, modifiers);
            result.events.push_back(GameEvent{EventType::DamageApplied, sniper->id, character->id, character->tileId,
                suppression.id + ":Reaction", damage});
            result.events.insert(result.events.end(), aura.begin(), aura.end()); result.events.insert(result.events.end(), modifiers.begin(), modifiers.end());
            const bool reactionKilled = character->health <= 0;
            ResolveAmmoShot(*sniper, reactionKilled, false, suppression.id + ":Reaction", result.events);
            if (reactionKilled)
            {
                character->health = 0; character->alive = false; destination->occupantId = InvalidEntityId;
                result.events.push_back(GameEvent{EventType::CharacterDied, sniper->id, character->id,
                    character->tileId, suppression.id + ":Reaction", 0});
            }
        }
        (void)suppressing;
    }
    if (!character->alive) return result;
    // Aim fields belong to their owner and react when an enemy enters their configured area.
    for (CharacterState& owner : state_.characters)
    {
        if (!owner.alive || owner.team == character->team) continue;
        for (auto field = owner.modifierEffects.begin(); field != owner.modifierEffects.end();)
        {
            if (field->kind != ModifierEffectKind::AimField) { ++field; continue; }
            const TileState* center = FindTile(field->areaCenterTileId);
            if (!center || HexDistance(center->coord, destination->coord) > field->areaRadius) { ++field; continue; }
            std::vector<GameEvent> auraEvents, modifierEvents;
            const AbilityDefinition& aim = definitions_.abilities.at(field->sourceAbilityId);
            std::int32_t triggerDamage = field->areaDamage;
            if (AbilityValue(aim, "BonusIfOwnerStayed") > 0.0 && owner.tileId == static_cast<EntityId>(field->valueC)
                && AbilityValue(aim, "StayedDamageMultiplier") > 0.0)
                triggerDamage = static_cast<std::int32_t>(triggerDamage * AbilityValue(aim, "StayedDamageMultiplier") + 0.5);
            const CharacterDefinition& movingCharacterDefinition = definitions_.characters.at(character->definitionId);
            if (AbilityValue(aim, "DamageMultiplierVsLowHealth") > 1.0
                && character->health * 100 <= movingCharacterDefinition.stats.maxHealth * 35)
                triggerDamage = static_cast<std::int32_t>(triggerDamage * AbilityValue(aim, "DamageMultiplierVsLowHealth") + 0.5);
            const std::int32_t damage = ResolveDamage(owner, *character, triggerDamage,
                field->valueA, auraEvents, modifierEvents);
            result.events.push_back(GameEvent{EventType::DamageApplied, owner.id, character->id,
                character->tileId, field->id, damage});
            result.events.insert(result.events.end(), auraEvents.begin(), auraEvents.end());
            result.events.insert(result.events.end(), modifierEvents.begin(), modifierEvents.end());
            if (field->valueB > 0)
            {
                const std::int32_t before = character->initiative;
                character->initiative = std::max(0, character->initiative - field->valueB);
                result.events.push_back(GameEvent{EventType::InitiativeChanged, owner.id, character->id,
                    character->tileId, field->id, character->initiative - before});
            }
            const std::int32_t extension = std::max(0, static_cast<std::int32_t>(AbilityValue(aim, "ExtendDebuffDurationInitiative")));
            if (extension > 0)
            {
                auto oldest = character->modifierEffects.end();
                for (auto candidate = character->modifierEffects.begin(); candidate != character->modifierEffects.end(); ++candidate)
                    if (candidate->isDebuff && (oldest == character->modifierEffects.end() || candidate->remainingInitiative < oldest->remainingInitiative)) oldest = candidate;
                if (oldest != character->modifierEffects.end()) oldest->remainingInitiative += extension;
            }
            if (character->health > 0 && AbilityValue(aim, "NextOutgoingDamageMultiplier") > 0.0)
            {
                ModifierEffectState weak; weak.id = field->id + ":Weak"; weak.causerId = owner.id;
                weak.sourceAbilityId = field->sourceAbilityId; weak.kind = ModifierEffectKind::AimWeakness;
                weak.remainingInitiative = 80; weak.outgoingDamageMultiplierBasisPoints = static_cast<std::int32_t>(
                    AbilityValue(aim, "NextOutgoingDamageMultiplier") * 10000.0 + 0.5);
                weak.consumeOnOutgoingDamage = true;
                character->modifierEffects.push_back(weak);
                result.events.push_back(GameEvent{EventType::EffectAdded, owner.id, character->id,
                    character->tileId, weak.id, weak.remainingInitiative});
            }
            if (character->health > 0 && AbilityValue(aim, "MarkDamageMultiplier") > 0.0)
            {
                ModifierEffectState mark; mark.id = field->id + ":Mark"; mark.causerId = owner.id; mark.isDebuff = true;
                mark.sourceAbilityId = field->sourceAbilityId; mark.kind = ModifierEffectKind::AimMark;
                mark.incomingDamageMultiplierFromCauserBasisPoints = static_cast<std::int32_t>(AbilityValue(aim, "MarkDamageMultiplier") * 10000.0 + 0.5);
                mark.consumeOnDamageFromCauser = true; mark.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(aim, "MarkDurationInitiative")));
                character->modifierEffects.push_back(mark);
                result.events.push_back(GameEvent{EventType::EffectAdded, owner.id, character->id,
                    character->tileId, mark.id, mark.remainingInitiative});
            }
            if (character->health <= 0)
            {
                character->health = 0; character->alive = false; destination->occupantId = InvalidEntityId;
                owner.resourceCurrent = std::min(owner.resourceMaximum, owner.resourceCurrent + std::max(0, static_cast<std::int32_t>(AbilityValue(aim, "AmmoOnKill"))));
                for (AbilityState& ability : owner.abilities) if (ability.definitionId == field->sourceAbilityId)
                {
                    const std::int32_t before = ability.cooldownRemaining;
                    ability.cooldownRemaining = std::max(0, before - std::max(0, static_cast<std::int32_t>(AbilityValue(aim, "CooldownReductionOnKill"))));
                    if (before != ability.cooldownRemaining) result.events.push_back(GameEvent{EventType::CooldownChanged, owner.id, owner.id,
                        owner.tileId, ability.definitionId, ability.cooldownRemaining - before});
                }
                result.events.push_back(GameEvent{EventType::CharacterDied, owner.id, character->id, character->tileId, field->id, 0});
            }
            else
            {
                if (AbilityValue(aim, "CoverFire") > 0.0 && owner.resourceCurrent > 0)
                {
                    const CharacterDefinition& ownerDefinition = definitions_.characters.at(owner.definitionId);
                    std::vector<GameEvent> coverAura, coverModifiers;
                    std::int32_t coverBase = static_cast<std::int32_t>(
                        ownerDefinition.stats.attack * AbilityValue(aim, "CoverFireDamageMultiplier") + 0.5);
                    if (RollBasisPoints(ownerDefinition.criticalChanceBasisPoints))
                        coverBase = static_cast<std::int32_t>(ApplyBasisPoints(
                            coverBase, ownerDefinition.criticalDamageMultiplierBasisPoints));
                    const std::int32_t coverDamage = ResolveDamage(owner, *character, coverBase, 0, coverAura, coverModifiers);
                    result.events.push_back(GameEvent{EventType::DamageApplied, owner.id, character->id, character->tileId, field->id + ":CoverFire", coverDamage});
                    result.events.insert(result.events.end(), coverAura.begin(), coverAura.end()); result.events.insert(result.events.end(), coverModifiers.begin(), coverModifiers.end());
                    const bool coverKilled = character->health <= 0;
                    ResolveAmmoShot(owner, coverKilled, false, field->id + ":CoverFire", result.events);
                    if (coverKilled)
                    {
                        character->health = 0; character->alive = false; destination->occupantId = InvalidEntityId;
                        result.events.push_back(GameEvent{EventType::CharacterDied, owner.id, character->id,
                            character->tileId, field->id + ":CoverFire", 0});
                    }
                }
                if (character->alive && AbilityValue(aim, "ApplyTrackingMark") > 0.0)
                {
                    ModifierEffectState tracking; tracking.id = field->id + ":Tracking"; tracking.sourceAbilityId = field->sourceAbilityId;
                    tracking.kind = ModifierEffectKind::TrackingMark; tracking.causerId = owner.id; tracking.isDebuff = true;
                    tracking.triggerOnMove = true; tracking.triggerDamageOnMove = static_cast<std::int32_t>(
                        field->areaDamage * AbilityValue(aim, "TrackingDamageMultiplier") + 0.5);
                    tracking.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(aim, "TrackingDurationInitiative")));
                    character->modifierEffects.push_back(tracking);
                    result.events.push_back(GameEvent{EventType::EffectAdded, owner.id, character->id,
                        character->tileId, tracking.id, tracking.remainingInitiative});
                }
            }
            if (field->counterA > 0) { --field->counterA; field->areaDamage /= 2; ++field; }
            else { result.events.push_back(GameEvent{EventType::EffectRemoved, owner.id, owner.id, field->areaCenterTileId, field->id, 0}); field = owner.modifierEffects.erase(field); }
            break;
        }
    }
    std::vector<ModifierEffectState> moveTriggers;
    for (const ModifierEffectState& effect : character->modifierEffects)
    {
        if (effect.triggerOnMove) moveTriggers.push_back(effect);
    }
    for (const ModifierEffectState& trigger : moveTriggers)
    {
        CharacterState* causer = FindCharacter(trigger.causerId);
        if (trigger.kind == ModifierEffectKind::RendBleed)
        {
            for (ModifierEffectState& effect : character->modifierEffects) if (effect.id == trigger.id) effect.triggerOnMove = false;
        }
        else
        {
            character->modifierEffects.erase(std::remove_if(character->modifierEffects.begin(), character->modifierEffects.end(),
                [&trigger](const ModifierEffectState& effect) { return effect.id == trigger.id; }), character->modifierEffects.end());
            result.events.push_back(GameEvent{EventType::EffectRemoved, trigger.causerId, character->id,
                character->tileId, trigger.id, 0});
        }
        if (!causer || !causer->alive) continue;
        if (trigger.triggerDamageOnMove > 0)
        {
            std::vector<GameEvent> auraEvents;
            std::vector<GameEvent> modifierEvents;
            const std::int32_t damage = ResolveDamage(*causer, *character, trigger.triggerDamageOnMove, 0,
                auraEvents, modifierEvents);
            result.events.push_back(GameEvent{EventType::DamageApplied, causer->id, character->id,
                character->tileId, trigger.id, damage});
            result.events.insert(result.events.end(), auraEvents.begin(), auraEvents.end());
            result.events.insert(result.events.end(), modifierEvents.begin(), modifierEvents.end());
        }
        const CharacterDefinition& targetDefinition = definitions_.characters.at(character->definitionId);
        const double delayMultiplier = PassiveValue(targetDefinition, "DelayReceivedMultiplier");
        const auto loseInitiative = [&result, causer, delayMultiplier, &trigger](CharacterState& target, const std::int32_t requested)
        {
            const std::int32_t loss = delayMultiplier > 0.0
                ? std::max(0, static_cast<std::int32_t>(requested * delayMultiplier + 0.5)) : std::max(0, requested);
            const std::int32_t previous = target.initiative;
            target.initiative = std::max(0, target.initiative - loss);
            if (target.initiative != previous) result.events.push_back(GameEvent{EventType::InitiativeChanged,
                causer->id, target.id, target.tileId, trigger.id, target.initiative - previous});
        };
        loseInitiative(*character, trigger.triggerDelayOnMove);
        if (trigger.triggerAreaRadius > 0 && trigger.triggerAreaInitiativeLoss > 0)
        {
            const TileState* center = FindTile(character->tileId);
            for (CharacterState& other : state_.characters)
            {
                const TileState* tile = FindTile(other.tileId);
                if (center && tile && other.alive && other.team != causer->team && other.id != character->id
                    && HexDistance(center->coord, tile->coord) <= trigger.triggerAreaRadius)
                {
                    loseInitiative(other, trigger.triggerAreaInitiativeLoss);
                }
            }
        }
        if (trigger.triggerOutgoingDamageMultiplierBasisPoints != 10000 && character->health > 0)
        {
            ModifierEffectState weakness; weakness.id = trigger.id + ":Weakness"; weakness.causerId = causer->id;
            weakness.remainingInitiative = 40; weakness.isDebuff = true;
            weakness.outgoingDamageMultiplierBasisPoints = trigger.triggerOutgoingDamageMultiplierBasisPoints;
            character->modifierEffects.push_back(weakness);
            result.events.push_back(GameEvent{EventType::EffectAdded, causer->id, character->id,
                character->tileId, weakness.id, weakness.remainingInitiative});
        }
        if (trigger.triggerSlowDurationInitiative > 0 && character->health > 0)
        {
            ModifierEffectState slow; slow.id = trigger.id + ":Slow"; slow.causerId = causer->id;
            slow.remainingInitiative = trigger.triggerSlowDurationInitiative; slow.isDebuff = true;
            slow.moveCostMultiplierBasisPoints = trigger.triggerSlowMultiplierBasisPoints;
            character->modifierEffects.push_back(slow);
            result.events.push_back(GameEvent{EventType::EffectAdded, causer->id, character->id,
                character->tileId, slow.id, slow.remainingInitiative});
        }
        if (trigger.triggerInitiativeGainForCauser > 0)
        {
            causer->initiative += trigger.triggerInitiativeGainForCauser;
            result.events.push_back(GameEvent{EventType::InitiativeChanged, causer->id, causer->id,
                causer->tileId, trigger.id, trigger.triggerInitiativeGainForCauser});
        }
        if (character->health <= 0)
        {
            character->health = 0; character->alive = false; destination->occupantId = InvalidEntityId;
            result.events.push_back(GameEvent{EventType::CharacterDied, causer->id, character->id,
                character->tileId, trigger.id, 0});
            break;
        }
    }
    for (auto effect = character->modifierEffects.begin(); effect != character->modifierEffects.end();)
    {
        effect->firstMovePenaltyAvailable = false;
        if (effect->maximumMovedFields > 0)
        {
            if (effect->movedFields < std::numeric_limits<std::int32_t>::max())
            {
                ++effect->movedFields;
            }
        }
        if (effect->maximumMovedFields > 0 && effect->movedFields >= effect->maximumMovedFields)
        {
            result.events.push_back(GameEvent{EventType::EffectRemoved, character->id, character->id,
                destination->id, effect->id, 0});
            effect = character->modifierEffects.erase(effect);
        }
        else
        {
            ++effect;
        }
    }
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
    if (action.type == ActionType::UseAbility)
    {
        return ValidateAbility(action);
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

DecisionSnapshot Game::ObserveTeamDecision(const Team perspective) const
{
    DecisionSnapshot snapshot;
    snapshot.state = state_;
    snapshot.state.matchId = 0;
    snapshot.state.seed = 0;
    snapshot.state.randomState = 0;

    const std::vector<EntityId> visibleTileIds = TeamVisibleTileIds(perspective);
    const auto isVisibleTile = [&visibleTileIds](const EntityId tileId)
    {
        return std::binary_search(visibleTileIds.begin(), visibleTileIds.end(), tileId);
    };
    snapshot.state.tiles.erase(
        std::remove_if(snapshot.state.tiles.begin(), snapshot.state.tiles.end(),
            [&isVisibleTile](const TileState& tile) { return !isVisibleTile(tile.id); }),
        snapshot.state.tiles.end());
    snapshot.state.characters.erase(
        std::remove_if(snapshot.state.characters.begin(), snapshot.state.characters.end(),
            [perspective, &isVisibleTile](const CharacterState& character)
            {
                return character.team != perspective && !isVisibleTile(character.tileId);
            }),
        snapshot.state.characters.end());
    snapshot.state.devices.erase(
        std::remove_if(snapshot.state.devices.begin(), snapshot.state.devices.end(),
            [this, perspective, &isVisibleTile](const DeviceState& device)
            {
                const CharacterState* owner = FindCharacter(device.ownerId);
                if (owner && owner->team == perspective) return false;
                if (!isVisibleTile(device.tileId)) return true;
                return device.type == DeviceType::Mine && device.enemyRevealInitiative <= 0;
            }),
        snapshot.state.devices.end());
    std::unordered_set<EntityId> visibleCharacterIds;
    for (const CharacterState& character : snapshot.state.characters)
    {
        visibleCharacterIds.insert(character.id);
    }
    for (CharacterState& character : snapshot.state.characters)
    {
        for (ModifierEffectState& effect : character.modifierEffects)
        {
            if (visibleCharacterIds.find(effect.causerId) == visibleCharacterIds.end())
            {
                effect.causerId = InvalidEntityId;
            }
        }
    }

    const auto active = std::find_if(snapshot.state.characters.begin(), snapshot.state.characters.end(),
        [this](const CharacterState& character) { return character.id == state_.activeCharacterId; });
    if (active == snapshot.state.characters.end())
    {
        snapshot.state.activeCharacterId = InvalidEntityId;
    }
    if (!IsTerminal() && active != snapshot.state.characters.end() && active->team == perspective)
    {
        snapshot.legalActions = LegalActions(active->id);
    }
    return snapshot;
}

NumericObservation Game::ObserveNumeric(const ObservationSchema& schema) const
{
    if (schema.schemaVersion != definitions_.schemaVersion
        || schema.contentVersion != definitions_.contentVersion)
    {
        throw std::invalid_argument("Observation schema does not match the loaded Core content package.");
    }
    const DecisionSnapshot observation = ObserveDecision();
    return EncodeOmniscientObservation(observation.state, observation.legalActions, schema);
}

NumericObservation Game::ObserveTeamNumeric(const Team perspective, const ObservationSchema& schema) const
{
    if (schema.schemaVersion != definitions_.schemaVersion
        || schema.contentVersion != definitions_.contentVersion)
    {
        throw std::invalid_argument("Observation schema does not match the loaded Core content package.");
    }
    const DecisionSnapshot observation = ObserveTeamDecision(perspective);
    return EncodeOmniscientObservation(observation.state, observation.legalActions, schema);
}

Game Game::Clone() const
{
    return *this;
}

std::int32_t Game::ResolveDamage(CharacterState& source, CharacterState& target, std::int64_t damage,
    std::int32_t defenseIgnore, std::vector<GameEvent>& auraEvents,
    std::vector<GameEvent>& modifierEvents)
{
    const CharacterDefinition& sourceDefinition = definitions_.characters.at(source.definitionId);
    const CharacterDefinition& targetDefinition = definitions_.characters.at(target.definitionId);
    const TileState* initialSourceTile = FindTile(source.tileId);
    const TileState* initialTargetTile = FindTile(target.tileId);
    if (PassiveValue(sourceDefinition, "CoreAmmoCharacter") > 0.0)
    {
        if (!source.movedSinceLastTurn && source.stationaryShotStacks > 0)
            damage = static_cast<std::int64_t>(damage * (1.0 + PassiveValue(sourceDefinition,
                "StationaryShotDamagePerStack") * source.stationaryShotStacks) + 0.5);
        if (source.resourceCurrent == 1)
        {
            const double multiplier = PassiveValue(sourceDefinition, "AllDamageMultiplierWhileOneAmmo");
            if (multiplier > 0.0) damage = static_cast<std::int64_t>(damage * multiplier + 0.5);
        }
    }
    if (PassiveValue(sourceDefinition, "CoreComboCharacter") > 0.0 && source.nextDamageAfterMoveReady)
    {
        const double multiplier = PassiveValue(sourceDefinition, "NextHitDamageMultiplierAfterMove");
        if (multiplier > 0.0) damage = static_cast<std::int64_t>(damage * multiplier + 0.5);
    }
    if (PassiveValue(sourceDefinition, "CoreBloodCharacter") > 0.0)
    {
        const double healthFraction = sourceDefinition.stats.maxHealth > 0
            ? static_cast<double>(source.health) / sourceDefinition.stats.maxHealth : 1.0;
        const double lowThreshold = PassiveValue(sourceDefinition, "LowHealthThresholdPercent") / 100.0;
        const double lowMultiplier = PassiveValue(sourceDefinition, "DamageMultiplierWhileLowHealth");
        if (lowThreshold > 0.0 && healthFraction <= lowThreshold && lowMultiplier > 0.0)
            damage = static_cast<std::int64_t>(damage * lowMultiplier + 0.5);
        const std::int32_t bloodThreshold = std::max(0, static_cast<std::int32_t>(PassiveValue(sourceDefinition, "HighBloodDamageThreshold")));
        const double bloodMultiplier = PassiveValue(sourceDefinition, "HighBloodDamageMultiplier");
        if (bloodThreshold > 0 && source.resourceCurrent >= bloodThreshold && bloodMultiplier > 0.0)
            damage = static_cast<std::int64_t>(damage * bloodMultiplier + 0.5);
    }
    if (initialSourceTile && initialTargetTile && HexDistance(initialSourceTile->coord, initialTargetTile->coord) > 1)
    {
        const double multiplier = PassiveValue(targetDefinition, "RangedDamageMultiplier");
        if (multiplier > 0.0) damage = static_cast<std::int64_t>(damage * multiplier + 0.5);
    }
    std::vector<DefinitionId> consumedOutgoingEffects;
    for (const ModifierEffectState& effect : source.modifierEffects)
    {
        if (effect.onlyAffectCauserDamage && effect.causerId != target.id)
        {
            continue;
        }
        defenseIgnore = std::max(0, defenseIgnore + effect.defenseIgnoreAdd);
        damage = ApplyBasisPoints(damage, effect.outgoingDamageMultiplierBasisPoints);
        if (effect.consumeOnOutgoingDamage && damage > 0) consumedOutgoingEffects.push_back(effect.id);
        if (effect.preferredTargetId != InvalidEntityId)
        {
            if (target.id != effect.preferredTargetId)
            {
                damage = ApplyBasisPoints(damage, effect.outgoingDamageMultiplierAgainstOtherTargetsBasisPoints);
                if (effect.initiativeLossWhenIgnoringPreferredTarget > 0)
                {
                    const std::int32_t previous = source.initiative;
                    source.initiative = std::max(0, source.initiative - effect.initiativeLossWhenIgnoringPreferredTarget);
                    modifierEvents.push_back(GameEvent{EventType::InitiativeChanged, effect.causerId, source.id,
                        source.tileId, effect.id, source.initiative - previous});
                }
            }
            else if (effect.initiativeGainForCauserWhenAttacked > 0)
            {
                if (CharacterState* causer = FindCharacter(effect.causerId); causer && causer->alive)
                {
                    causer->initiative += effect.initiativeGainForCauserWhenAttacked;
                    modifierEvents.push_back(GameEvent{EventType::InitiativeChanged, causer->id, causer->id,
                        causer->tileId, effect.id, effect.initiativeGainForCauserWhenAttacked});
                }
            }
        }
        const std::int32_t heal = static_cast<std::int32_t>(ApplyBasisPoints(
            damage, effect.healFractionFromDamageDealtBasisPoints));
        const std::int32_t appliedHeal = std::min(
            heal, std::max(0, sourceDefinition.stats.maxHealth - source.health));
        source.health += appliedHeal;
        if (appliedHeal > 0)
        {
            modifierEvents.push_back(GameEvent{
                EventType::HealingApplied, source.id, source.id, source.tileId, effect.id, appliedHeal});
        }
    }
    for (const DefinitionId& effectId : consumedOutgoingEffects)
    {
        source.modifierEffects.erase(std::remove_if(source.modifierEffects.begin(), source.modifierEffects.end(),
            [&effectId](const ModifierEffectState& effect) { return effect.id == effectId; }), source.modifierEffects.end());
        modifierEvents.push_back(GameEvent{EventType::EffectRemoved, source.id, source.id,
            source.tileId, effectId, 0});
    }

    if (target.resourceCurrent > 0)
    {
        const double multiplier = PassiveValue(targetDefinition, "DamageMultiplierWhileHasChain");
        if (multiplier > 0.0) damage = static_cast<std::int64_t>(damage * multiplier + 0.5);
    }
    if (target.stationaryGuardActive)
    {
        const double multiplier = PassiveValue(targetDefinition, "StationaryIncomingDamageMultiplier");
        if (multiplier > 0.0) damage = static_cast<std::int64_t>(damage * multiplier + 0.5);
    }
    if (PassiveValue(targetDefinition, "CoreComboCharacter") > 0.0 && target.nextReductionAfterMoveReady)
    {
        const double multiplier = PassiveValue(targetDefinition, "NextIncomingDamageMultiplierAfterMove");
        if (multiplier > 0.0) damage = static_cast<std::int64_t>(damage * multiplier + 0.5);
    }
    if (PassiveValue(targetDefinition, "CoreBloodCharacter") > 0.0)
    {
        if (target.firstBloodDamageReductionAvailable)
        {
            const double multiplier = PassiveValue(targetDefinition, "FirstIncomingDamageMultiplierPerTurn");
            if (multiplier > 0.0 && multiplier < 1.0) { damage = static_cast<std::int64_t>(damage * multiplier + 0.5); target.firstBloodDamageReductionAvailable = false; }
        }
        const double fraction = targetDefinition.stats.maxHealth > 0 ? static_cast<double>(target.health) / targetDefinition.stats.maxHealth : 1.0;
        const double threshold = PassiveValue(targetDefinition, "LowHealthThresholdPercent") / 100.0;
        const double multiplier = PassiveValue(targetDefinition, "IncomingDamageMultiplierWhileLowHealth");
        if (threshold > 0.0 && fraction <= threshold && multiplier > 0.0) damage = static_cast<std::int64_t>(damage * multiplier + 0.5);
    }
    const bool sourceDebuffedByTarget = std::any_of(source.modifierEffects.begin(), source.modifierEffects.end(),
        [&target, &source](const ModifierEffectState& effect) { return effect.causerId == target.id && effect.causerId != source.id; });
    if (sourceDebuffedByTarget)
    {
        const double multiplier = PassiveValue(targetDefinition, "IncomingDamageMultiplierFromDebuffedEnemies");
        if (multiplier > 0.0) damage = static_cast<std::int64_t>(damage * multiplier + 0.5);
    }

    std::vector<DefinitionId> consumedDamageEffects;
    std::int32_t shadowWoundComboGain = 0;
    for (const ModifierEffectState& effect : target.modifierEffects)
    {
        if (effect.redirectPreMitigationBasisPoints <= 0 || effect.causerId == target.id)
        {
            continue;
        }
        CharacterState* protector = FindCharacter(effect.causerId);
        if (!protector || !protector->alive)
        {
            continue;
        }
        const std::int32_t redirected = static_cast<std::int32_t>(std::min<std::int64_t>(damage,
            ApplyBasisPoints(damage, std::clamp(effect.redirectPreMitigationBasisPoints, 0, 10000))));
        if (redirected <= 0)
        {
            continue;
        }
        damage -= redirected;
        const std::int32_t protectorDamage = std::min(protector->health, static_cast<std::int32_t>(
            ApplyBasisPoints(redirected, effect.redirectedDamageMultiplierBasisPoints)));
        protector->health -= protectorDamage;
        modifierEvents.push_back(GameEvent{EventType::DamageApplied, source.id, protector->id,
            protector->tileId, effect.id, protectorDamage});
        const CharacterDefinition& protectorDefinition = definitions_.characters.at(protector->definitionId);
        const std::int32_t requestedHeal = static_cast<std::int32_t>(
            ApplyBasisPoints(redirected, effect.protectorHealFractionBasisPoints));
        const std::int32_t appliedHeal = std::min(requestedHeal,
            std::max(0, protectorDefinition.stats.maxHealth - protector->health));
        protector->health += appliedHeal;
        if (appliedHeal > 0)
        {
            modifierEvents.push_back(GameEvent{EventType::HealingApplied, protector->id, protector->id,
                protector->tileId, effect.id, appliedHeal});
        }
        if (effect.protectedInitiativePerHit > 0)
        {
            target.initiative += effect.protectedInitiativePerHit;
            modifierEvents.push_back(GameEvent{EventType::InitiativeChanged, protector->id, target.id,
                target.tileId, effect.id, effect.protectedInitiativePerHit});
        }
        if (protector->health <= 0)
        {
            protector->alive = false;
            if (TileState* tile = FindTile(protector->tileId)) tile->occupantId = InvalidEntityId;
            modifierEvents.push_back(GameEvent{EventType::CharacterDied, source.id, protector->id,
                protector->tileId, effect.id, 0});
        }
        break;
    }

    const TileState* protectedTargetTile = FindTile(target.tileId);
    for (CharacterState& auraOwner : state_.characters)
    {
        if (!auraOwner.alive || auraOwner.team != target.team || !protectedTargetTile)
        {
            continue;
        }
        const auto auraDefinitionEntry = definitions_.characters.find(auraOwner.definitionId);
        const TileState* auraTile = FindTile(auraOwner.tileId);
        if (auraDefinitionEntry == definitions_.characters.end() || !auraTile)
        {
            continue;
        }

        const CharacterDefinition& auraDefinition = auraDefinitionEntry->second;
        const double reduction = std::clamp(
            PassiveValue(auraDefinition, "CoreIncomingDamageAuraReduction"), 0.0, 0.95);
        const std::int32_t radius = std::max(
            0, static_cast<std::int32_t>(PassiveValue(auraDefinition, "CoreIncomingDamageAuraRadius") + 0.5));
        const bool affectsSelf = PassiveValue(auraDefinition, "CoreIncomingDamageAuraAffectSelf") > 0.0;
        if (reduction <= 0.0 || (auraOwner.id == target.id && !affectsSelf)
            || HexDistance(auraTile->coord, protectedTargetTile->coord) > radius)
        {
            continue;
        }

        const std::int64_t previousDamage = damage;
        damage = static_cast<std::int64_t>(static_cast<double>(damage) * (1.0 - reduction) + 0.5);
        const std::int32_t preventedDamage = static_cast<std::int32_t>(
            std::max<std::int64_t>(0, previousDamage - damage));
        if (preventedDamage <= 0)
        {
            continue;
        }

        auto recentPrevention = std::find_if(auraOwner.recentAuraPreventions.begin(),
            auraOwner.recentAuraPreventions.end(),
            [&source](const RecentAuraPreventionState& state) { return state.sourceId == source.id; });
        if (recentPrevention == auraOwner.recentAuraPreventions.end())
        {
            auraOwner.recentAuraPreventions.push_back({source.id, state_.globalInitiative});
        }
        else
        {
            recentPrevention->globalInitiative = state_.globalInitiative;
        }

        const std::int32_t heal = std::max(0, static_cast<std::int32_t>(
            preventedDamage * PassiveValue(auraDefinition, "CoreAuraHealFractionOfPreventedDamage") + 0.5));
        const std::int32_t appliedHeal = std::min(
            heal, std::max(0, auraDefinition.stats.maxHealth - auraOwner.health));
        auraOwner.health += appliedHeal;
        if (appliedHeal > 0)
        {
            auraEvents.push_back(GameEvent{EventType::HealingApplied, auraOwner.id, auraOwner.id,
                auraOwner.tileId, "IncomingDamageAura", appliedHeal});
        }
        const std::int32_t initiativeGain = std::max(0, static_cast<std::int32_t>(
            PassiveValue(auraDefinition, "CoreAuraInitiativeOnPrevent") + 0.5));
        auraOwner.initiative += initiativeGain;
        if (initiativeGain > 0)
        {
            auraEvents.push_back(GameEvent{EventType::InitiativeChanged, auraOwner.id, auraOwner.id,
                auraOwner.tileId, "IncomingDamageAura", initiativeGain});
        }
        const std::int32_t cooldownReduction = std::max(0, static_cast<std::int32_t>(
            PassiveValue(auraDefinition, "CoreAuraCooldownReductionOnPrevent") + 0.5));
        for (AbilityState& ability : auraOwner.abilities)
        {
            const std::int32_t previousCooldown = ability.cooldownRemaining;
            ability.cooldownRemaining = std::max(0, previousCooldown - cooldownReduction);
            if (ability.cooldownRemaining != previousCooldown)
            {
                auraEvents.push_back(GameEvent{EventType::CooldownChanged, auraOwner.id, auraOwner.id,
                    auraOwner.tileId, ability.definitionId, ability.cooldownRemaining - previousCooldown});
            }
        }
    }

    const TileState* sourceTile = FindTile(source.tileId);
    for (const ModifierEffectState& effect : target.modifierEffects)
    {
        if (effect.onlyAffectCauserDamage && effect.causerId != source.id)
        {
            continue;
        }
        if (effect.onlyAgainstRangedSources && sourceTile && protectedTargetTile
            && HexDistance(sourceTile->coord, protectedTargetTile->coord) <= 1)
        {
            continue;
        }
        if (effect.kind != ModifierEffectKind::DeathMark
            && effect.kind != ModifierEffectKind::SharedBreach)
            defenseIgnore = std::max(0, defenseIgnore + effect.defenseIgnoreAdd);
        if (effect.causerId == source.id)
        {
            if (effect.kind == ModifierEffectKind::DeathMark)
                defenseIgnore = std::max(0, defenseIgnore + effect.defenseIgnoreAdd);
            damage = ApplyBasisPoints(damage, effect.incomingDamageMultiplierFromCauserBasisPoints);
            if (effect.kind == ModifierEffectKind::DeathMark
                && effect.triggerSlowMultiplierBasisPoints > 0
                && target.health * 10000 <= targetDefinition.stats.maxHealth * effect.triggerSlowMultiplierBasisPoints)
                damage = ApplyBasisPoints(damage, 10000 + effect.triggerOutgoingDamageMultiplierBasisPoints);
            if (effect.kind == ModifierEffectKind::ShadowWound)
            {
                damage += std::max(0, effect.valueA);
                shadowWoundComboGain = std::max(shadowWoundComboGain, std::max(0, effect.valueB));
            }
        }
        if (effect.kind == ModifierEffectKind::SharedBreach)
        {
            const CharacterState* breachOwner = FindCharacter(effect.causerId);
            const std::uint32_t sourceBit = source.id > 0 && source.id < 31
                ? (1u << static_cast<std::uint32_t>(source.id)) : 0u;
            const bool unused = sourceBit != 0u
                ? (effect.flags & sourceBit) == 0u : effect.counterA == 0;
            if (breachOwner && breachOwner->team == source.team && unused)
                defenseIgnore += std::max(0, effect.valueA);
        }
        damage = ApplyBasisPoints(damage, effect.incomingDamageMultiplierBasisPoints);
        if (effect.consumeOnDamageFromCauser && effect.causerId == source.id)
            consumedDamageEffects.push_back(effect.id);
        if (effect.kind == ModifierEffectKind::AssassinFollowup && (effect.flags & 2u) != 0u)
            consumedDamageEffects.push_back(effect.id);
        if (effect.kind == ModifierEffectKind::OneHitProtection)
            consumedDamageEffects.push_back(effect.id);
    }
    for (const DefinitionId& effectId : consumedDamageEffects)
    {
        target.modifierEffects.erase(std::remove_if(target.modifierEffects.begin(), target.modifierEffects.end(),
            [&effectId](const ModifierEffectState& effect) { return effect.id == effectId; }), target.modifierEffects.end());
        modifierEvents.push_back(GameEvent{EventType::EffectRemoved, source.id, target.id,
            target.tileId, effectId, 0});
    }

    const std::int32_t resourceDefense = target.resourceCurrent <= 0
        ? std::max(0, static_cast<std::int32_t>(PassiveValue(targetDefinition, "DefenseWhileNoChain")))
        : 0;
    const std::int32_t heldDefense = target.heldPositionFromPreviousTurn && !target.movedSinceLastTurn
        ? std::max(0, static_cast<std::int32_t>(PassiveValue(targetDefinition, "DefenseAddWhileHoldingPosition"))) : 0;
    const std::int32_t elementDefense = target.activeElement != 0
        ? std::max(0, static_cast<std::int32_t>(PassiveValue(targetDefinition, "DefenseWhileElementStored"))) : 0;
    const std::int32_t bloodDefenseThreshold = std::max(0, static_cast<std::int32_t>(PassiveValue(targetDefinition, "BloodDefenseThreshold")));
    const std::int32_t bloodDefense = bloodDefenseThreshold > 0 && target.resourceCurrent >= bloodDefenseThreshold
        ? std::max(0, static_cast<std::int32_t>(PassiveValue(targetDefinition, "DefenseWhileHighBlood"))) : 0;
    std::int32_t deviceDefense = 0;
    const std::int32_t defensePerDevice = std::max(0, static_cast<std::int32_t>(PassiveValue(targetDefinition, "DefensePerActiveDevice")));
    if (defensePerDevice > 0)
    {
        const std::int32_t count = static_cast<std::int32_t>(std::count_if(state_.devices.begin(), state_.devices.end(),
            [&target](const DeviceState& device) { return device.ownerId == target.id; }));
        const std::int32_t maximum = std::max(0, static_cast<std::int32_t>(PassiveValue(targetDefinition, "MaxDefenseFromDevices")));
        deviceDefense += maximum > 0 ? std::min(maximum, count * defensePerDevice) : count * defensePerDevice;
    }
    const std::int32_t networkRange = std::max(0, static_cast<std::int32_t>(PassiveValue(targetDefinition, "DeviceNetworkDefenseRange")));
    if (networkRange > 0 && protectedTargetTile && std::any_of(state_.devices.begin(), state_.devices.end(),
        [this, &target, protectedTargetTile, networkRange](const DeviceState& device)
        {
            const TileState* tile = FindTile(device.tileId);
            return device.ownerId == target.id && tile && HexDistance(protectedTargetTile->coord, tile->coord) <= networkRange;
        }))
        deviceDefense += std::max(0, static_cast<std::int32_t>(PassiveValue(targetDefinition, "DefenseNearDevice")));
    const std::int64_t defense = std::max(0, targetDefinition.stats.defense + resourceDefense + heldDefense + bloodDefense
        + elementDefense + target.rockDefenseBonus + deviceDefense - defenseIgnore);
    const std::int64_t denominator = 100 + defense;
    std::int32_t finalDamage = static_cast<std::int32_t>(std::min<std::int64_t>(
        std::numeric_limits<std::int32_t>::max(), (damage * 100 + denominator / 2) / denominator));
    for (auto effect = target.modifierEffects.begin(); effect != target.modifierEffects.end() && finalDamage > 0;)
    {
        if (effect->kind != ModifierEffectKind::EngineerShield || effect->valueA <= 0)
        {
            ++effect;
            continue;
        }
        const std::int32_t absorbed = std::min(finalDamage, effect->valueA);
        finalDamage -= absorbed;
        effect->valueA -= absorbed;
        if (effect->valueA <= 0)
        {
            modifierEvents.push_back(GameEvent{EventType::EffectRemoved, effect->causerId, target.id,
                target.tileId, effect->id, absorbed});
            effect = target.modifierEffects.erase(effect);
        }
        else ++effect;
    }
    const std::int32_t appliedDamage = std::min(target.health, finalDamage);
    target.health -= appliedDamage;
    if (appliedDamage > 0)
    {
        if (PassiveValue(sourceDefinition, "CoreBloodCharacter") > 0.0)
            GainBlood(source, appliedDamage, false, "DamageDealt", modifierEvents);
        if (PassiveValue(targetDefinition, "CoreBloodCharacter") > 0.0)
        {
            const std::int32_t rawGain = std::max(0, static_cast<std::int32_t>(appliedDamage
                * PassiveValue(targetDefinition, "BloodGainFractionFromDamageTaken") + 0.5));
            GainBlood(target, rawGain, true, "DamageTaken", modifierEvents);
        }
        if (shadowWoundComboGain > 0)
            GainCombo(source, shadowWoundComboGain, "ShadowWound", modifierEvents);
        if (PassiveValue(targetDefinition, "CoreComboCharacter") > 0.0)
            target.nextReductionAfterMoveReady = false;
        for (ModifierEffectState& effect : target.modifierEffects)
        {
            if (effect.kind == ModifierEffectKind::SharedBreach)
            {
                const CharacterState* breachOwner = FindCharacter(effect.causerId);
                if (!breachOwner || breachOwner->team != source.team) continue;
                const std::uint32_t sourceBit = source.id > 0 && source.id < 31
                    ? (1u << static_cast<std::uint32_t>(source.id)) : 0u;
                if (sourceBit != 0u) effect.flags |= sourceBit;
                else effect.counterA = 1;
            }
            if (effect.causerId != source.id) continue;
            if (effect.kind == ModifierEffectKind::DeathMark)
            {
                const bool repeated = (effect.flags & 2u) != 0u;
                const bool canGrant = (effect.flags & 1u) == 0u
                    || (repeated && effect.counterB != source.turnSequence);
                if (canGrant)
                {
                    const std::int32_t before = source.resourceCurrent;
                    GainCombo(source, std::max(0, effect.counterA), effect.id, modifierEvents);
                    effect.flags |= 1u; effect.counterB = source.turnSequence;
                    if (before < source.resourceMaximum && source.resourceCurrent >= source.resourceMaximum
                        && effect.valueB > 0)
                    {
                        for (AbilityState& ability : source.abilities)
                        {
                            const auto definition = definitions_.abilities.find(ability.definitionId);
                            if (definition == definitions_.abilities.end()
                                || definition->second.operation != AbilityOperation::Execute) continue;
                            const std::int32_t previous = ability.cooldownRemaining;
                            ability.cooldownRemaining = std::max(0, previous - effect.valueB);
                            if (previous != ability.cooldownRemaining) modifierEvents.push_back(GameEvent{
                                EventType::CooldownChanged, source.id, source.id, source.tileId,
                                ability.definitionId, ability.cooldownRemaining - previous});
                        }
                    }
                }
            }
        }
        for (ModifierEffectState& effect : target.modifierEffects)
        {
            if (effect.kind != ModifierEffectKind::SuppressingFire) continue;
            effect.preventsMovement = false;
            CharacterState* sniper = FindCharacter(effect.causerId);
            if (!sniper) continue;
            const AbilityDefinition& suppressing = definitions_.abilities.at(effect.sourceAbilityId);
            const std::uint32_t allyBit = source.id > 0 && source.id < 31
                ? (1u << static_cast<std::uint32_t>(source.id)) : 0u;
            const bool firstHitByThisAlly = allyBit != 0u
                ? (static_cast<std::uint32_t>(effect.counterB) & allyBit) == 0u
                : effect.counterB == 0;
            if (source.id != sniper->id && source.team == sniper->team && firstHitByThisAlly)
            {
                effect.counterB = allyBit != 0u
                    ? static_cast<std::int32_t>(static_cast<std::uint32_t>(effect.counterB) | allyBit)
                    : 1;
                effect.remainingInitiative += std::max(0, static_cast<std::int32_t>(AbilityValue(suppressing, "AllyHitDurationExtension")));
                if ((effect.flags & 2u) != 0u)
                {
                    ModifierEffectState followup; followup.id = effect.id + ":FreeFollowup"; followup.sourceAbilityId = effect.sourceAbilityId;
                    followup.kind = ModifierEffectKind::SuppressingFire; followup.causerId = sniper->id;
                    followup.preferredTargetId = target.id; followup.valueA = 1; followup.remainingInitiative = effect.remainingInitiative;
                    sniper->modifierEffects.push_back(followup);
                    modifierEvents.push_back(GameEvent{EventType::EffectAdded, sniper->id, sniper->id, sniper->tileId,
                        followup.id, followup.remainingInitiative});
                }
            }
            if (target.health <= 0)
            {
                const std::int32_t before = sniper->resourceCurrent;
                sniper->resourceCurrent = std::min(sniper->resourceMaximum, sniper->resourceCurrent + effect.valueA);
                if (before != sniper->resourceCurrent) modifierEvents.push_back(GameEvent{EventType::ResourceChanged,
                    sniper->id, sniper->id, sniper->tileId, effect.id, sniper->resourceCurrent - before});
                if ((effect.flags & 1u) != 0u && effect.valueB > 0)
                {
                    const TileState* deadTile = FindTile(target.tileId); CharacterState* nearest = nullptr; std::int32_t best = effect.valueB + 1;
                    for (CharacterState& candidate : state_.characters)
                    {
                        const TileState* tile = FindTile(candidate.tileId);
                        if (!candidate.alive || candidate.id == target.id || candidate.team == sniper->team || candidate.team == Team::Neutral || !deadTile || !tile) continue;
                        const std::int32_t distance = HexDistance(deadTile->coord, tile->coord);
                        if (distance < best) { best = distance; nearest = &candidate; }
                    }
                    if (nearest)
                    {
                        ModifierEffectState chained = effect; chained.counterB = 0; chained.counterC = 0;
                        nearest->modifierEffects.push_back(chained);
                        modifierEvents.push_back(GameEvent{EventType::EffectAdded, sniper->id, nearest->id,
                            nearest->tileId, chained.id, chained.remainingInitiative});
                    }
                }
            }
        }
    }
    if (target.health <= 0 && PassiveValue(targetDefinition, "CoreBloodCharacter") > 0.0 && target.specialCooldownRemaining <= 0)
    {
        const std::int32_t cost = std::max(0, static_cast<std::int32_t>(PassiveValue(targetDefinition, "BloodCheatDeathCost")));
        const std::int32_t heal = std::max(0, static_cast<std::int32_t>(PassiveValue(targetDefinition, "BloodCheatDeathHeal")));
        if (cost > 0 && heal > 0 && target.resourceCurrent >= cost)
        {
            target.resourceCurrent -= cost; target.health = std::clamp(heal, 1, targetDefinition.stats.maxHealth);
            target.specialCooldownRemaining = std::max(0, static_cast<std::int32_t>(PassiveValue(targetDefinition, "BloodCheatDeathCooldownInitiative")));
            modifierEvents.push_back(GameEvent{EventType::ResourceChanged, target.id, target.id, target.tileId, "BloodCheatDeath", -cost});
            modifierEvents.push_back(GameEvent{EventType::HealingApplied, target.id, target.id, target.tileId, "BloodCheatDeath", target.health});
        }
    }
    if (target.health <= 0 && target.resourceCurrent > 0 && target.specialCooldownRemaining <= 0)
    {
        const std::int32_t healPerResource = std::max(
            0, static_cast<std::int32_t>(PassiveValue(targetDefinition, "CheatDeathHealPerConsumedChain")));
        if (healPerResource > 0)
        {
            const std::int32_t consumed = target.resourceCurrent;
            target.resourceCurrent = 0;
            target.specialCooldownRemaining = std::max(0, static_cast<std::int32_t>(
                PassiveValue(targetDefinition, "CheatDeathCooldownInitiative")));
            target.health = std::clamp(healPerResource * consumed, 1, targetDefinition.stats.maxHealth);
            modifierEvents.push_back(GameEvent{EventType::ResourceChanged, target.id, target.id,
                target.tileId, "NativeResource", -consumed});
            modifierEvents.push_back(GameEvent{EventType::HealingApplied, target.id, target.id,
                target.tileId, "CheatDeath", target.health});
        }
    }
    return appliedDamage;
}

void Game::ResolveConcentrateHit(CharacterState& source, CharacterState& target, const bool killed,
    const bool basicAttack, AbilityState* usedAbility, const std::int32_t appliedDamage,
    std::vector<GameEvent>& events)
{
    auto focus = std::find_if(source.modifierEffects.begin(), source.modifierEffects.end(),
        [](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::Concentrate; });
    if (focus == source.modifierEffects.end() || appliedDamage <= 0) return;
    const AbilityDefinition& definition = definitions_.abilities.at(focus->sourceAbilityId);
    if (!killed)
    {
        const std::int32_t reduction = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DefenseReductionOnHit")));
        if (reduction > 0)
        {
            ModifierEffectState debuff; debuff.id = focus->id + ":DefenseBreak"; debuff.causerId = source.id;
            debuff.isDebuff = true; debuff.defenseIgnoreAdd = reduction;
            debuff.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "DefenseReductionDurationInitiative")));
            target.modifierEffects.push_back(debuff);
            events.push_back(GameEvent{EventType::EffectAdded, source.id, target.id, target.tileId, debuff.id, debuff.remainingInitiative});
        }
        const double fraction = AbilityValue(definition, "CurrentInitiativeLossFraction");
        if (fraction > 0.0)
        {
            const std::int32_t before = target.initiative;
            target.initiative = std::max(0, target.initiative - static_cast<std::int32_t>(target.initiative * fraction + 0.5));
            events.push_back(GameEvent{EventType::InitiativeChanged, source.id, target.id, target.tileId, focus->id, target.initiative - before});
        }
    }
    if (AbilityValue(definition, "GrantFreeStepAfterHit") > 0.0) source.hasFreePreparedStep = true;
    if (usedAbility)
    {
        const std::int32_t reduction = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "AbilityCooldownReductionOnConsume")));
        const std::int32_t before = usedAbility->cooldownRemaining;
        usedAbility->cooldownRemaining = std::max(0, before - reduction);
        if (before != usedAbility->cooldownRemaining) events.push_back(GameEvent{EventType::CooldownChanged, source.id, source.id,
            source.tileId, usedAbility->definitionId, usedAbility->cooldownRemaining - before});
    }
    const double splash = AbilityValue(definition, "SplashDamageFraction");
    const TileState* center = FindTile(target.tileId);
    if (splash > 0.0 && center)
    {
        for (CharacterState& other : state_.characters)
        {
            const TileState* tile = FindTile(other.tileId);
            if (!other.alive || other.id == target.id || other.team == source.team || !tile || HexDistance(center->coord, tile->coord) != 1) continue;
            std::vector<GameEvent> aura, modifiers;
            const std::int32_t damage = ResolveDamage(source, other, static_cast<std::int32_t>(appliedDamage * splash + 0.5), 0, aura, modifiers);
            events.push_back(GameEvent{EventType::DamageApplied, source.id, other.id, other.tileId, focus->id + ":Splash", damage});
            events.insert(events.end(), aura.begin(), aura.end()); events.insert(events.end(), modifiers.begin(), modifiers.end());
        }
    }
    if ((focus->flags & 4u) != 0u && killed) return;
    if (--focus->counterA > 0)
    {
        const std::int32_t bonus = std::max(0, focus->outgoingDamageMultiplierBasisPoints - 10000);
        focus->outgoingDamageMultiplierBasisPoints = 10000 + static_cast<std::int32_t>(bonus * (focus->valueA / 10000.0) + 0.5);
        return;
    }
    const bool volley = basicAttack && (focus->flags & 8u) != 0u;
    const DefinitionId removedId = focus->id;
    source.modifierEffects.erase(focus);
    events.push_back(GameEvent{EventType::EffectRemoved, source.id, source.id, source.tileId, removedId, 0});
    if (volley && target.alive)
    {
        const CharacterDefinition& sourceDefinition = definitions_.characters.at(source.definitionId);
        while (source.resourceCurrent > 0 && target.alive)
        {
            std::vector<GameEvent> aura, modifiers;
            std::int32_t volleyBase = sourceDefinition.stats.attack / 2;
            if (RollBasisPoints(sourceDefinition.criticalChanceBasisPoints))
                volleyBase = static_cast<std::int32_t>(ApplyBasisPoints(
                    volleyBase, sourceDefinition.criticalDamageMultiplierBasisPoints));
            const std::int32_t damage = ResolveDamage(source, target, volleyBase, 0, aura, modifiers);
            events.push_back(GameEvent{EventType::DamageApplied, source.id, target.id, target.tileId, removedId + ":Volley", damage});
            events.insert(events.end(), aura.begin(), aura.end()); events.insert(events.end(), modifiers.begin(), modifiers.end());
            if (target.health <= 0)
            {
                target.health = 0; target.alive = false;
                if (TileState* tile = FindTile(target.tileId)) tile->occupantId = InvalidEntityId;
                events.push_back(GameEvent{EventType::CharacterDied, source.id, target.id,
                    target.tileId, removedId + ":Volley", 0});
            }
            ResolveAmmoShot(source, !target.alive, false, removedId + ":Volley", events);
        }
    }
}

bool Game::ResolveAmmoShot(CharacterState& source, const bool killed, const bool explicitRefund,
    const DefinitionId& sourceId, std::vector<GameEvent>& events)
{
    if (source.resourceCurrent <= 0) return false;
    const CharacterDefinition& definition = definitions_.characters.at(source.definitionId);
    const std::int32_t before = source.resourceCurrent;
    --source.resourceCurrent;
    source.attackedThisTurn = true;
    source.spentResourceThisTurn = true;
    events.push_back(GameEvent{EventType::ResourceChanged, source.id, source.id,
        source.tileId, sourceId, -1});
    const bool passiveRefund = killed && before == 1 && !source.refundedLastResourceKillThisTurn
        && PassiveValue(definition, "LastAmmoKillRefund") > 0.0;
    if (explicitRefund || passiveRefund)
    {
        source.resourceCurrent = std::min(source.resourceMaximum, source.resourceCurrent + 1);
        events.push_back(GameEvent{EventType::ResourceChanged, source.id, source.id,
            source.tileId, passiveRefund ? "LastAmmoKillRefund" : sourceId + ":Refund", 1});
    }
    source.refundedLastResourceKillThisTurn = source.refundedLastResourceKillThisTurn || passiveRefund;
    if (source.resourceCurrent == 0 && !source.triggeredEmptyResourceThisTurn)
    {
        source.triggeredEmptyResourceThisTurn = true;
        const std::int32_t gain = std::max(0, static_cast<std::int32_t>(
            PassiveValue(definition, "EmptyMagazineInitiativeGain")));
        source.initiative += gain;
        if (gain > 0) events.push_back(GameEvent{EventType::InitiativeChanged, source.id,
            source.id, source.tileId, "EmptyMagazine", gain});
    }
    if (!source.movedSinceLastTurn && PassiveValue(definition, "StationaryShotDamagePerStack") > 0.0)
    {
        const std::int32_t maximum = std::max(1, static_cast<std::int32_t>(
            PassiveValue(definition, "StationaryShotMaxStacks")));
        source.stationaryShotStacks = std::min(maximum, source.stationaryShotStacks + 1);
    }
    return true;
}

std::int32_t Game::GainCombo(CharacterState& source, const std::int32_t amount,
    const DefinitionId& sourceId, std::vector<GameEvent>& events)
{
    if (amount <= 0 || source.resourceMaximum <= 0) return 0;
    const CharacterDefinition& definition = definitions_.characters.at(source.definitionId);
    const std::int32_t before = source.resourceCurrent;
    source.resourceCurrent = std::clamp(source.resourceCurrent + amount, 0, source.resourceMaximum);
    const std::int32_t gained = source.resourceCurrent - before;
    if (gained > 0) events.push_back(GameEvent{EventType::ResourceChanged, source.id, source.id,
        source.tileId, sourceId, gained});
    if (before >= source.resourceMaximum && !source.overflowRewardUsedThisTurn)
    {
        const std::int32_t initiative = std::max(0, static_cast<std::int32_t>(
            PassiveValue(definition, "ComboOverflowInitiativeGain")));
        if (initiative > 0)
        {
            source.initiative += initiative;
            source.overflowRewardUsedThisTurn = true;
            events.push_back(GameEvent{EventType::InitiativeChanged, source.id, source.id,
                source.tileId, "ComboOverflow", initiative});
        }
    }
    return gained;
}

std::int32_t Game::SpendCombo(CharacterState& source, const std::int32_t amount,
    const DefinitionId& sourceId, std::vector<GameEvent>& events)
{
    const std::int32_t spent = std::min(std::max(0, amount), std::max(0, source.resourceCurrent));
    if (spent <= 0) return 0;
    source.resourceCurrent -= spent;
    source.resourceSpentThisTurnCount += spent;
    events.push_back(GameEvent{EventType::ResourceChanged, source.id, source.id,
        source.tileId, sourceId, -spent});
    const CharacterDefinition& definition = definitions_.characters.at(source.definitionId);
    const std::int32_t gain = spent * std::max(0, static_cast<std::int32_t>(
        PassiveValue(definition, "ComboSpendInitiativeGainPerPoint")));
    if (gain > 0)
    {
        source.initiative += gain;
        events.push_back(GameEvent{EventType::InitiativeChanged, source.id, source.id,
            source.tileId, sourceId + ":ComboSpend", gain});
    }
    return spent;
}

std::int32_t Game::GainBlood(CharacterState& source, const std::int32_t amount, const bool raw,
    const DefinitionId& sourceId, std::vector<GameEvent>& events)
{
    if (amount <= 0 || source.resourceMaximum <= 0) return 0;
    const CharacterDefinition& definition = definitions_.characters.at(source.definitionId);
    const double configuredMultiplier = PassiveValue(definition, "BloodGainMultiplier");
    const double gainMultiplier = raw ? 1.0 : std::max(0.0, PassiveValue(definition, "BloodGainFraction"))
        * (configuredMultiplier > 0.0 ? configuredMultiplier : 1.0);
    const std::int32_t requested = raw ? amount : std::max(0, static_cast<std::int32_t>(amount * gainMultiplier + 0.5));
    const std::int32_t before = source.resourceCurrent;
    const std::int32_t overflow = std::max(0, before + requested - source.resourceMaximum);
    source.resourceCurrent = std::clamp(before + requested, 0, source.resourceMaximum);
    const std::int32_t gained = source.resourceCurrent - before;
    if (gained > 0) events.push_back(GameEvent{EventType::ResourceChanged, source.id, source.id, source.tileId, sourceId, gained});
    if (overflow > 0)
    {
        const std::int32_t healing = std::min(std::max(0, static_cast<std::int32_t>(overflow
            * PassiveValue(definition, "BloodOverflowHealFraction") + 0.5)), std::max(0, definition.stats.maxHealth - source.health));
        source.health += healing;
        if (healing > 0) events.push_back(GameEvent{EventType::HealingApplied, source.id, source.id, source.tileId, sourceId + ":Overflow", healing});
    }
    if (before < source.resourceMaximum && source.resourceCurrent >= source.resourceMaximum && source.bloodCapTriggerAvailable)
    {
        const std::int32_t reduction = std::max(0, static_cast<std::int32_t>(PassiveValue(definition, "CooldownReductionOnBloodCap")));
        for (AbilityState& ability : source.abilities) { const std::int32_t previous = ability.cooldownRemaining; ability.cooldownRemaining = std::max(0, previous - reduction);
            if (previous != ability.cooldownRemaining) events.push_back(GameEvent{EventType::CooldownChanged, source.id, source.id, source.tileId, ability.definitionId, ability.cooldownRemaining - previous}); }
        source.bloodCapTriggerAvailable = false;
    }
    return gained;
}

std::int32_t Game::SpendBlood(CharacterState& source, const std::int32_t maximumAmount,
    const DefinitionId& sourceId, std::vector<GameEvent>& events)
{
    const std::int32_t spent = maximumAmount > 0 ? std::min(source.resourceCurrent, maximumAmount) : source.resourceCurrent;
    if (spent <= 0) return 0;
    source.resourceCurrent -= spent;
    events.push_back(GameEvent{EventType::ResourceChanged, source.id, source.id, source.tileId, sourceId, -spent});
    const CharacterDefinition& definition = definitions_.characters.at(source.definitionId);
    std::int32_t gain = std::max(0, static_cast<std::int32_t>(PassiveValue(definition, "InitiativeGainOnBloodSpend")));
    const std::int32_t threshold = std::max(0, static_cast<std::int32_t>(PassiveValue(definition, "BloodSpendThresholdForInitiative")));
    if (threshold > 0 && spent >= threshold) gain += std::max(0, static_cast<std::int32_t>(PassiveValue(definition, "InitiativeGainOnLargeBloodSpend")));
    source.initiative += gain;
    if (gain > 0) events.push_back(GameEvent{EventType::InitiativeChanged, source.id, source.id, source.tileId, sourceId + ":Spend", gain});
    return spent;
}

void Game::ResolveAssassinHit(CharacterState& source, CharacterState& target,
    const std::int32_t appliedDamage, const bool killed, const DefinitionId& sourceId,
    std::vector<GameEvent>& events)
{
    if (appliedDamage <= 0) return;
    const CharacterDefinition& definition = definitions_.characters.at(source.definitionId);
    if (PassiveValue(definition, "CoreComboCharacter") <= 0.0) return;

    for (auto effect = source.modifierEffects.begin(); effect != source.modifierEffects.end();)
    {
        if (effect->kind == ModifierEffectKind::Invisibility)
        {
            events.push_back(GameEvent{EventType::EffectRemoved, source.id, source.id,
                source.tileId, effect->id, 0});
            effect = source.modifierEffects.erase(effect);
        }
        else ++effect;
    }
    source.nextDamageAfterMoveReady = false;
    if (!source.freeAdjacentMoveUsedThisTurn
        && PassiveValue(definition, "FreeAdjacentMoveAfterDamage") > 0.0)
        source.freeAdjacentMoveAvailable = true;

    if (!source.openingRewardUsedThisTurn
        && target.health + appliedDamage >= definitions_.characters.at(target.definitionId).stats.maxHealth)
    {
        const std::int32_t combo = std::max(0, static_cast<std::int32_t>(
            PassiveValue(definition, "ComboOnFullHealthHit")));
        if (combo > 0)
        {
            GainCombo(source, combo, sourceId + ":Opening", events);
            source.openingRewardUsedThisTurn = true;
        }
    }

    if (killed)
    {
        for (auto effect = source.modifierEffects.begin(); effect != source.modifierEffects.end();)
        {
            if (effect->kind == ModifierEffectKind::AssassinFollowup && (effect->flags & 1u) != 0u)
            {
                for (AbilityState& ability : source.abilities)
                {
                    if (ability.definitionId != effect->sourceAbilityId) continue;
                    const std::int32_t previous = ability.cooldownRemaining;
                    ability.cooldownRemaining = 0;
                    if (ability.usesCharges) ability.currentCharges = ability.maxCharges;
                    if (previous != 0) events.push_back(GameEvent{EventType::CooldownChanged, source.id, source.id,
                        source.tileId, ability.definitionId, -previous});
                }
                events.push_back(GameEvent{EventType::EffectRemoved, source.id, source.id,
                    source.tileId, effect->id, 0});
                effect = source.modifierEffects.erase(effect);
            }
            else ++effect;
        }
        GainCombo(source, std::max(0, static_cast<std::int32_t>(
            PassiveValue(definition, "ComboOnKill"))), sourceId + ":Kill", events);
        const std::int32_t duration = std::max(0, static_cast<std::int32_t>(
            PassiveValue(definition, "InvisibilityOnKillDurationInitiative")));
        const bool invisibilityPrevented = std::any_of(source.modifierEffects.begin(), source.modifierEffects.end(),
            [](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::DeathMark
                && (effect.flags & 4u) != 0u; });
        if (duration > 0 && !invisibilityPrevented)
        {
            ModifierEffectState invisible; invisible.id = "Character3KillInvisibility";
            invisible.kind = ModifierEffectKind::Invisibility; invisible.causerId = source.id;
            invisible.remainingInitiative = duration; source.modifierEffects.push_back(invisible);
            events.push_back(GameEvent{EventType::EffectAdded, source.id, source.id,
                source.tileId, invisible.id, duration});
        }
        const auto deathMark = std::find_if(target.modifierEffects.begin(), target.modifierEffects.end(),
            [&source](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::DeathMark
                && effect.causerId == source.id && effect.counterC > 0 && (effect.flags & 16u) == 0u; });
        if (deathMark != target.modifierEffects.end())
        {
            const TileState* deadTile = FindTile(target.tileId);
            CharacterState* nearest = nullptr; std::int32_t nearestDistance = std::numeric_limits<std::int32_t>::max();
            for (CharacterState& candidate : state_.characters)
            {
                const TileState* tile = FindTile(candidate.tileId);
                if (!candidate.alive || candidate.id == target.id || candidate.team == source.team || !deadTile || !tile) continue;
                const std::int32_t distance = HexDistance(deadTile->coord, tile->coord);
                if (distance <= deathMark->counterC && distance < nearestDistance)
                { nearest = &candidate; nearestDistance = distance; }
            }
            if (nearest)
            {
                ModifierEffectState transferred = *deathMark;
                transferred.id += ":Transfer"; transferred.flags |= 16u;
                nearest->modifierEffects.push_back(transferred);
                events.push_back(GameEvent{EventType::EffectAdded, source.id, nearest->id,
                    nearest->tileId, transferred.id, transferred.remainingInitiative});
            }
        }
    }
    else if (source.secondaryCooldownRemaining <= 0)
    {
        const std::int32_t duration = std::max(0, static_cast<std::int32_t>(
            PassiveValue(definition, "HuntMarkDurationInitiative")));
        const double multiplier = PassiveValue(definition, "HuntMarkDamageMultiplier");
        if (duration > 0 && multiplier > 0.0)
        {
            ModifierEffectState mark; mark.id = "Character3HuntMark"; mark.causerId = source.id;
            mark.kind = ModifierEffectKind::AssassinHuntMark; mark.isDebuff = true;
            mark.remainingInitiative = duration;
            mark.incomingDamageMultiplierFromCauserBasisPoints = static_cast<std::int32_t>(multiplier * 10000.0 + 0.5);
            target.modifierEffects.push_back(mark);
            source.secondaryCooldownRemaining = duration + std::max(0, static_cast<std::int32_t>(
                PassiveValue(definition, "HuntMarkCooldownAfterExpiryInitiative")));
            events.push_back(GameEvent{EventType::EffectAdded, source.id, target.id,
                target.tileId, mark.id, duration});
        }
    }
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
    const bool ammoCharacter = PassiveValue(attackerDefinition, "CoreAmmoCharacter") > 0.0;
    const bool comboCharacter = PassiveValue(attackerDefinition, "CoreComboCharacter") > 0.0;
    const bool concentrateFreeAmmo = std::any_of(attacker->modifierEffects.begin(), attacker->modifierEffects.end(),
        [](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::Concentrate && (effect.flags & 2u) != 0u; });
    const bool suppressionFreeAmmo = std::any_of(attacker->modifierEffects.begin(), attacker->modifierEffects.end(),
        [target](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::SuppressingFire
            && effect.preferredTargetId == target->id && effect.valueA == 1; });
    const bool freeAmmoAttack = ammoCharacter && (concentrateFreeAmmo || suppressionFreeAmmo || (!attacker->usedFreeBasicAttackThisTurn
        && PassiveValue(attackerDefinition, "FirstBasicAttackFreeAmmo") > 0.0));
    const std::int32_t attackCost = BasicAttackCost(*attacker);
    attacker->initiative -= attackCost;

    std::int64_t damage = std::max(0, attackerDefinition.stats.attack);
    std::int32_t defenseIgnore = 0;
    std::vector<GameEvent> modifierEvents;
    const std::int32_t criticalChance = std::clamp(attackerDefinition.criticalChanceBasisPoints, 0, 10000);
    const bool critical = RollBasisPoints(criticalChance);
    if (critical)
    {
        const std::int64_t multiplier = std::max(0, attackerDefinition.criticalDamageMultiplierBasisPoints);
        damage = (damage * multiplier + 5000) / 10000;
    }

    for (const ModifierEffectState& effect : attacker->modifierEffects)
    {
        damage = ApplyBasisPoints(damage, effect.basicAttackDamageMultiplierBasisPoints);
    }

    std::vector<GameEvent> auraEvents;
    const std::int32_t appliedDamage = ResolveDamage(
        *attacker, *target, damage, defenseIgnore, auraEvents, modifierEvents);

    ActionResult result;
    result.success = true;
    result.message = critical ? "Critical basic attack executed." : "Basic attack executed.";
    result.events.push_back(GameEvent{
        EventType::InitiativeChanged, attacker->id, attacker->id, attacker->tileId, {}, -attackCost});
    result.events.push_back(GameEvent{
        EventType::DamageApplied, attacker->id, target->id, target->tileId, "Physical", appliedDamage});
    result.events.insert(result.events.end(), auraEvents.begin(), auraEvents.end());
    result.events.insert(result.events.end(), modifierEvents.begin(), modifierEvents.end());
    ResolveAssassinHit(*attacker, *target, appliedDamage, target->health <= 0, "BasicAttack", result.events);
    const CharacterDefinition& attackerDefinitionForBlood = definitions_.characters.at(attacker->definitionId);
    if (appliedDamage > 0 && attacker->basicAttackBloodAvailable
        && PassiveValue(attackerDefinitionForBlood, "CoreBloodCharacter") > 0.0)
    {
        const std::int32_t bonus = std::max(0, static_cast<std::int32_t>(PassiveValue(attackerDefinitionForBlood, "FirstBasicAttackBloodGain")));
        if (bonus > 0) { GainBlood(*attacker, bonus, true, "BasicAttackBlood", result.events); attacker->basicAttackBloodAvailable = false; }
    }
    for (auto preparation = attacker->modifierEffects.begin(); preparation != attacker->modifierEffects.end();)
    {
        if (preparation->kind == ModifierEffectKind::PiercingMark && preparation->preferredTargetId == InvalidEntityId
            && preparation->counterA > 0 && preparation->valueA > 0)
        {
            ModifierEffectState breakEffect; breakEffect.id = preparation->id + ":DefenseBreak"; breakEffect.causerId = attacker->id;
            breakEffect.isDebuff = true; breakEffect.defenseIgnoreAdd = preparation->counterA * preparation->valueA;
            breakEffect.remainingInitiative = std::max(1, preparation->valueB); target->modifierEffects.push_back(breakEffect);
            result.events.push_back(GameEvent{EventType::EffectAdded, attacker->id, target->id, target->tileId,
                breakEffect.id, breakEffect.remainingInitiative});
            preparation = attacker->modifierEffects.erase(preparation);
        }
        else ++preparation;
    }
    std::vector<DefinitionId> piercingAbilities;
    for (const ModifierEffectState& mark : target->modifierEffects)
        if (mark.kind == ModifierEffectKind::PiercingMark && mark.causerId == attacker->id && !mark.sourceAbilityId.empty()) piercingAbilities.push_back(mark.sourceAbilityId);
    for (const DefinitionId& piercingId : piercingAbilities)
    {
        const AbilityDefinition& piercing = definitions_.abilities.at(piercingId);
        const double chain = AbilityValue(piercing, "ChainMarkDamageMultiplier");
        if (chain <= 0.0) continue;
        for (CharacterState& other : state_.characters)
        {
            if (!other.alive || other.id == target->id) continue;
            const bool marked = std::any_of(other.modifierEffects.begin(), other.modifierEffects.end(),
                [&piercingId, attacker](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::PiercingMark
                    && effect.causerId == attacker->id && effect.sourceAbilityId == piercingId; });
            if (!marked) continue;
            std::vector<GameEvent> chainAura, chainModifiers;
            const std::int32_t chainDamage = ResolveDamage(*attacker, other,
                static_cast<std::int32_t>(attackerDefinition.stats.attack * chain + 0.5), 0, chainAura, chainModifiers);
            result.events.push_back(GameEvent{EventType::DamageApplied, attacker->id, other.id, other.tileId, piercingId + ":Chain", chainDamage});
            result.events.insert(result.events.end(), chainAura.begin(), chainAura.end()); result.events.insert(result.events.end(), chainModifiers.begin(), chainModifiers.end());
        }
        for (CharacterState& markedCharacter : state_.characters)
            markedCharacter.modifierEffects.erase(std::remove_if(markedCharacter.modifierEffects.begin(), markedCharacter.modifierEffects.end(),
                [&piercingId, attacker](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::PiercingMark
                    && effect.causerId == attacker->id && effect.sourceAbilityId == piercingId; }), markedCharacter.modifierEffects.end());
    }
    ResolveConcentrateHit(*attacker, *target, target->health <= 0, true, nullptr, appliedDamage, result.events);
    if (suppressionFreeAmmo)
    {
        attacker->modifierEffects.erase(std::remove_if(attacker->modifierEffects.begin(), attacker->modifierEffects.end(),
            [target](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::SuppressingFire
                && effect.preferredTargetId == target->id && effect.valueA == 1; }), attacker->modifierEffects.end());
    }
    for (auto effect = attacker->modifierEffects.begin(); effect != attacker->modifierEffects.end();)
    {
        effect->basicAttackCostAvailable = false;
        if (effect->consumeOnBasicAttack)
        {
            result.events.push_back(GameEvent{
                EventType::EffectRemoved, attacker->id, attacker->id, attacker->tileId, effect->id, 0});
            effect = attacker->modifierEffects.erase(effect);
        }
        else
        {
            ++effect;
        }
    }

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

    if (ammoCharacter)
    {
        attacker->attackedThisTurn = true;
        if (freeAmmoAttack)
        {
            if (!concentrateFreeAmmo) attacker->usedFreeBasicAttackThisTurn = true;
        }
        else if (attacker->resourceCurrent > 0)
        {
            const std::int32_t before = attacker->resourceCurrent;
            --attacker->resourceCurrent;
            attacker->spentResourceThisTurn = true;
            result.events.push_back(GameEvent{EventType::ResourceChanged, attacker->id, attacker->id,
                attacker->tileId, "Ammo", attacker->resourceCurrent - before});
            const bool killed = !target->alive;
            if (killed && before == 1 && !attacker->refundedLastResourceKillThisTurn
                && PassiveValue(attackerDefinition, "LastAmmoKillRefund") > 0.0)
            {
                ++attacker->resourceCurrent;
                attacker->refundedLastResourceKillThisTurn = true;
                result.events.push_back(GameEvent{EventType::ResourceChanged, attacker->id, attacker->id,
                    attacker->tileId, "LastAmmoKillRefund", 1});
            }
            if (attacker->resourceCurrent == 0 && !attacker->triggeredEmptyResourceThisTurn)
            {
                attacker->triggeredEmptyResourceThisTurn = true;
                const std::int32_t gain = std::max(0, static_cast<std::int32_t>(
                    PassiveValue(attackerDefinition, "EmptyMagazineInitiativeGain")));
                attacker->initiative += gain;
                if (gain > 0) result.events.push_back(GameEvent{EventType::InitiativeChanged, attacker->id,
                    attacker->id, attacker->tileId, "EmptyMagazine", gain});
            }
        }
        if (!attacker->movedSinceLastTurn)
        {
            const std::int32_t maximum = std::max(1, static_cast<std::int32_t>(
                PassiveValue(attackerDefinition, "StationaryShotMaxStacks")));
            if (PassiveValue(attackerDefinition, "StationaryShotDamagePerStack") > 0.0)
                attacker->stationaryShotStacks = std::min(maximum, attacker->stationaryShotStacks + 1);
        }
    }
    if (comboCharacter) attacker->attackedThisTurn = true;

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
    const bool concentrateFreeAmmo = std::any_of(attacker->modifierEffects.begin(), attacker->modifierEffects.end(),
        [](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::Concentrate && (effect.flags & 2u) != 0u; });
    const bool suppressionFreeAmmo = std::any_of(attacker->modifierEffects.begin(), attacker->modifierEffects.end(),
        [target](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::SuppressingFire
            && effect.preferredTargetId == target->id && effect.valueA == 1; });
    if (PassiveValue(definition, "CoreAmmoCharacter") > 0.0
        && attacker->resourceCurrent <= 0
        && !concentrateFreeAmmo
        && !suppressionFreeAmmo
        && (attacker->usedFreeBasicAttackThisTurn
            || PassiveValue(definition, "FirstBasicAttackFreeAmmo") <= 0.0))
    {
        return Failure("insufficient_resource", "The character has no ammunition for a basic attack.");
    }
    std::int32_t attackRange = std::max(0, definition.stats.attackRange);
    for (const ModifierEffectState& effect : target->modifierEffects)
    {
        if (effect.kind == ModifierEffectKind::PiercingMark && effect.causerId == attacker->id && !effect.sourceAbilityId.empty())
        {
            const auto piercing = definitions_.abilities.find(effect.sourceAbilityId);
            if (piercing != definitions_.abilities.end()) attackRange += std::max(0,
                static_cast<std::int32_t>(AbilityValue(piercing->second, "AttackRangeVsMarked")));
        }
    }
    if (HexDistance(attackerTile->coord, targetTile->coord) > attackRange)
    {
        return Failure("target_out_of_range", "The target is outside basic attack range.");
    }
    const std::vector<EntityId> visibleTiles = TeamVisibleTileIds(attacker->team);
    if (!std::binary_search(visibleTiles.begin(), visibleTiles.end(), targetTile->id))
    {
        return Failure("target_not_visible", "The target is not visible to the attacking team.");
    }
    if (attacker->initiative < BasicAttackCost(*attacker))
    {
        return Failure("insufficient_initiative", "The character cannot afford this basic attack.");
    }
    return ActionResult{true};
}

ActionResult Game::ValidateAbility(const GameAction& action) const
{
    if (state_.status != MatchStatus::Running)
    {
        return Failure("match_not_running", "Abilities require a running match.");
    }
    const CharacterState* actor = FindCharacter(action.actorId);
    if (!actor || !actor->alive)
    {
        return Failure("invalid_ability_actor", "Ability actor must exist and be alive.");
    }
    if (state_.activeCharacterId != actor->id)
    {
        return Failure("not_active_character", "Only the active character can use an ability.");
    }
    if (std::any_of(actor->modifierEffects.begin(), actor->modifierEffects.end(),
            [](const ModifierEffectState& effect) { return effect.preventsAbilityUse; }))
    {
        return Failure("ability_use_blocked", "An active effect prevents this character from using abilities.");
    }
    const auto abilityDefinition = definitions_.abilities.find(action.abilityId);
    if (abilityDefinition == definitions_.abilities.end()
        || abilityDefinition->second.operation == AbilityOperation::Unsupported)
    {
        return Failure("unsupported_ability", "This ability has not been migrated to RoundCore.");
    }
    const auto abilityState = std::find_if(actor->abilities.begin(), actor->abilities.end(),
        [&action](const AbilityState& ability) { return ability.definitionId == action.abilityId; });
    if (abilityState == actor->abilities.end())
    {
        return Failure("ability_not_owned", "The acting character does not own this ability.");
    }
    const bool hookAnchorReturn = abilityDefinition->second.operation == AbilityOperation::HookShot
        && std::any_of(actor->modifierEffects.begin(), actor->modifierEffects.end(),
            [&action](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::HookAnchor
                && effect.areaCenterTileId == action.targetTileId; });
    if (!hookAnchorReturn && ((!abilityState->usesCharges && abilityState->cooldownRemaining > 0)
        || (abilityState->usesCharges && abilityState->currentCharges <= 0)))
    {
        return Failure("ability_not_ready", "The ability has no ready cooldown or charge.");
    }

    const AbilityDefinition& definition = abilityDefinition->second;
    if ((definition.operation == AbilityOperation::Snipe || definition.operation == AbilityOperation::PiercingShot)
        && actor->resourceCurrent <= 0)
    {
        return Failure("insufficient_resource", "Snipe requires ammunition.");
    }
    if (definition.operation == AbilityOperation::Aim
        && actor->resourceCurrent < std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "AmmoCost"))))
    {
        return Failure("insufficient_resource", "Aim requires its configured ammunition cost.");
    }
    const TileState* actorTile = FindTile(actor->tileId);
    if (!actorTile)
    {
        return Failure("missing_ability_tile", "Ability actor must occupy a board tile.");
    }
    if (definition.operation == AbilityOperation::GroundAura || definition.operation == AbilityOperation::Aim
        || definition.operation == AbilityOperation::HookShot || definition.operation == AbilityOperation::Fireball
        || definition.operation == AbilityOperation::Lightningstrike || definition.operation == AbilityOperation::Rockblast
        || definition.operation == AbilityOperation::ArcaneShift || definition.operation == AbilityOperation::BuildTurret
        || definition.operation == AbilityOperation::BuildMine || definition.operation == AbilityOperation::BuildShieldEmitter
        || definition.operation == AbilityOperation::BuildSpotterBeacon)
    {
        const TileState* targetTile = FindTile(action.targetTileId);
        if (!targetTile)
        {
            return Failure("missing_ability_target_tile", "Ground abilities require a board tile target.");
        }
        const CharacterDefinition& actorDefinition = definitions_.characters.at(actor->definitionId);
        const std::int32_t elementRange = actor->activeElement == 2
            ? std::max(0, static_cast<std::int32_t>(PassiveValue(actorDefinition, "ElementIceRangeAdd"))) : 0;
        if (HexDistance(actorTile->coord, targetTile->coord) > std::max(0, definition.range + elementRange))
        {
            return Failure("ability_target_out_of_range", "The target tile is outside ability range.");
        }
        if (definition.operation == AbilityOperation::HookShot)
        {
            const auto terrain = definitions_.terrains.find(targetTile->terrainId);
            if (terrain == definitions_.terrains.end() || !terrain->second.walkable || targetTile->blocksMovement)
                return Failure("ability_target_blocked", "Hook Shot requires a walkable destination.");
            if (targetTile->occupantId != InvalidEntityId)
            {
                const CharacterState* occupant = FindCharacter(targetTile->occupantId);
                if (!occupant || occupant->team != actor->team || occupant->id == actor->id
                    || AbilityValue(definition, "CanSwapWithAlly") <= 0.0)
                    return Failure("ability_target_occupied", "Hook Shot destination is occupied.");
            }
        }
        if (definition.operation == AbilityOperation::ArcaneShift)
        {
            const auto terrain = definitions_.terrains.find(targetTile->terrainId);
            if (terrain == definitions_.terrains.end() || !terrain->second.walkable || targetTile->blocksMovement)
                return Failure("ability_target_blocked", "Arcane Shift requires a walkable destination.");
            if (targetTile->occupantId != InvalidEntityId)
            {
                const CharacterState* occupant = FindCharacter(targetTile->occupantId);
                if (!occupant || occupant->team != actor->team || occupant->id == actor->id
                    || AbilityValue(definition, "CanSwapWithAlly") <= 0.0)
                    return Failure("ability_target_occupied", "Arcane Shift destination is occupied.");
            }
        }
        if (definition.operation >= AbilityOperation::BuildTurret
            && definition.operation <= AbilityOperation::BuildSpotterBeacon)
        {
            const auto terrain = definitions_.terrains.find(targetTile->terrainId);
            if (terrain == definitions_.terrains.end() || !terrain->second.walkable || targetTile->blocksMovement
                || targetTile->fieldObjectId != InvalidEntityId)
                return Failure("ability_target_blocked", "Building a device requires a free walkable tile.");
            if (definition.operation == AbilityOperation::BuildTurret && targetTile->occupantId != InvalidEntityId)
                return Failure("ability_target_occupied", "A turret cannot be built under a character.");
        }
        if (definition.requiresVisibility)
        {
            const std::vector<EntityId> visibleTiles = TeamVisibleTileIds(actor->team);
            if (!std::binary_search(visibleTiles.begin(), visibleTiles.end(), targetTile->id))
            {
                return Failure("ability_target_not_visible", "The ability target tile is not visible to the acting team.");
            }
        }
        if (!hookAnchorReturn && actor->initiative < std::max(0, definition.initiativeCost))
        {
            return Failure("insufficient_initiative", "The character cannot afford this ability.");
        }
        return ActionResult{true};
    }

    const CharacterState* target = FindCharacter(action.targetEntityId);
    if (!target || !target->alive)
    {
        return Failure("invalid_ability_characters", "Ability target must exist and be alive.");
    }
    const bool sameTeam = actor->team == target->team;
    const bool relationMatches = definition.targetRelation == AbilityTargetRelation::AnyCharacter
        || (definition.targetRelation == AbilityTargetRelation::Self && actor->id == target->id)
        || (definition.targetRelation == AbilityTargetRelation::Ally && sameTeam)
        || (definition.targetRelation == AbilityTargetRelation::Enemy && !sameTeam
            && target->team != Team::Neutral);
    if (!relationMatches)
    {
        return Failure("invalid_ability_relation", "The target does not match the ability's target relation.");
    }
    if (definition.operation == AbilityOperation::SuppressingFire && actor->resourceCurrent <= 0)
    {
        const bool targetDebuffed = std::any_of(target->modifierEffects.begin(), target->modifierEffects.end(),
            [](const ModifierEffectState& effect) { return effect.isDebuff; });
        if (!targetDebuffed || AbilityValue(definition, "FreeAmmoVsDebuffed") <= 0.0)
            return Failure("insufficient_resource", "Suppressing Fire requires ammunition.");
    }
    const std::int32_t recentAuraWindow = std::max(0,
        static_cast<std::int32_t>(AbilityValue(definition, "RecentAllyDamageWindowInitiative")));
    if (recentAuraWindow > 0)
    {
        const auto prevention = std::find_if(actor->recentAuraPreventions.begin(), actor->recentAuraPreventions.end(),
            [target](const RecentAuraPreventionState& state) { return state.sourceId == target->id; });
        if (prevention == actor->recentAuraPreventions.end()
            || prevention->globalInitiative + static_cast<std::uint64_t>(recentAuraWindow) < state_.globalInitiative)
        {
            return Failure("missing_recent_aura_prevention",
                "This ability requires recent damage prevention against the target.");
        }
    }
    const TileState* targetTile = FindTile(target->tileId);
    if (!actorTile || !targetTile)
    {
        return Failure("missing_ability_tile", "Ability actor and target must occupy board tiles.");
    }
    std::int32_t abilityRange = std::max(0, definition.range);
    const CharacterDefinition& actorDefinitionForRange = definitions_.characters.at(actor->definitionId);
    if (actor->activeElement == 2)
        abilityRange += std::max(0, static_cast<std::int32_t>(PassiveValue(actorDefinitionForRange, "ElementIceRangeAdd")));
    if (definition.operation == AbilityOperation::Shadowstrike
        && std::any_of(target->modifierEffects.begin(), target->modifierEffects.end(),
            [](const ModifierEffectState& effect) { return effect.isDebuff; }))
        abilityRange += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "RangeVsDebuffed")));
    if (definition.operation == AbilityOperation::Execute)
    {
        const CharacterDefinition& targetDefinition = definitions_.characters.at(target->definitionId);
        if (AbilityValue(definition, "BonusRangeHealthThreshold") > 0.0
            && target->health <= targetDefinition.stats.maxHealth * AbilityValue(definition, "BonusRangeHealthThreshold"))
            abilityRange += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "RangeVsLowHealth")));
        if (AbilityValue(definition, "RequireMaxCombo") > 0.0
            && actor->resourceCurrent < actor->resourceMaximum)
            return Failure("insufficient_resource", "Execute requires maximum combo.");
    }
    if (HexDistance(actorTile->coord, targetTile->coord) > abilityRange)
    {
        return Failure("ability_target_out_of_range", "The target is outside ability range.");
    }
    if (definition.requiresVisibility)
    {
        const std::vector<EntityId> visibleTiles = TeamVisibleTileIds(actor->team);
        if (!std::binary_search(visibleTiles.begin(), visibleTiles.end(), targetTile->id))
        {
            return Failure("ability_target_not_visible", "The ability target is not visible to the acting team.");
        }
    }
    if (actor->initiative < std::max(0, definition.initiativeCost))
    {
        return Failure("insufficient_initiative", "The character cannot afford this ability.");
    }
    const std::int32_t resourceCost = std::max(0,
        static_cast<std::int32_t>(AbilityValue(definition, "ChainCost")));
    if (actor->resourceCurrent < resourceCost)
    {
        return Failure("insufficient_resource", "The character cannot afford this ability's resource cost.");
    }
    return ActionResult{true};
}

ActionResult Game::ApplyAbility(const GameAction& action)
{
    CharacterState* actor = FindCharacter(action.actorId);
    CharacterState* target = FindCharacter(action.targetEntityId);
    if (!actor)
    {
        return Failure("invalid_state", "Validated ability references missing actor state.");
    }
    AbilityState* abilityState = nullptr;
    for (AbilityState& candidate : actor->abilities)
    {
        if (candidate.definitionId == action.abilityId)
        {
            abilityState = &candidate;
            break;
        }
    }
    if (!abilityState)
    {
        return Failure("invalid_state", "Validated ability has no runtime state.");
    }
    const AbilityDefinition& definition = definitions_.abilities.at(action.abilityId);
    if (definition.operation != AbilityOperation::GroundAura && definition.operation != AbilityOperation::Aim
        && definition.operation != AbilityOperation::HookShot && definition.operation != AbilityOperation::Fireball
        && definition.operation != AbilityOperation::Lightningstrike && definition.operation != AbilityOperation::Rockblast
        && definition.operation != AbilityOperation::ArcaneShift && definition.operation != AbilityOperation::BuildTurret
        && definition.operation != AbilityOperation::BuildMine && definition.operation != AbilityOperation::BuildShieldEmitter
        && definition.operation != AbilityOperation::BuildSpotterBeacon && !target)
    {
        return Failure("invalid_state", "Validated ability references missing target state.");
    }
    if (definition.operation == AbilityOperation::HookShot)
    {
        const auto anchor = std::find_if(actor->modifierEffects.begin(), actor->modifierEffects.end(),
            [&action](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::HookAnchor
                && effect.areaCenterTileId == action.targetTileId; });
        if (anchor != actor->modifierEffects.end())
        {
            TileState* origin = FindTile(actor->tileId);
            TileState* destination = FindTile(action.targetTileId);
            if (!origin || !destination || destination->occupantId != InvalidEntityId)
                return Failure("invalid_state", "Hook Shot anchor is no longer free.");
            origin->occupantId = InvalidEntityId; destination->occupantId = actor->id;
            actor->tileId = destination->id;
            const DefinitionId effectId = anchor->id;
            actor->modifierEffects.erase(anchor);
            ActionResult anchorResult; anchorResult.success = true;
            anchorResult.message = "Returned to Hook Shot anchor.";
            anchorResult.events.push_back(GameEvent{EventType::CharacterMoved, actor->id, actor->id,
                destination->id, action.abilityId + ":Return", 0});
            anchorResult.events.push_back(GameEvent{EventType::EffectRemoved, actor->id, actor->id,
                destination->id, effectId, 0});
            ++state_.step;
            return anchorResult;
        }
    }
    const CharacterDefinition& castingDefinition = definitions_.characters.at(actor->definitionId);
    const bool mageCast = definition.operation >= AbilityOperation::Fireball
        && definition.operation <= AbilityOperation::ArcaneShift;
    const std::int32_t storedElement = mageCast ? actor->activeElement : 0;
    const std::int32_t elementCostReduction = mageCast && storedElement != 0 && !actor->elementCostReductionUsed
        ? std::max(0, static_cast<std::int32_t>(PassiveValue(castingDefinition, "FirstElementUseCostReduction"))) : 0;
    const std::int32_t cost = std::max(0, definition.initiativeCost - elementCostReduction);
    actor->initiative -= cost;
    if (elementCostReduction > 0) actor->elementCostReductionUsed = true;

    ActionResult result;
    result.success = true;
    result.message = "Ability executed.";
    result.events.push_back(GameEvent{
        EventType::InitiativeChanged, actor->id, actor->id, actor->tileId, action.abilityId, -cost});
    const std::int32_t resourceCost = std::max(0,
        static_cast<std::int32_t>(AbilityValue(definition, "ChainCost")));
    if (resourceCost > 0)
    {
        actor->resourceCurrent -= resourceCost;
        result.events.push_back(GameEvent{EventType::ResourceChanged, actor->id, actor->id,
            actor->tileId, action.abilityId, -resourceCost});
        if (actor->firstResourceSpendRefundAvailable)
        {
            actor->firstResourceSpendRefundAvailable = false;
            const CharacterDefinition& actorDefinition = definitions_.characters.at(actor->definitionId);
            const std::int32_t refund = std::max(0, static_cast<std::int32_t>(
                PassiveValue(actorDefinition, "FirstChainSpendInitiativeRefund")));
            actor->initiative += refund;
            if (refund > 0)
            {
                result.events.push_back(GameEvent{EventType::InitiativeChanged, actor->id, actor->id,
                    actor->tileId, action.abilityId, refund});
            }
        }
    }
    if (abilityState->usesCharges)
    {
        --abilityState->currentCharges;
        result.events.push_back(GameEvent{
            EventType::AbilityChargesChanged, actor->id, actor->id, actor->tileId, action.abilityId, -1});
        if (abilityState->currentCharges < abilityState->maxCharges && abilityState->cooldownRemaining <= 0)
        {
            abilityState->cooldownRemaining = abilityState->cooldownDuration;
            result.events.push_back(GameEvent{
                EventType::CooldownChanged, actor->id, actor->id, actor->tileId,
                action.abilityId, abilityState->cooldownDuration});
        }
    }
    else
    {
        abilityState->cooldownRemaining = abilityState->cooldownDuration;
        result.events.push_back(GameEvent{
            EventType::CooldownChanged, actor->id, actor->id, actor->tileId,
            action.abilityId, abilityState->cooldownDuration});
    }

    const auto isDebuffed = [](const CharacterState& character)
    {
        return std::any_of(character.modifierEffects.begin(), character.modifierEffects.end(),
            [](const ModifierEffectState& effect) { return effect.isDebuff; });
    };
    const auto effectDuration = [this](const CharacterState& owner, const std::int32_t base)
    {
        const CharacterDefinition& ownerDefinition = definitions_.characters.at(owner.definitionId);
        const double multiplier = PassiveValue(ownerDefinition, "DebuffDurationMultiplier");
        std::int32_t duration = multiplier > 0.0
            ? std::max(1, static_cast<std::int32_t>(base * multiplier + 0.5)) : std::max(1, base);
        for (const ModifierEffectState& effect : owner.modifierEffects)
        {
            duration += std::max(0, effect.incomingEffectDurationAdd);
        }
        return duration;
    };
    const auto addEffect = [&result](CharacterState& owner, ModifierEffectState effect)
    {
        owner.modifierEffects.erase(std::remove_if(owner.modifierEffects.begin(), owner.modifierEffects.end(),
            [&effect](const ModifierEffectState& existing) { return existing.id == effect.id; }),
            owner.modifierEffects.end());
        const DefinitionId effectId = effect.id;
        const std::int32_t duration = effect.remainingInitiative;
        owner.modifierEffects.push_back(std::move(effect));
        result.events.push_back(GameEvent{EventType::EffectAdded, owner.modifierEffects.back().causerId,
            owner.id, owner.tileId, effectId, duration});
    };
    const auto applyDelay = [this, &result, &action](CharacterState& source, CharacterState& delayed,
        const std::int32_t requested, const DefinitionId& reason)
    {
        const CharacterDefinition& delayedDefinition = definitions_.characters.at(delayed.definitionId);
        const double multiplier = PassiveValue(delayedDefinition, "DelayReceivedMultiplier");
        const std::int32_t amount = multiplier > 0.0
            ? std::max(0, static_cast<std::int32_t>(requested * multiplier + 0.5))
            : std::max(0, requested);
        const std::int32_t before = delayed.initiative;
        delayed.initiative = std::max(0, delayed.initiative - amount);
        if (delayed.initiative != before)
        {
            result.events.push_back(GameEvent{EventType::InitiativeChanged, source.id, delayed.id,
                delayed.tileId, reason.empty() ? action.abilityId : reason, delayed.initiative - before});
        }
    };
    const auto applyDamage = [this, &result, &action](CharacterState& source, CharacterState& damaged,
        const std::int32_t amount, const std::int32_t defenseIgnore, const DefinitionId& reason)
    {
        std::vector<GameEvent> auraEvents;
        std::vector<GameEvent> modifierEvents;
        const std::int32_t applied = ResolveDamage(source, damaged, std::max(0, amount),
            std::max(0, defenseIgnore), auraEvents, modifierEvents);
        result.events.push_back(GameEvent{EventType::DamageApplied, source.id, damaged.id, damaged.tileId,
            reason.empty() ? action.abilityId : reason, applied});
        result.events.insert(result.events.end(), auraEvents.begin(), auraEvents.end());
        result.events.insert(result.events.end(), modifierEvents.begin(), modifierEvents.end());
        return applied;
    };
    const auto finalizeDeath = [this, &result, &action](CharacterState& source, CharacterState& damaged)
    {
        if (damaged.health > 0 || !damaged.alive) return;
        damaged.health = 0;
        damaged.alive = false;
        if (TileState* tile = FindTile(damaged.tileId)) tile->occupantId = InvalidEntityId;
        result.events.push_back(GameEvent{EventType::CharacterDied, source.id, damaged.id,
            damaged.tileId, action.abilityId, 0});
    };

    if (definition.operation == AbilityOperation::GroundAura)
    {
        ModifierEffectState effect;
        effect.id = action.abilityId + ":GroundAura";
        effect.causerId = actor->id;
        effect.remainingInitiative = std::max(
            1, static_cast<std::int32_t>(AbilityValue(definition, "DurationInitiative")));
        effect.areaCenterTileId = action.targetTileId;
        effect.areaRadius = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "AuraRadius")));
        effect.areaDamage = std::max(0, definition.amount);
        effect.areaDamageType = definition.damageType;
        effect.areaInitiativeLossPerPulse = std::max(
            0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeLossPerPulse")));
        effect.areaFollowsCauser = AbilityValue(definition, "FollowCaster") > 0.0;
        actor->modifierEffects.erase(std::remove_if(actor->modifierEffects.begin(), actor->modifierEffects.end(),
            [&effect](const ModifierEffectState& existing) { return existing.id == effect.id; }),
            actor->modifierEffects.end());
        actor->modifierEffects.push_back(effect);
        result.events.push_back(GameEvent{EventType::EffectAdded, actor->id, actor->id,
            effect.areaCenterTileId, effect.id, effect.remainingInitiative});
    }
    else if (definition.operation == AbilityOperation::TargetModifier)
    {
        ModifierEffectState effect;
        effect.id = action.abilityId + ":Modifier";
        effect.causerId = actor->id;
        effect.remainingInitiative = std::max(
            1, static_cast<std::int32_t>(AbilityValue(definition, "DurationInitiative")));
        const double redirectFraction = std::max(AbilityValue(definition, "RedirectPreMitigationFraction"),
            AbilityValue(definition, "RedirectDamageFraction"));
        effect.redirectPreMitigationBasisPoints = std::clamp(
            static_cast<std::int32_t>(redirectFraction * 10000.0 + 0.5), 0, 10000);
        effect.redirectedDamageMultiplierBasisPoints = std::max(0, static_cast<std::int32_t>(
            (1.0 + AbilityValue(definition, "RedirectedDamageMultiplierAdd")) * 10000.0 + 0.5));
        effect.protectorHealFractionBasisPoints = std::max(0, static_cast<std::int32_t>(
            AbilityValue(definition, "PaladinHealFractionFromRedirect") * 10000.0 + 0.5));
        effect.protectedInitiativePerHit = std::max(0,
            static_cast<std::int32_t>(AbilityValue(definition, "ProtectedAllyInitiativePerHit")));
        target->modifierEffects.erase(std::remove_if(target->modifierEffects.begin(), target->modifierEffects.end(),
            [&effect](const ModifierEffectState& existing) { return existing.id == effect.id; }),
            target->modifierEffects.end());
        target->modifierEffects.push_back(effect);
        result.events.push_back(GameEvent{EventType::EffectAdded, actor->id, target->id,
            target->tileId, effect.id, effect.remainingInitiative});
    }
    else if (definition.operation == AbilityOperation::SelfModifier)
    {
        ModifierEffectState effect;
        effect.id = action.abilityId + ":Modifier";
        effect.causerId = actor->id;
        effect.remainingInitiative = std::max(
            1, static_cast<std::int32_t>(AbilityValue(definition, "DurationInitiative")));
        const double incomingMultiplier = AbilityValue(definition, "SelfIncomingDamageMultiplier");
        const double outgoingMultiplier = AbilityValue(definition, "OutgoingDamageMultiplier");
        effect.incomingDamageMultiplierBasisPoints = std::max(
            0, static_cast<std::int32_t>((incomingMultiplier > 0.0 ? incomingMultiplier : 1.0) * 10000.0 + 0.5));
        effect.outgoingDamageMultiplierBasisPoints = std::max(
            0, static_cast<std::int32_t>((outgoingMultiplier > 0.0 ? outgoingMultiplier : 1.0) * 10000.0 + 0.5));
        effect.defenseIgnoreAdd = std::max(
            0, static_cast<std::int32_t>(AbilityValue(definition, "DefenseIgnore")));
        effect.healFractionFromDamageDealtBasisPoints = std::max(
            0, static_cast<std::int32_t>(AbilityValue(definition, "HealFractionFromDamageDealt") * 10000.0 + 0.5));
        effect.moveCostAdd = static_cast<std::int32_t>(AbilityValue(definition, "MoveCostAdd"));
        actor->modifierEffects.erase(std::remove_if(actor->modifierEffects.begin(), actor->modifierEffects.end(),
            [&effect](const ModifierEffectState& existing) { return existing.id == effect.id; }),
            actor->modifierEffects.end());
        actor->modifierEffects.push_back(effect);
        result.events.push_back(GameEvent{EventType::EffectAdded, actor->id, actor->id,
            actor->tileId, effect.id, effect.remainingInitiative});
    }
    else if (definition.operation == AbilityOperation::Knockback)
    {
        std::int32_t damage = std::max(0, definition.amount);
        if (isDebuffed(*target))
        {
            const double multiplier = AbilityValue(definition, "DamageMultiplierVsDebuffed");
            if (multiplier > 0.0) damage = std::max(0, static_cast<std::int32_t>(damage * multiplier + 0.5));
        }
        applyDamage(*actor, *target, damage, definition.defenseIgnore, {});
        if (target->health > 0)
        {
            const CharacterDefinition& targetDefinition = definitions_.characters.at(target->definitionId);
            const std::int32_t ignoreChance = std::clamp(static_cast<std::int32_t>(
                PassiveValue(targetDefinition, "IgnoreEnemyForcedMovementChance") * 10000.0 + 0.5), 0, 10000);
            const bool ignored = ignoreChance > 0 && RollBasisPoints(ignoreChance);
            const std::int32_t requestedDistance = std::max(0,
                static_cast<std::int32_t>(AbilityValue(definition, "KnockbackDistance")));
            std::int32_t moved = 0;
            while (!ignored && moved < requestedDistance)
            {
                const TileState* sourceTile = FindTile(actor->tileId);
                const TileState* currentTile = FindTile(target->tileId);
                TileState* best = nullptr;
                std::int32_t bestDistance = sourceTile && currentTile ? HexDistance(sourceTile->coord, currentTile->coord) : -1;
                for (TileState& candidate : state_.tiles)
                {
                    const auto terrain = definitions_.terrains.find(candidate.terrainId);
                    if (!sourceTile || !currentTile || HexDistance(currentTile->coord, candidate.coord) != 1
                        || candidate.occupantId != InvalidEntityId || candidate.blocksMovement
                        || terrain == definitions_.terrains.end() || !terrain->second.walkable) continue;
                    const std::int32_t distance = HexDistance(sourceTile->coord, candidate.coord);
                    if (distance > bestDistance || (distance == bestDistance && (!best || candidate.id < best->id)))
                    {
                        bestDistance = distance; best = &candidate;
                    }
                }
                if (!best) break;
                if (TileState* old = FindTile(target->tileId)) old->occupantId = InvalidEntityId;
                best->occupantId = target->id; target->tileId = best->id; ++moved;
                result.events.push_back(GameEvent{EventType::CharacterMoved, target->id, target->id,
                    best->id, action.abilityId, 0});
            }
            if (moved < requestedDistance)
            {
                applyDamage(*actor, *target, std::max(0, static_cast<std::int32_t>(
                    AbilityValue(definition, "WallBonusDamage"))), definition.defenseIgnore, action.abilityId + ":Wall");
            }
            applyDelay(*actor, *target, std::max(0, static_cast<std::int32_t>(
                AbilityValue(definition, "TargetInitiativeLossOnHit"))), {});
            const std::int32_t selfSteps = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "SelfStepTowardTarget")));
            for (std::int32_t step = 0; step < selfSteps; ++step)
            {
                TileState* actorTile = FindTile(actor->tileId);
                const TileState* targetTile = FindTile(target->tileId);
                TileState* best = nullptr;
                std::int32_t bestDistance = actorTile && targetTile ? HexDistance(actorTile->coord, targetTile->coord) : 0;
                for (TileState& candidate : state_.tiles)
                {
                    const auto terrain = definitions_.terrains.find(candidate.terrainId);
                    if (!actorTile || !targetTile || HexDistance(actorTile->coord, candidate.coord) != 1
                        || candidate.occupantId != InvalidEntityId || candidate.blocksMovement
                        || terrain == definitions_.terrains.end() || !terrain->second.walkable) continue;
                    const std::int32_t distance = HexDistance(candidate.coord, targetTile->coord);
                    if (distance < bestDistance && (!best || distance < HexDistance(best->coord, targetTile->coord)
                        || (distance == HexDistance(best->coord, targetTile->coord) && candidate.id < best->id))) best = &candidate;
                }
                if (!best) break;
                actorTile->occupantId = InvalidEntityId; best->occupantId = actor->id; actor->tileId = best->id;
                actor->movedSinceLastTurn = true; actor->stationaryGuardActive = false;
                result.events.push_back(GameEvent{EventType::CharacterMoved, actor->id, actor->id,
                    best->id, action.abilityId + ":Follow", 0});
            }
            const double slow = AbilityValue(definition, "ApplySlowOnHitStrength");
            if (slow > 0.0 && target->health > 0)
            {
                ModifierEffectState effect; effect.id = action.abilityId + ":Slow"; effect.causerId = actor->id;
                effect.isDebuff = true; effect.remainingInitiative = effectDuration(*target, std::max(1,
                    static_cast<std::int32_t>(AbilityValue(definition, "ApplySlowOnHitDuration") * 40.0)));
                effect.moveCostMultiplierBasisPoints = std::max(10000,
                    static_cast<std::int32_t>((1.0 + slow) * 10000.0 + 0.5));
                addEffect(*target, effect);
            }
            const double targetIncoming = AbilityValue(definition, "IncomingDamageMultiplierOnHit");
            if (targetIncoming > 0.0 && targetIncoming != 1.0)
            {
                ModifierEffectState effect; effect.id = action.abilityId + ":Exposed"; effect.causerId = actor->id;
                effect.isDebuff = true; effect.remainingInitiative = effectDuration(*target, 40);
                effect.incomingDamageMultiplierBasisPoints = static_cast<std::int32_t>(targetIncoming * 10000.0 + 0.5);
                addEffect(*target, effect);
            }
            const double actorOutgoing = AbilityValue(definition, "OutgoingDamageMultiplierAfterHit");
            if (actorOutgoing > 0.0 && actorOutgoing != 1.0)
            {
                ModifierEffectState effect; effect.id = action.abilityId + ":Momentum"; effect.causerId = actor->id;
                effect.remainingInitiative = 40;
                effect.outgoingDamageMultiplierBasisPoints = static_cast<std::int32_t>(actorOutgoing * 10000.0 + 0.5);
                addEffect(*actor, effect);
            }
            const double selfIncoming = AbilityValue(definition, "SelfIncomingDamageMultiplierFromTarget");
            if (selfIncoming > 0.0 && selfIncoming != 1.0)
            {
                ModifierEffectState effect; effect.id = action.abilityId + ":Guard"; effect.causerId = target->id;
                effect.onlyAffectCauserDamage = true; effect.incomingDamageMultiplierBasisPoints =
                    static_cast<std::int32_t>(selfIncoming * 10000.0 + 0.5);
                effect.remainingInitiative = std::max(1, static_cast<std::int32_t>(
                    AbilityValue(definition, "SelfIncomingDamageMultiplierFromTargetDuration") * 40.0));
                addEffect(*actor, effect);
            }
        }
        finalizeDeath(*actor, *target);
    }
    else if (definition.operation == AbilityOperation::DelayStrike)
    {
        const CharacterDefinition& targetDefinition = definitions_.characters.at(target->definitionId);
        std::int32_t damage = definition.amount;
        const double highInitiativeMultiplier = AbilityValue(definition, "OutgoingDamageMultiplierOnHighInitiativeTarget");
        if (highInitiativeMultiplier > 0.0 && target->initiative >= targetDefinition.stats.maxInitiative)
        {
            damage = std::max(0, static_cast<std::int32_t>(damage * highInitiativeMultiplier + 0.5));
        }
        applyDamage(*actor, *target, damage, definition.defenseIgnore, {});
        const bool alreadyDebuffed = isDebuffed(*target);
        const bool alreadyDelayed = std::any_of(target->modifierEffects.begin(), target->modifierEffects.end(),
            [&action](const ModifierEffectState& effect) { return effect.id == action.abilityId + ":Delayed"; });
        std::int32_t delay = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DelayAmount")));
        if (alreadyDebuffed) delay += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ExtraDelayVsDebuffed")));
        if (alreadyDelayed) delay += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "AdditionalInitiativeLossIfAlreadyDelayed")));
        applyDelay(*actor, *target, delay, {});
        const std::int32_t radius = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "AoEInitiativeLossRadius")));
        const std::int32_t areaLoss = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "AoEInitiativeLossAmount")));
        const TileState* center = FindTile(target->tileId);
        if (center && radius > 0 && areaLoss > 0)
        {
            for (CharacterState& other : state_.characters)
            {
                const TileState* tile = FindTile(other.tileId);
                if (other.alive && other.team != actor->team && other.id != target->id && tile
                    && HexDistance(center->coord, tile->coord) <= radius) applyDelay(*actor, other, areaLoss, action.abilityId + ":Area");
            }
        }
        const std::int32_t selfGain = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "SelfInitiativeGainOnUse")));
        actor->initiative += selfGain;
        if (selfGain > 0) result.events.push_back(GameEvent{EventType::InitiativeChanged, actor->id, actor->id,
            actor->tileId, action.abilityId, selfGain});
        if (target->health > 0)
        {
            ModifierEffectState marker; marker.id = action.abilityId + ":Delayed"; marker.causerId = actor->id;
            marker.isDebuff = true; marker.remainingInitiative = effectDuration(*target, 40);
            marker.incomingEffectDurationAdd = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "IncomingEffectDurationAdd") * 40.0));
            marker.moveCostAdd = static_cast<std::int32_t>(AbilityValue(definition, "MoveCostAddOnTarget"));
            marker.blockMovementBonuses = AbilityValue(definition, "BlockMovementBonusesOnTarget") > 0.0;
            addEffect(*target, marker);
        }
        finalizeDeath(*actor, *target);
    }
    else if (definition.operation == AbilityOperation::Shackle)
    {
        applyDamage(*actor, *target, definition.amount, definition.defenseIgnore, {});
        if (target->health > 0)
        {
            ModifierEffectState effect; effect.id = action.abilityId + ":Shackle"; effect.causerId = actor->id;
            effect.isDebuff = true; effect.remainingInitiative = effectDuration(*target, std::max(1,
                static_cast<std::int32_t>(AbilityValue(definition, "SlowDuration") * 40.0)));
            const double slow = AbilityValue(definition, "SlowStrength");
            effect.moveCostMultiplierBasisPoints = std::max(10000, static_cast<std::int32_t>((1.0 + slow) * 10000.0 + 0.5));
            const double outgoing = AbilityValue(definition, "OutgoingDamageMultiplierWhileShackled");
            effect.outgoingDamageMultiplierBasisPoints = outgoing > 0.0 ? static_cast<std::int32_t>(outgoing * 10000.0 + 0.5) : 10000;
            effect.initiativeLossOnMove = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeLossOnMove")));
            effect.blockMovementBonuses = AbilityValue(definition, "BlockMovementBonuses") > 0.0;
            const double incoming = AbilityValue(definition, "IncomingDamageMultiplierFromCauserWhileShackled");
            effect.incomingDamageMultiplierFromCauserBasisPoints = incoming > 0.0 ? static_cast<std::int32_t>(incoming * 10000.0 + 0.5) : 10000;
            effect.initiativeLossOnExpire = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeLossOnExpire")));
            addEffect(*target, effect);
            const double selfIncoming = AbilityValue(definition, "SelfIncomingDamageMultiplierFromTargetWhileShackled");
            if (selfIncoming > 0.0)
            {
                ModifierEffectState protection; protection.id = action.abilityId + ":Protection"; protection.causerId = target->id;
                protection.remainingInitiative = effect.remainingInitiative; protection.onlyAffectCauserDamage = true;
                protection.incomingDamageMultiplierBasisPoints = static_cast<std::int32_t>(selfIncoming * 10000.0 + 0.5);
                addEffect(*actor, protection);
            }
        }
        finalizeDeath(*actor, *target);
    }
    else if (definition.operation == AbilityOperation::Bind)
    {
        ModifierEffectState effect; effect.id = action.abilityId + ":Bind"; effect.causerId = actor->id;
        effect.isDebuff = true; effect.triggerOnMove = true; effect.remainingInitiative = effectDuration(*target,
            std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "DurationInitiative"))));
        effect.moveCostAdd = 2; effect.blockMovementBonuses = AbilityValue(definition, "BlockMovementBonusesOnTarget") > 0.0;
        effect.triggerDamageOnMove = std::max(0, definition.amount);
        const double triggerVsDebuffed = AbilityValue(definition, "TriggerDamageMultiplierVsDebuffed");
        if (isDebuffed(*target) && triggerVsDebuffed > 0.0)
        {
            effect.triggerDamageOnMove = std::max(0, static_cast<std::int32_t>(
                effect.triggerDamageOnMove * triggerVsDebuffed + 0.5));
        }
        effect.triggerDelayOnMove = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DelayAmount")));
        const double outgoing = AbilityValue(definition, "TriggerOutgoingDamageMultiplier");
        effect.triggerOutgoingDamageMultiplierBasisPoints = outgoing > 0.0 ? static_cast<std::int32_t>(outgoing * 10000.0 + 0.5) : 10000;
        const double slow = AbilityValue(definition, "TriggerSlowAmount");
        effect.triggerSlowMultiplierBasisPoints = std::max(10000, static_cast<std::int32_t>((1.0 + slow) * 10000.0 + 0.5));
        effect.triggerSlowDurationInitiative = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "TriggerSlowDuration") * 40.0));
        effect.triggerAreaRadius = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "TriggerAoEInitiativeLossRadius")));
        effect.triggerAreaInitiativeLoss = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "TriggerAoEInitiativeLossAmount")));
        effect.triggerInitiativeGainForCauser = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "TriggerInitiativeGainForCauser")));
        addEffect(*target, effect);
    }
    else if (definition.operation == AbilityOperation::Taunt)
    {
        applyDamage(*actor, *target, definition.amount, definition.defenseIgnore, {});
        if (target->health > 0)
        {
            ModifierEffectState effect; effect.id = action.abilityId + ":Taunt"; effect.causerId = actor->id;
            effect.preferredTargetId = actor->id; effect.isDebuff = true;
            const bool debuffed = isDebuffed(*target);
            effect.remainingInitiative = effectDuration(*target, std::max(1,
                static_cast<std::int32_t>(AbilityValue(definition, "DurationInitiative"))
                + (debuffed ? static_cast<std::int32_t>(AbilityValue(definition, "ExtraDurationVsDebuffed") * 40.0) : 0)));
            const double reduction = AbilityValue(definition, "ReductionAmount");
            const double explicitMultiplier = AbilityValue(definition, "OutgoingDamageMultiplierWhileTaunted");
            const double multiplier = explicitMultiplier > 0.0 ? explicitMultiplier : std::max(0.0, 1.0 - reduction);
            effect.outgoingDamageMultiplierAgainstOtherTargetsBasisPoints = static_cast<std::int32_t>(multiplier * 10000.0 + 0.5);
            effect.initiativeLossWhenIgnoringPreferredTarget = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeLossWhenIgnoringCauser")));
            effect.initiativeGainForCauserWhenAttacked = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeGainForCauserWhenAttacked")));
            const double fromCauser = AbilityValue(definition, "IncomingDamageMultiplierFromCauserWhileTaunted");
            effect.incomingDamageMultiplierFromCauserBasisPoints = fromCauser > 0.0 ? static_cast<std::int32_t>(fromCauser * 10000.0 + 0.5) : 10000;
            const double allIncoming = AbilityValue(definition, "IncomingDamageMultiplierWhileTaunted");
            effect.incomingDamageMultiplierBasisPoints = allIncoming > 0.0 ? static_cast<std::int32_t>(allIncoming * 10000.0 + 0.5) : 10000;
            addEffect(*target, effect);
            applyDelay(*actor, *target, std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ImmediateInitiativeLoss"))), {});
            const double selfIncoming = AbilityValue(definition, "SelfIncomingDamageMultiplierFromTargetWhileTaunted");
            if (selfIncoming > 0.0)
            {
                ModifierEffectState protection; protection.id = action.abilityId + ":Protection"; protection.causerId = target->id;
                protection.remainingInitiative = effect.remainingInitiative; protection.onlyAffectCauserDamage = true;
                protection.incomingDamageMultiplierBasisPoints = static_cast<std::int32_t>(selfIncoming * 10000.0 + 0.5);
                addEffect(*actor, protection);
            }
        }
        finalizeDeath(*actor, *target);
    }
    else if (definition.operation == AbilityOperation::Concentrate)
    {
        ModifierEffectState focus; focus.id = action.abilityId + ":Concentrate"; focus.sourceAbilityId = action.abilityId;
        focus.kind = ModifierEffectKind::Concentrate; focus.causerId = actor->id;
        const CharacterDefinition& actorDefinition = definitions_.characters.at(actor->definitionId);
        const std::int32_t missing = std::max(0, actorDefinition.stats.maxInitiative - actor->initiative);
        double multiplier = AbilityValue(definition, "ConcentrateBonusMultiplier");
        if (multiplier <= 0.0) multiplier = 1.0;
        multiplier *= 1.0 + missing * AbilityValue(definition, "DamagePerInitative");
        focus.outgoingDamageMultiplierBasisPoints = std::max(0, static_cast<std::int32_t>(multiplier * 10000.0 + 0.5));
        const bool unlimited = AbilityValue(definition, "UnlimitedUntilMove") > 0.0;
        focus.remainingInitiative = unlimited ? 0 : std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "ConcentrateDurationInitiative")));
        focus.remainingTurns = unlimited ? std::numeric_limits<std::int32_t>::max() : 0;
        focus.counterA = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "EmpoweredHitCount")));
        focus.valueA = static_cast<std::int32_t>(AbilityValue(definition, "SecondHitBonusFraction") * 10000.0 + 0.5);
        focus.valueB = focus.outgoingDamageMultiplierBasisPoints;
        focus.valueC = actor->resourceCurrent <= 0 ? std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ReloadAmmoAfterDelay"))) : 0;
        focus.flags = (AbilityValue(definition, "ExpireOnMove") > 0.0 ? 1u : 0u)
            | (AbilityValue(definition, "BasicAttackConsumesNoAmmo") > 0.0 ? 2u : 0u)
            | (AbilityValue(definition, "PersistOnKill") > 0.0 ? 4u : 0u)
            | (AbilityValue(definition, "DumpMagazineAfterBasic") > 0.0 ? 8u : 0u)
            | (AbilityValue(definition, "BlockAmmoRegeneration") > 0.0 ? 16u : 0u);
        addEffect(*actor, focus);
    }
    else if (definition.operation == AbilityOperation::Aim)
    {
        const std::int32_t ammo = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "AmmoCost")));
        if (ammo > 0)
        {
            actor->resourceCurrent -= ammo; actor->spentResourceThisTurn = true; actor->attackedThisTurn = true;
            result.events.push_back(GameEvent{EventType::ResourceChanged, actor->id, actor->id, actor->tileId, action.abilityId, -ammo});
            if (actor->resourceCurrent == 0 && !actor->triggeredEmptyResourceThisTurn)
            {
                actor->triggeredEmptyResourceThisTurn = true;
                const CharacterDefinition& actorDefinition = definitions_.characters.at(actor->definitionId);
                const std::int32_t gain = std::max(0, static_cast<std::int32_t>(
                    PassiveValue(actorDefinition, "EmptyMagazineInitiativeGain")));
                actor->initiative += gain;
                if (gain > 0) result.events.push_back(GameEvent{EventType::InitiativeChanged,
                    actor->id, actor->id, actor->tileId, "EmptyMagazine", gain});
            }
        }
        ModifierEffectState field; field.id = action.abilityId + ":AimField:" + std::to_string(state_.step);
        field.sourceAbilityId = action.abilityId; field.kind = ModifierEffectKind::AimField; field.causerId = actor->id;
        field.areaCenterTileId = action.targetTileId;
        field.areaRadius = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DamageRange")));
        const double triggerMultiplier = AbilityValue(definition, "TriggerDamageMultiplier");
        field.areaDamage = static_cast<std::int32_t>(definition.amount * (triggerMultiplier > 0.0 ? triggerMultiplier : 1.0) + 0.5);
        field.valueA = std::max(0, definition.defenseIgnore);
        field.valueB = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "TriggerInitiativeLoss")));
        field.valueC = static_cast<std::int32_t>(actor->tileId);
        field.counterA = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "PersistTriggerCount")));
        field.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "AimFieldDurationInitiative")));
        const std::int32_t maximumFields = std::max(1, abilityState->maxCharges);
        while (static_cast<std::int32_t>(std::count_if(actor->modifierEffects.begin(), actor->modifierEffects.end(),
            [&action](const ModifierEffectState& effect)
            { return effect.kind == ModifierEffectKind::AimField && effect.sourceAbilityId == action.abilityId; })) >= maximumFields)
        {
            const auto oldest = std::find_if(actor->modifierEffects.begin(), actor->modifierEffects.end(),
                [&action](const ModifierEffectState& effect)
                { return effect.kind == ModifierEffectKind::AimField && effect.sourceAbilityId == action.abilityId; });
            if (oldest == actor->modifierEffects.end()) break;
            result.events.push_back(GameEvent{EventType::EffectRemoved, actor->id, actor->id,
                oldest->areaCenterTileId, oldest->id, 0});
            actor->modifierEffects.erase(oldest);
        }
        actor->modifierEffects.push_back(field);
        result.events.push_back(GameEvent{EventType::EffectAdded, actor->id, actor->id, field.areaCenterTileId, field.id, field.remainingInitiative});
    }
    else if (definition.operation == AbilityOperation::PiercingShot)
    {
        const TileState* sourceTile = FindTile(actor->tileId); const TileState* endTile = FindTile(target->tileId);
        const std::int32_t lineDistance = sourceTile && endTile ? HexDistance(sourceTile->coord, endTile->coord) : 0;
        const bool resetRangeOnHit = AbilityValue(definition, "ResetRangeOnEveryHit") > 0.0;
        const bool redirectAfterFirstHit = AbilityValue(definition, "RedirectAfterFirstHit") > 0.0;
        const std::int32_t fullRange = std::max(1, definition.range);
        std::vector<TileState*> lineTiles;
        if (sourceTile && endTile)
        {
            const std::int32_t rayQ = endTile->coord.q - sourceTile->coord.q;
            const std::int32_t rayR = endTile->coord.r - sourceTile->coord.r;
            for (TileState& tile : state_.tiles)
            {
                const std::int32_t dq = tile.coord.q - sourceTile->coord.q;
                const std::int32_t dr = tile.coord.r - sourceTile->coord.r;
                if ((tile.fieldObjectHealth > 0 || tile.terrainObjectHealth > 0)
                    && dq * rayR == dr * rayQ && dq * rayQ + dr * rayR > 0)
                {
                    lineTiles.push_back(&tile);
                }
            }
            std::sort(lineTiles.begin(), lineTiles.end(), [sourceTile](const TileState* left, const TileState* right)
            { return HexDistance(sourceTile->coord, left->coord) < HexDistance(sourceTile->coord, right->coord); });
        }
        std::vector<CharacterState*> hits;
        for (CharacterState& candidate : state_.characters)
        {
            const TileState* tile = FindTile(candidate.tileId);
            if (candidate.alive && candidate.team != actor->team && candidate.team != Team::Neutral && tile && sourceTile && endTile
                && HexDistance(sourceTile->coord, tile->coord) + HexDistance(tile->coord, endTile->coord) == lineDistance) hits.push_back(&candidate);
        }
        std::sort(hits.begin(), hits.end(), [sourceTile, this](const CharacterState* a, const CharacterState* b)
        { return HexDistance(sourceTile->coord, FindTile(a->tileId)->coord) < HexDistance(sourceTile->coord, FindTile(b->tileId)->coord); });
        if (resetRangeOnHit && sourceTile && endTile)
        {
            const std::int32_t rayQ = endTile->coord.q - sourceTile->coord.q;
            const std::int32_t rayR = endTile->coord.r - sourceTile->coord.r;
            std::vector<CharacterState*> rayCandidates;
            for (CharacterState& candidate : state_.characters)
            {
                const TileState* tile = FindTile(candidate.tileId);
                if (!candidate.alive || candidate.team == actor->team || candidate.team == Team::Neutral || !tile) continue;
                const std::int32_t dq = tile->coord.q - sourceTile->coord.q;
                const std::int32_t dr = tile->coord.r - sourceTile->coord.r;
                if (dq * rayR == dr * rayQ && dq * rayQ + dr * rayR > 0) rayCandidates.push_back(&candidate);
            }
            std::sort(rayCandidates.begin(), rayCandidates.end(), [sourceTile, this](const CharacterState* a, const CharacterState* b)
            { return HexDistance(sourceTile->coord, FindTile(a->tileId)->coord) < HexDistance(sourceTile->coord, FindTile(b->tileId)->coord); });
            const TileState* lastHitTile = sourceTile;
            for (CharacterState* candidate : rayCandidates)
            {
                const TileState* tile = FindTile(candidate->tileId);
                if (tile && HexDistance(lastHitTile->coord, tile->coord) <= fullRange)
                {
                    if (std::find(hits.begin(), hits.end(), candidate) == hits.end()) hits.push_back(candidate);
                    lastHitTile = tile;
                }
            }
        }
        if (redirectAfterFirstHit && !hits.empty())
        {
            const TileState* firstTile = FindTile(hits.front()->tileId);
            const std::int32_t spentRange = sourceTile && firstTile ? HexDistance(sourceTile->coord, firstTile->coord) : fullRange;
            const std::int32_t remainingRange = std::max(0, fullRange - spentRange);
            CharacterState* nearest = nullptr;
            std::int32_t nearestDistance = remainingRange + 1;
            for (CharacterState& candidate : state_.characters)
            {
                const TileState* tile = FindTile(candidate.tileId);
                if (!candidate.alive || candidate.team == actor->team || candidate.team == Team::Neutral
                    || !tile || !firstTile || std::find(hits.begin(), hits.end(), &candidate) != hits.end()) continue;
                const std::int32_t distance = HexDistance(firstTile->coord, tile->coord);
                if (distance < nearestDistance || (distance == nearestDistance && nearest && candidate.id < nearest->id))
                {
                    nearest = &candidate; nearestDistance = distance;
                }
            }
            if (nearest && nearestDistance <= remainingRange)
            {
                const TileState* nearestTile = FindTile(nearest->tileId);
                std::vector<CharacterState*> redirectedHits;
                for (CharacterState& candidate : state_.characters)
                {
                    const TileState* tile = FindTile(candidate.tileId);
                    if (!candidate.alive || candidate.team == actor->team || candidate.team == Team::Neutral
                        || !tile || !firstTile || !nearestTile
                        || std::find(hits.begin(), hits.end(), &candidate) != hits.end()) continue;
                    if (HexDistance(firstTile->coord, tile->coord) + HexDistance(tile->coord, nearestTile->coord)
                        == nearestDistance) redirectedHits.push_back(&candidate);
                }
                std::sort(redirectedHits.begin(), redirectedHits.end(), [firstTile, this](const CharacterState* left, const CharacterState* right)
                { return HexDistance(firstTile->coord, FindTile(left->tileId)->coord)
                    < HexDistance(firstTile->coord, FindTile(right->tileId)->coord); });
                hits.insert(hits.end(), redirectedHits.begin(), redirectedHits.end());
            }
        }
        bool killedAny = false; std::int32_t totalHits = 0; std::int32_t primaryApplied = 0;
        const auto firePass = [&](const double passMultiplier)
        {
            std::int32_t remainingObjectRange = fullRange;
            const TileState* previousObjectTile = sourceTile;
            std::vector<EntityId> splashVictims;
            for (TileState* objectTile : lineTiles)
            {
                if (!objectTile || !previousObjectTile) continue;
                const std::int32_t gap = HexDistance(previousObjectTile->coord, objectTile->coord);
                if (gap > remainingObjectRange) break;
                remainingObjectRange -= gap;
                const std::int32_t objectDamage = std::max(0,
                    static_cast<std::int32_t>(definition.amount * passMultiplier + 0.5));
                bool destroyedObject = false;
                bool hitObjectOnTile = false;
                const auto damageObject = [&](std::int32_t& health, const char* suffix)
                {
                    if (health <= 0) return;
                    const std::int32_t applied = std::min(health, objectDamage);
                    health -= applied; ++totalHits; hitObjectOnTile = true;
                    result.events.push_back(GameEvent{EventType::DamageApplied, actor->id, InvalidEntityId,
                        objectTile->id, action.abilityId + suffix, applied});
                    destroyedObject = destroyedObject || health <= 0;
                };
                damageObject(objectTile->fieldObjectHealth, ":FieldObject");
                damageObject(objectTile->terrainObjectHealth, ":TerrainObject");
                if (objectTile->fieldObjectHealth <= 0) objectTile->blocksMovement = false;
                if (destroyedObject)
                {
                    const double splashMultiplier = AbilityValue(definition, "DestroyedObjectSplashDamageMultiplier");
                    if (splashMultiplier > 0.0)
                    {
                        for (CharacterState& candidate : state_.characters)
                        {
                            const TileState* candidateTile = FindTile(candidate.tileId);
                            if (!candidate.alive || candidate.team == actor->team || candidate.team == Team::Neutral
                                || !candidateTile || HexDistance(objectTile->coord, candidateTile->coord) != 1
                                || std::find(splashVictims.begin(), splashVictims.end(), candidate.id) != splashVictims.end()) continue;
                            splashVictims.push_back(candidate.id);
                            applyDamage(*actor, candidate, static_cast<std::int32_t>(
                                definition.amount * passMultiplier * splashMultiplier + 0.5), 0,
                                action.abilityId + ":ObjectSplash");
                            if (candidate.health <= 0) killedAny = true;
                            finalizeDeath(*actor, candidate);
                        }
                    }
                }
                if (resetRangeOnHit && hitObjectOnTile) remainingObjectRange = fullRange;
                previousObjectTile = objectTile;
            }
            std::int32_t hitIndex = 0;
            for (CharacterState* hit : hits)
            {
                if (!hit->alive) continue;
                std::int32_t amount = static_cast<std::int32_t>(definition.amount * passMultiplier + 0.5);
                if (hitIndex++ > 0 && AbilityValue(definition, "LaterTargetDamageMultiplier") > 0.0)
                    amount = static_cast<std::int32_t>(amount * AbilityValue(definition, "LaterTargetDamageMultiplier") + 0.5);
                const std::int32_t applied = applyDamage(*actor, *hit, amount, definition.defenseIgnore, {}); ++totalHits;
                if (primaryApplied == 0) primaryApplied = applied;
                applyDelay(*actor, *hit, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeLossOnHit")), action.abilityId);
                if (hit->health <= 0) killedAny = true;
                else
                {
                    const std::int32_t markDuration = std::max(static_cast<std::int32_t>(AbilityValue(definition, "TargetMarkDurationInitiative")),
                        static_cast<std::int32_t>(AbilityValue(definition, "ChainMarkDurationInitiative")));
                    if (markDuration > 0)
                    {
                        ModifierEffectState mark; mark.id = action.abilityId + ":PiercingMark"; mark.sourceAbilityId = action.abilityId;
                        mark.kind = ModifierEffectKind::PiercingMark; mark.causerId = actor->id; mark.isDebuff = true;
                        mark.remainingInitiative = markDuration; addEffect(*hit, mark);
                    }
                }
                finalizeDeath(*actor, *hit);
                if (applied > 0 && hitIndex == 1 && AbilityValue(definition, "KnockbackFirstTargetDistance") > 0.0 && hit->alive && sourceTile)
                {
                    TileState* current = FindTile(hit->tileId); TileState* push = nullptr; std::int32_t farthest = current ? HexDistance(sourceTile->coord, current->coord) : 0;
                    for (TileState& tile : state_.tiles) if (current && tile.occupantId == InvalidEntityId && !tile.blocksMovement
                        && HexDistance(tile.coord, current->coord) == 1 && HexDistance(sourceTile->coord, tile.coord) > farthest) { push = &tile; farthest = HexDistance(sourceTile->coord, tile.coord); }
                    if (push && current) { current->occupantId = InvalidEntityId; push->occupantId = hit->id; hit->tileId = push->id; }
                }
            }
        };
        firePass(1.0);
        ResolveAmmoShot(*actor, killedAny, false, action.abilityId, result.events);
        const double repeat = AbilityValue(definition, "RepeatShotDamageMultiplier");
        if (repeat > 0.0 && actor->resourceCurrent > 0)
        {
            firePass(repeat);
            ResolveAmmoShot(*actor, killedAny, false, action.abilityId + ":Repeat", result.events);
        }
        const std::int32_t reduction = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DefenseReductionPerHitStack")));
        if (reduction > 0 && totalHits > 0)
        {
            ModifierEffectState prep; prep.id = action.abilityId + ":DefensePreparation"; prep.sourceAbilityId = action.abilityId;
            prep.kind = ModifierEffectKind::PiercingMark; prep.causerId = actor->id; prep.counterA = totalHits; prep.valueA = reduction;
            prep.valueB = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "DefenseReductionDurationInitiative")));
            prep.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "PreparationDurationInitiative"))); addEffect(*actor, prep);
        }
        if (!hits.empty()) ResolveConcentrateHit(*actor, *hits.front(), killedAny, false,
            abilityState, primaryApplied, result.events);
    }
    else if (definition.operation == AbilityOperation::SuppressingFire)
    {
        const bool alreadyDebuffed = isDebuffed(*target);
        const std::int32_t suppressingApplied = applyDamage(*actor, *target,
            definition.amount, definition.defenseIgnore, {});
        const bool killed = target->health <= 0;
        if (!killed)
        {
            ModifierEffectState suppression; suppression.id = action.abilityId + ":Suppression";
            suppression.sourceAbilityId = action.abilityId; suppression.kind = ModifierEffectKind::SuppressingFire;
            suppression.causerId = actor->id; suppression.isDebuff = true; suppression.remainingInitiative = effectDuration(*target,
                std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "SuppressionDurationInitiative"))));
            suppression.moveCostAdd = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "MoveCostAddOnTarget")));
            suppression.basicAttackCostAdd = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "BasicAttackCostAdd")));
            suppression.basicAttackCostAvailable = suppression.basicAttackCostAdd > 0;
            suppression.defenseIgnoreAdd = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DefenseReduction")));
            suppression.preventsMovement = AbilityValue(definition, "RootUntilDamaged") > 0.0;
            suppression.counterA = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "CooldownReductionOnMove")));
            suppression.valueA = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "AmmoOnKill")));
            suppression.valueB = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ChainRange")));
            suppression.valueC = static_cast<std::int32_t>(AbilityValue(definition, "ReactionDamageMultiplier") * 10000.0 + 0.5);
            suppression.flags = (AbilityValue(definition, "ChainOnKill") > 0.0 ? 1u : 0u)
                | (AbilityValue(definition, "FreeFollowupAfterAllyHit") > 0.0 ? 2u : 0u)
                | (AbilityValue(definition, "RevealToAllTeams") > 0.0 ? 4u : 0u);
            addEffect(*target, suppression);
        }
        if (!(alreadyDebuffed && AbilityValue(definition, "FreeAmmoVsDebuffed") > 0.0))
        {
            ResolveAmmoShot(*actor, killed, false, action.abilityId, result.events);
        }
        else actor->attackedThisTurn = true;
        finalizeDeath(*actor, *target);
        ResolveConcentrateHit(*actor, *target, killed, false, abilityState,
            suppressingApplied, result.events);
    }
    else if (definition.operation == AbilityOperation::Snipe)
    {
        const CharacterDefinition& targetDefinition = definitions_.characters.at(target->definitionId);
        const TileState* actorTile = FindTile(actor->tileId);
        const TileState* targetTile = FindTile(target->tileId);
        const std::int32_t missingInitiative = std::max(0, targetDefinition.stats.maxInitiative - target->initiative);
        std::int32_t damage = std::max(0, definition.amount + static_cast<std::int32_t>(
            missingInitiative * AbilityValue(definition, "InitativeScaling") + 0.5));
        const std::int32_t readyInterval = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "ReadyDamageInitiativeInterval")));
        const double readyPerInterval = AbilityValue(definition, "ReadyDamagePercentPerInterval");
        const double readyCap = AbilityValue(definition, "ReadyDamagePercentCap");
        if (readyPerInterval > 0.0)
        {
            const double bonus = std::min(readyCap > 0.0 ? readyCap : readyPerInterval * 1000.0,
                (abilityState->readyInitiativeElapsed / readyInterval) * readyPerInterval);
            damage = static_cast<std::int32_t>(damage * (1.0 + bonus) + 0.5);
        }
        abilityState->readyInitiativeElapsed = 0;
        bool adjacentAlly = false;
        if (targetTile)
        {
            for (const CharacterState& other : state_.characters)
            {
                const TileState* otherTile = FindTile(other.tileId);
                if (other.alive && other.id != target->id && other.team == target->team && otherTile
                    && HexDistance(targetTile->coord, otherTile->coord) == 1) { adjacentAlly = true; break; }
            }
        }
        const double isolatedMultiplier = AbilityValue(definition, "DamageMultiplierVsIsolated");
        if (!adjacentAlly && isolatedMultiplier > 0.0) damage = static_cast<std::int32_t>(damage * isolatedMultiplier + 0.5);
        const double lowHealthThreshold = AbilityValue(definition, "LowHealthThresholdPercent");
        if (lowHealthThreshold > 0.0 && target->health * 100.0 <= targetDefinition.stats.maxHealth * lowHealthThreshold)
        {
            const double multiplier = AbilityValue(definition, "DamageMultiplierVsLowHealth");
            if (multiplier > 0.0) damage = static_cast<std::int32_t>(damage * multiplier + 0.5);
        }
        std::int32_t ignore = definition.defenseIgnore;
        if (missingInitiative >= static_cast<std::int32_t>(AbilityValue(definition, "RequiredMissingInitiativeForDefenseIgnore")))
            ignore += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ConditionalDefenseIgnore")));
        const std::int32_t ammoBefore = actor->resourceCurrent;
        DefinitionId debuffToExtendId;
        if (AbilityValue(definition, "ExtendDebuffDurationInitiative") > 0.0)
        {
            const ModifierEffectState* shortest = nullptr;
            for (const ModifierEffectState& candidate : target->modifierEffects)
                if (candidate.isDebuff && (!shortest || candidate.remainingInitiative < shortest->remainingInitiative))
                    shortest = &candidate;
            if (shortest) debuffToExtendId = shortest->id;
        }
        const std::int32_t snipeApplied = applyDamage(*actor, *target, damage, ignore, {});
        const bool killed = target->health <= 0;
        if (!killed)
        {
            const std::int32_t defenseReduction = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DefenseReductionOnHit")));
            if (defenseReduction > 0)
            {
                ModifierEffectState effect; effect.id = action.abilityId + ":DefenseBreak"; effect.causerId = actor->id;
                effect.isDebuff = true; effect.defenseIgnoreAdd = defenseReduction;
                effect.remainingInitiative = effectDuration(*target, std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "DefenseReductionDurationInitiative"))));
                addEffect(*target, effect);
            }
            const double markMultiplier = AbilityValue(definition, "MarkNextHitDamageMultiplier");
            if (markMultiplier > 0.0)
            {
                ModifierEffectState mark; mark.id = action.abilityId + ":Mark"; mark.causerId = actor->id;
                mark.sourceAbilityId = action.abilityId; mark.kind = ModifierEffectKind::SnipeMark;
                mark.isDebuff = true; mark.incomingDamageMultiplierFromCauserBasisPoints = static_cast<std::int32_t>(markMultiplier * 10000.0 + 0.5);
                mark.consumeOnDamageFromCauser = true; mark.remainingInitiative = effectDuration(*target,
                    std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "MarkDurationInitiative"))));
                addEffect(*target, mark);
            }
            const std::int32_t extension = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ExtendDebuffDurationInitiative")));
            if (extension > 0 && !debuffToExtendId.empty())
            {
                const auto selected = std::find_if(target->modifierEffects.begin(), target->modifierEffects.end(),
                    [&debuffToExtendId](const ModifierEffectState& candidate) { return candidate.id == debuffToExtendId; });
                if (selected != target->modifierEffects.end()) selected->remainingInitiative += extension;
            }
        }
        const double secondaryMultiplier = AbilityValue(definition, "SecondaryTargetDamageMultiplier");
        TileState* behind = nullptr;
        if (secondaryMultiplier > 0.0 && actorTile && targetTile)
        {
            std::int32_t farthest = HexDistance(actorTile->coord, targetTile->coord);
            for (TileState& tile : state_.tiles)
            {
                if (HexDistance(tile.coord, targetTile->coord) != 1) continue;
                const std::int32_t distance = HexDistance(actorTile->coord, tile.coord);
                if (distance > farthest) { farthest = distance; behind = &tile; }
            }
            if (behind && behind->occupantId != InvalidEntityId)
            {
                if (CharacterState* secondary = FindCharacter(behind->occupantId); secondary && secondary->alive && secondary->team != actor->team)
                {
                    applyDamage(*actor, *secondary, static_cast<std::int32_t>(damage * secondaryMultiplier + 0.5), ignore, action.abilityId + ":Secondary");
                    finalizeDeath(*actor, *secondary);
                }
            }
        }
        if (ammoBefore == 1 && AbilityValue(definition, "LastAmmoKnockbackDistance") > 0.0 && target->alive && actorTile && targetTile)
        {
            const std::int32_t requestedDistance = std::max(0,
                static_cast<std::int32_t>(AbilityValue(definition, "LastAmmoKnockbackDistance")));
            for (std::int32_t step = 0; step < requestedDistance; ++step)
            {
                TileState* current = FindTile(target->tileId);
                TileState* push = nullptr;
                std::int32_t farthest = current ? HexDistance(actorTile->coord, current->coord) : 0;
                for (TileState& tile : state_.tiles)
                {
                    if (!current || tile.occupantId != InvalidEntityId || tile.blocksMovement
                        || HexDistance(tile.coord, current->coord) != 1) continue;
                    const std::int32_t candidateDistance = HexDistance(actorTile->coord, tile.coord);
                    if (candidateDistance > farthest) { farthest = candidateDistance; push = &tile; }
                }
                if (!push || !current) break;
                current->occupantId = InvalidEntityId; push->occupantId = target->id; target->tileId = push->id;
                result.events.push_back(GameEvent{EventType::CharacterMoved, actor->id, target->id,
                    push->id, action.abilityId + ":Knockback", 1});
            }
        }
        const std::int32_t distance = actorTile && targetTile ? HexDistance(actorTile->coord, targetTile->coord) : 0;
        const bool refundRange = distance >= definition.range && !actor->usedRangeRefundThisTurn
            && AbilityValue(definition, "RefundAmmoAtMaxRange") > 0.0;
        const bool refundKill = killed && AbilityValue(definition, "RefundAmmoOnKill") > 0.0;
        ResolveAmmoShot(*actor, killed, refundRange || refundKill, action.abilityId, result.events);
        actor->usedRangeRefundThisTurn = actor->usedRangeRefundThisTurn || refundRange;
        if (killed && !actor->usedKillCooldownReductionThisTurn)
        {
            const std::int32_t reduction = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "CooldownReductionOnKill")));
            const std::int32_t beforeCooldown = abilityState->cooldownRemaining;
            abilityState->cooldownRemaining = std::max(0, abilityState->cooldownRemaining - reduction);
            actor->usedKillCooldownReductionThisTurn = reduction > 0;
            if (abilityState->cooldownRemaining != beforeCooldown)
                result.events.push_back(GameEvent{EventType::CooldownChanged, actor->id, actor->id,
                    actor->tileId, action.abilityId, abilityState->cooldownRemaining - beforeCooldown});
        }
        finalizeDeath(*actor, *target);
        ResolveConcentrateHit(*actor, *target, killed, false, abilityState, snipeApplied, result.events);
    }
    else if (definition.operation == AbilityOperation::Shadowstrike)
    {
        TileState* sourceTile = FindTile(actor->tileId);
        TileState* targetTile = FindTile(target->tileId);
        TileState* destination = nullptr;
        std::int32_t bestDistance = AbilityValue(definition, "LandOppositeTarget") > 0.0 ? -1 : std::numeric_limits<std::int32_t>::max();
        for (TileState& candidate : state_.tiles)
        {
            if (!sourceTile || !targetTile || candidate.occupantId != InvalidEntityId || candidate.blocksMovement
                || HexDistance(candidate.coord, targetTile->coord) != 1) continue;
            const auto terrain = definitions_.terrains.find(candidate.terrainId);
            if (terrain == definitions_.terrains.end() || !terrain->second.walkable) continue;
            const std::int32_t distance = HexDistance(sourceTile->coord, candidate.coord);
            const bool better = AbilityValue(definition, "LandOppositeTarget") > 0.0
                ? distance > bestDistance : distance < bestDistance;
            if (better) { destination = &candidate; bestDistance = distance; }
        }
        if (!destination) return Failure("no_landing_tile", "Shadowstrike has no free landing tile.");
        sourceTile->occupantId = InvalidEntityId; destination->occupantId = actor->id; actor->tileId = destination->id;
        result.events.push_back(GameEvent{EventType::CharacterMoved, actor->id, actor->id,
            destination->id, action.abilityId, 0});
        const std::int32_t comboBefore = actor->resourceCurrent;
        const std::int32_t applied = applyDamage(*actor, *target, definition.amount,
            definition.defenseIgnore + std::max(0, static_cast<std::int32_t>(
                AbilityValue(definition, "DefenseIgnoreOppositeLanding"))), {});
        std::int32_t combo = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ComboPointsGain")));
        if (comboBefore == 0) combo += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "BonusComboAtZero")));
        const std::int32_t isolationRadius = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "IsolationRadius")));
        bool isolated = isolationRadius > 0;
        for (const CharacterState& other : state_.characters)
        {
            const TileState* otherTile = FindTile(other.tileId);
            if (isolated && other.alive && other.id != target->id && other.team == target->team && otherTile && targetTile
                && HexDistance(otherTile->coord, targetTile->coord) <= isolationRadius) isolated = false;
        }
        if (isolated) combo += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "BonusComboVsIsolated")));
        GainCombo(*actor, combo, action.abilityId, result.events);
        if (comboBefore < actor->resourceMaximum && actor->resourceCurrent >= actor->resourceMaximum)
        {
            const std::int32_t reduction = std::max(0, static_cast<std::int32_t>(
                AbilityValue(definition, "ExecuteCooldownReductionOnReachMaxCombo")));
            for (AbilityState& candidate : actor->abilities)
            {
                const auto candidateDefinition = definitions_.abilities.find(candidate.definitionId);
                if (candidateDefinition == definitions_.abilities.end()
                    || candidateDefinition->second.operation != AbilityOperation::Execute) continue;
                const std::int32_t before = candidate.cooldownRemaining;
                candidate.cooldownRemaining = std::max(0, before - reduction);
                if (before != candidate.cooldownRemaining) result.events.push_back(GameEvent{EventType::CooldownChanged,
                    actor->id, actor->id, actor->tileId, candidate.definitionId, candidate.cooldownRemaining - before});
            }
        }
        const std::int32_t followup = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "FollowupBasicAttackCostReduction")));
        if (followup > 0)
        {
            ModifierEffectState effect; effect.id = action.abilityId + ":Followup"; effect.sourceAbilityId = action.abilityId;
            effect.kind = ModifierEffectKind::AssassinFollowup; effect.causerId = actor->id;
            effect.preferredTargetId = target->id; effect.basicAttackCostAvailable = true;
            effect.basicAttackCostAdd = -followup; effect.remainingTurns = 2; effect.expireOnTurnEnd = true;
            addEffect(*actor, effect);
        }
        if (AbilityValue(definition, "GrantFreeAdjacentMove") > 0.0) actor->freeAdjacentMoveAvailable = true;
        const double incoming = AbilityValue(definition, "NextIncomingDamageMultiplier");
        if (incoming > 0.0)
        {
            ModifierEffectState effect; effect.id = action.abilityId + ":Evasion"; effect.causerId = actor->id;
            effect.kind = ModifierEffectKind::AssassinFollowup; effect.flags = 2u;
            effect.incomingDamageMultiplierBasisPoints = static_cast<std::int32_t>(incoming * 10000.0 + 0.5);
            effect.consumeOnDamageFromCauser = false; effect.remainingTurns = 1; effect.expireOnTurnEnd = true;
            addEffect(*actor, effect);
        }
        if (target->health > 0 && applied > 0)
        {
            const std::int32_t duration = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ShadowWoundDurationInitiative")));
            if (duration > 0)
            {
                ModifierEffectState wound; wound.id = action.abilityId + ":ShadowWound";
                wound.sourceAbilityId = action.abilityId; wound.kind = ModifierEffectKind::ShadowWound;
                wound.causerId = actor->id; wound.isDebuff = true; wound.remainingInitiative = duration;
                wound.valueA = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ShadowWoundBonusDamage")));
                wound.valueB = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ShadowWoundComboGain")));
                wound.consumeOnDamageFromCauser = true; addEffect(*target, wound);
            }
        }
        ResolveAssassinHit(*actor, *target, applied, target->health <= 0, action.abilityId, result.events);
        finalizeDeath(*actor, *target);
    }
    else if (definition.operation == AbilityOperation::Armorpen)
    {
        const bool wasDebuffed = isDebuffed(*target);
        if (AbilityValue(definition, "RemoveOldestRemovableBuff") > 0.0)
        {
            const auto buff = std::find_if(target->modifierEffects.begin(), target->modifierEffects.end(),
                [](const ModifierEffectState& effect) { return !effect.isDebuff; });
            if (buff != target->modifierEffects.end())
            {
                result.events.push_back(GameEvent{EventType::EffectRemoved, actor->id, target->id,
                    target->tileId, buff->id, 0}); target->modifierEffects.erase(buff);
            }
        }
        const CharacterDefinition& targetDefinition = definitions_.characters.at(target->definitionId);
        const CharacterDefinition& actorDefinition = definitions_.characters.at(actor->definitionId);
        const std::int32_t comboBefore = actor->resourceCurrent;
        const std::int32_t applied = applyDamage(*actor, *target, definition.amount,
            std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "Armorpen"))), {});
        std::int32_t combo = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ComboPointsGain")));
        if (targetDefinition.stats.defense > actorDefinition.stats.defense)
            combo += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "BonusComboVsHigherDefense")));
        GainCombo(*actor, combo, action.abilityId, result.events);
        if (comboBefore < actor->resourceMaximum && actor->resourceCurrent >= actor->resourceMaximum)
        {
            const std::int32_t before = abilityState->cooldownRemaining;
            abilityState->cooldownRemaining = std::max(0, before - std::max(0, static_cast<std::int32_t>(
                AbilityValue(definition, "CooldownReductionOnReachMaxCombo"))));
            if (before != abilityState->cooldownRemaining) result.events.push_back(GameEvent{EventType::CooldownChanged,
                actor->id, actor->id, actor->tileId, action.abilityId, abilityState->cooldownRemaining - before});
        }
        if (wasDebuffed)
        {
            const std::int32_t refund = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeRefundVsDebuffed")));
            actor->initiative += refund;
            if (refund > 0) result.events.push_back(GameEvent{EventType::InitiativeChanged, actor->id, actor->id,
                actor->tileId, action.abilityId + ":Refund", refund});
        }
        if (target->health > 0)
        {
            const std::int32_t reduction = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DefenseReduction")));
            if (reduction > 0)
            {
                ModifierEffectState effect; effect.id = action.abilityId + ":DefenseBreak"; effect.causerId = actor->id;
                effect.isDebuff = true; effect.defenseIgnoreAdd = reduction; effect.remainingInitiative = std::max(1,
                    static_cast<std::int32_t>(AbilityValue(definition, "DefenseReductionDurationInitiative"))); addEffect(*target, effect);
            }
            const double weakness = AbilityValue(definition, "NextBasicAttackDamageMultiplier");
            if (weakness > 0.0 && weakness < 1.0)
            {
                ModifierEffectState effect; effect.id = action.abilityId + ":BasicWeakness"; effect.causerId = actor->id;
                effect.kind = ModifierEffectKind::BasicAttackWeakness; effect.isDebuff = true;
                effect.basicAttackDamageMultiplierBasisPoints = static_cast<std::int32_t>(weakness * 10000.0 + 0.5);
                effect.consumeOnBasicAttack = true; effect.remainingInitiative = std::max(1,
                    static_cast<std::int32_t>(AbilityValue(definition, "NextBasicAttackReductionDurationInitiative"))); addEffect(*target, effect);
            }
            const std::int32_t breachDuration = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "SharedBreachDurationInitiative")));
            if (breachDuration > 0)
            {
                ModifierEffectState effect; effect.id = action.abilityId + ":SharedBreach"; effect.causerId = actor->id;
                effect.kind = ModifierEffectKind::SharedBreach; effect.isDebuff = true; effect.remainingInitiative = breachDuration;
                effect.valueA = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "Armorpen"))); addEffect(*target, effect);
            }
            const double bleed = AbilityValue(definition, "BleedDamageFractionOnSurvive");
            const std::int32_t bleedDuration = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "BleedDurationInitiative")));
            if (bleed > 0.0 && bleedDuration > 0)
            {
                ModifierEffectState effect; effect.id = action.abilityId + ":Bleed"; effect.causerId = actor->id;
                effect.kind = ModifierEffectKind::Bleed; effect.isDebuff = true; effect.remainingInitiative = bleedDuration;
                effect.valueA = std::max(1, static_cast<std::int32_t>(applied * bleed + 0.5)); addEffect(*target, effect);
            }
            const double second = AbilityValue(definition, "SecondStrikeAppliedDamageFraction");
            if (second > 0.0 && applied > 0)
            {
                const std::int32_t secondApplied = std::min(target->health,
                    std::max(1, static_cast<std::int32_t>(applied * second + 0.5)));
                target->health -= secondApplied;
                result.events.push_back(GameEvent{EventType::DamageApplied, actor->id, target->id,
                    target->tileId, action.abilityId + ":SecondStrike", secondApplied});
            }
        }
        ResolveAssassinHit(*actor, *target, applied, target->health <= 0, action.abilityId, result.events);
        finalizeDeath(*actor, *target);
    }
    else if (definition.operation == AbilityOperation::Execute)
    {
        const std::int32_t requested = AbilityValue(definition, "MaxComboConsumed") > 0.0
            ? static_cast<std::int32_t>(AbilityValue(definition, "MaxComboConsumed")) : actor->resourceCurrent;
        const std::int32_t consumed = SpendCombo(*actor, requested, action.abilityId, result.events);
        std::int32_t perCombo = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DamagePerCombopoint")));
        const double comboMultiplier = AbilityValue(definition, "ComboDamageMultiplier");
        if (comboMultiplier > 0.0) perCombo = static_cast<std::int32_t>(perCombo * comboMultiplier + 0.5);
        std::int32_t damage = definition.amount + perCombo * consumed;
        std::int32_t ignore = definition.defenseIgnore;
        if (isDebuffed(*target)) ignore += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DefenseIgnoreVsDebuffed")));
        if (consumed >= std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "ComboThresholdForDefenseIgnore"))))
            ignore += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DefenseIgnoreAtComboThreshold")));
        ignore += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DefenseIgnoreAtMaxCombo")));
        const CharacterDefinition& targetDefinition = definitions_.characters.at(target->definitionId);
        if (AbilityValue(definition, "LowHealthDamageThreshold") > 0.0
            && target->health <= targetDefinition.stats.maxHealth * AbilityValue(definition, "LowHealthDamageThreshold")
            && AbilityValue(definition, "DamageMultiplierVsLowHealth") > 0.0)
            damage = static_cast<std::int32_t>(damage * AbilityValue(definition, "DamageMultiplierVsLowHealth") + 0.5);
        const std::int32_t applied = applyDamage(*actor, *target, damage, ignore, {});
        const bool killed = target->health <= 0;
        if (killed)
        {
            const std::int32_t gain = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeGainOnKill")));
            actor->initiative += gain;
            if (gain > 0) result.events.push_back(GameEvent{EventType::InitiativeChanged, actor->id, actor->id,
                actor->tileId, action.abilityId + ":Kill", gain});
            const std::int32_t divisor = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ComboRefundOnKillDivisor")));
            if (divisor > 0) GainCombo(*actor, consumed / divisor, action.abilityId + ":KillRefund", result.events);
            if (AbilityValue(definition, "GrantFreeAdjacentMoveOnKill") > 0.0) actor->freeAdjacentMoveAvailable = true;
        }
        else
        {
            GainCombo(*actor, std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ComboRefundOnSurvive"))),
                action.abilityId + ":SurviveRefund", result.events);
            const double bleed = AbilityValue(definition, "BleedDamageFractionOnSurvive");
            const std::int32_t duration = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "BleedDurationInitiative")));
            if (bleed > 0.0 && duration > 0)
            {
                ModifierEffectState effect; effect.id = action.abilityId + ":Bleed"; effect.causerId = actor->id;
                effect.kind = ModifierEffectKind::Bleed; effect.isDebuff = true; effect.remainingInitiative = duration;
                effect.valueA = std::max(1, static_cast<std::int32_t>(applied * bleed + 0.5)); addEffect(*target, effect);
            }
        }
        ResolveAssassinHit(*actor, *target, applied, killed, action.abilityId, result.events);
        finalizeDeath(*actor, *target);
    }
    else if (definition.operation == AbilityOperation::HookShot)
    {
        TileState* origin = FindTile(actor->tileId);
        TileState* destination = FindTile(action.targetTileId);
        CharacterState* ally = destination && destination->occupantId != InvalidEntityId
            ? FindCharacter(destination->occupantId) : nullptr;
        const std::int32_t distance = origin && destination ? HexDistance(origin->coord, destination->coord) : 0;
        if (ally)
        {
            origin->occupantId = ally->id; ally->tileId = origin->id;
            result.events.push_back(GameEvent{EventType::CharacterMoved, actor->id, ally->id,
                origin->id, action.abilityId + ":Swap", 0});
        }
        else origin->occupantId = InvalidEntityId;
        destination->occupantId = actor->id; actor->tileId = destination->id;
        result.events.push_back(GameEvent{EventType::CharacterMoved, actor->id, actor->id,
            destination->id, action.abilityId, distance});
        if (AbilityValue(definition, "GrantFreeAdjacentMove") > 0.0) actor->freeAdjacentMoveAvailable = true;
        if (distance >= std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "ComboDistanceThreshold"))))
            GainCombo(*actor, std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ComboGainAtDistance"))),
                action.abilityId, result.events);
        const double incoming = AbilityValue(definition, "NextIncomingDamageMultiplier");
        if (incoming > 0.0)
        {
            ModifierEffectState effect; effect.id = action.abilityId + ":Evasion"; effect.causerId = actor->id;
            effect.kind = ModifierEffectKind::AssassinFollowup; effect.flags = 2u;
            effect.incomingDamageMultiplierBasisPoints = static_cast<std::int32_t>(incoming * 10000.0 + 0.5);
            effect.remainingTurns = 1; effect.expireOnTurnEnd = true; addEffect(*actor, effect);
        }
        const std::int32_t attackReduction = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "NextBasicAttackCostReduction")));
        if (attackReduction > 0)
        {
            ModifierEffectState effect; effect.id = action.abilityId + ":AttackCost"; effect.causerId = actor->id;
            effect.basicAttackCostAvailable = true; effect.basicAttackCostAdd = -attackReduction;
            effect.consumeOnBasicAttack = true; effect.remainingTurns = 1; effect.expireOnTurnEnd = true; addEffect(*actor, effect);
        }
        const std::int32_t invisibility = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InvisibilityDurationInitiative")));
        const bool invisibilityPrevented = std::any_of(actor->modifierEffects.begin(), actor->modifierEffects.end(),
            [](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::DeathMark
                && (effect.flags & 4u) != 0u; });
        if (invisibility > 0 && !invisibilityPrevented)
        {
            ModifierEffectState effect; effect.id = action.abilityId + ":Invisibility";
            effect.kind = ModifierEffectKind::Invisibility; effect.causerId = actor->id;
            effect.remainingInitiative = invisibility; addEffect(*actor, effect);
        }
        if (AbilityValue(definition, "CreateReturnAnchor") > 0.0)
        {
            ModifierEffectState effect; effect.id = action.abilityId + ":Anchor";
            effect.kind = ModifierEffectKind::HookAnchor; effect.causerId = actor->id;
            effect.sourceAbilityId = action.abilityId; effect.areaCenterTileId = origin->id;
            effect.remainingTurns = 1; effect.expireOnTurnEnd = true; addEffect(*actor, effect);
        }
        if (AbilityValue(definition, "ResetCooldownOnKillAfterUse") > 0.0)
        {
            ModifierEffectState effect; effect.id = action.abilityId + ":KillReset";
            effect.kind = ModifierEffectKind::AssassinFollowup; effect.causerId = actor->id;
            effect.sourceAbilityId = action.abilityId; effect.flags = 1u;
            effect.remainingTurns = 1; effect.expireOnTurnEnd = true; addEffect(*actor, effect);
        }
    }
    else if (definition.operation == AbilityOperation::DeathMark)
    {
        const bool wasDebuffed = isDebuffed(*target);
        target->modifierEffects.erase(std::remove_if(target->modifierEffects.begin(), target->modifierEffects.end(),
            [actor](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::DeathMark
                && effect.causerId == actor->id; }), target->modifierEffects.end());
        if (wasDebuffed)
        {
            const std::int32_t refund = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeRefundVsDebuffed")));
            actor->initiative += refund;
            if (refund > 0) result.events.push_back(GameEvent{EventType::InitiativeChanged, actor->id, actor->id,
                actor->tileId, action.abilityId + ":Refund", refund});
        }
        if (AbilityValue(definition, "PreventInvisibility") > 0.0)
        {
            target->modifierEffects.erase(std::remove_if(target->modifierEffects.begin(), target->modifierEffects.end(),
                [](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::Invisibility; }),
                target->modifierEffects.end());
        }
        ModifierEffectState mark; mark.id = action.abilityId + ":DeathMark"; mark.sourceAbilityId = action.abilityId;
        mark.kind = ModifierEffectKind::DeathMark; mark.causerId = actor->id; mark.isDebuff = true;
        mark.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "DurationInitiative")));
        mark.incomingDamageMultiplierFromCauserBasisPoints = static_cast<std::int32_t>(
            AbilityValue(definition, "DamageMultiplierFromCauser") * 10000.0 + 0.5);
        mark.defenseIgnoreAdd = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DefenseIgnoreFromCauser")));
        mark.counterA = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ComboGainOnTrigger")));
        mark.counterC = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "TransferRangeOnKill")));
        mark.valueA = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "MoveDurationExtension")));
        mark.valueB = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ExecuteCooldownReductionOnMaxCombo")));
        mark.valueC = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DeathMarkCooldownReductionOnExpiry")));
        mark.triggerSlowMultiplierBasisPoints = static_cast<std::int32_t>(AbilityValue(definition, "LowHealthThreshold") * 10000.0 + 0.5);
        mark.triggerOutgoingDamageMultiplierBasisPoints = static_cast<std::int32_t>(AbilityValue(definition, "LowHealthDamageMultiplierAdd") * 10000.0 + 0.5);
        mark.flags = (AbilityValue(definition, "GrantComboEachCauserTurn") > 0.0 ? 2u : 0u)
            | (AbilityValue(definition, "PreventInvisibility") > 0.0 ? 4u : 0u);
        addEffect(*target, mark);
    }
    else if (definition.operation == AbilityOperation::Fireball)
    {
        TileState* center = FindTile(action.targetTileId);
        const std::int32_t radius = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DamageRange")));
        const double fireMultiplier = storedElement == 1
            ? std::max(1.0, PassiveValue(castingDefinition, "ElementFireDamageMultiplier")) : 1.0;
        std::vector<CharacterState*> hit;
        for (CharacterState& candidate : state_.characters)
        {
            TileState* tile = FindTile(candidate.tileId);
            if (!center || !tile || !candidate.alive || candidate.team == actor->team || candidate.team == Team::Neutral
                || HexDistance(center->coord, tile->coord) > radius) continue;
            double multiplier = fireMultiplier;
            if (radius > 0 && HexDistance(center->coord, tile->coord) == radius
                && AbilityValue(definition, "OuterRingDamageMultiplier") > 0.0)
                multiplier *= AbilityValue(definition, "OuterRingDamageMultiplier");
            std::int32_t damage = static_cast<std::int32_t>(definition.amount * multiplier + 0.5);
            if (tile->id == center->id) damage += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "CenterDamageAdd")));
            applyDamage(*actor, candidate, damage, definition.defenseIgnore, {}); hit.push_back(&candidate);
            applyDelay(*actor, candidate, std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeLossOnHit"))), {});
            if (candidate.health > 0 && AbilityValue(definition, "BurnDamageOnTurnStart") > 0.0)
            {
                ModifierEffectState burn; burn.id = action.abilityId + ":Burn"; burn.kind = ModifierEffectKind::FireballBurn;
                burn.causerId = actor->id; burn.isDebuff = true; burn.valueA = static_cast<std::int32_t>(AbilityValue(definition, "BurnDamageOnTurnStart"));
                burn.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "BurnDurationInitiative"))); addEffect(candidate, burn);
            }
            if (candidate.health > 0 && AbilityValue(definition, "DefenseReductionOnHit") > 0.0)
            {
                ModifierEffectState melt; melt.id = action.abilityId + ":Melt"; melt.causerId = actor->id; melt.isDebuff = true;
                melt.defenseIgnoreAdd = static_cast<std::int32_t>(AbilityValue(definition, "DefenseReductionOnHit"));
                melt.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "DefenseReductionDurationInitiative"))); addEffect(candidate, melt);
            }
            if (candidate.health > 0 && AbilityValue(definition, "KnockbackDistance") > 0.0)
            {
                TileState* current = FindTile(candidate.tileId); TileState* destination = nullptr;
                std::int32_t bestDistance = current && center ? HexDistance(center->coord, current->coord) : -1;
                for (TileState& tileCandidate : state_.tiles)
                {
                    const auto terrain = definitions_.terrains.find(tileCandidate.terrainId);
                    if (!current || !center || HexDistance(current->coord, tileCandidate.coord) != 1
                        || tileCandidate.occupantId != InvalidEntityId || tileCandidate.blocksMovement
                        || terrain == definitions_.terrains.end() || !terrain->second.walkable) continue;
                    const std::int32_t distance = HexDistance(center->coord, tileCandidate.coord);
                    if (distance > bestDistance && (!destination || tileCandidate.id < destination->id)) { destination = &tileCandidate; bestDistance = distance; }
                }
                if (destination) { current->occupantId = InvalidEntityId; destination->occupantId = candidate.id; candidate.tileId = destination->id;
                    result.events.push_back(GameEvent{EventType::CharacterMoved, actor->id, candidate.id, destination->id, action.abilityId + ":Knockback", 0}); }
            }
        }
        if (center && AbilityValue(definition, "CreateFireTerrainOnTarget") > 0.0)
        {
            for (const auto& entry : definitions_.terrains)
            {
                std::string id = entry.first; std::transform(id.begin(), id.end(), id.begin(), [](const unsigned char value) { return static_cast<char>(std::tolower(value)); });
                if (id.find("fire") != std::string::npos || id.rfind("terrain:9:", 0) == 0) { center->terrainId = entry.first; break; }
            }
        }
        const std::int32_t recoil = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "SelfKnockbackDistance")));
        if (center && recoil > 0)
        {
            TileState* current = FindTile(actor->tileId);
            for (std::int32_t step = 0; step < recoil && current; ++step)
            {
                TileState* destination = nullptr; std::int32_t bestDistance = HexDistance(center->coord, current->coord);
                for (TileState& tileCandidate : state_.tiles)
                { const auto terrain = definitions_.terrains.find(tileCandidate.terrainId); if (HexDistance(current->coord, tileCandidate.coord) != 1
                    || tileCandidate.occupantId != InvalidEntityId || tileCandidate.blocksMovement || terrain == definitions_.terrains.end() || !terrain->second.walkable) continue;
                  const std::int32_t distance = HexDistance(center->coord, tileCandidate.coord); if (distance > bestDistance && (!destination || tileCandidate.id < destination->id)) { destination = &tileCandidate; bestDistance = distance; } }
                if (!destination) break; current->occupantId = InvalidEntityId; destination->occupantId = actor->id; actor->tileId = destination->id; current = destination;
                result.events.push_back(GameEvent{EventType::CharacterMoved, actor->id, actor->id, destination->id, action.abilityId + ":Recoil", 0});
            }
        }
        if (AbilityValue(definition, "AfterburnDelayInitiative") > 0.0 && AbilityValue(definition, "AfterburnDamageMultiplier") > 0.0)
        {
            ModifierEffectState delayed; delayed.id = action.abilityId + ":Afterburn:" + std::to_string(state_.step);
            delayed.kind = ModifierEffectKind::DelayedArea; delayed.causerId = actor->id; delayed.sourceAbilityId = action.abilityId;
            delayed.areaCenterTileId = action.targetTileId; delayed.areaRadius = radius;
            delayed.valueA = static_cast<std::int32_t>(definition.amount * fireMultiplier * AbilityValue(definition, "AfterburnDamageMultiplier") + 0.5);
            delayed.remainingInitiative = static_cast<std::int32_t>(AbilityValue(definition, "AfterburnDelayInitiative")); addEffect(*actor, delayed);
        }
        for (CharacterState* candidate : hit) finalizeDeath(*actor, *candidate);
    }
    else if (definition.operation == AbilityOperation::Icebeam)
    {
        const bool alreadySlowed = std::any_of(target->modifierEffects.begin(), target->modifierEffects.end(),
            [actor](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::IcebeamSlow && effect.causerId == actor->id; });
        const double fireMultiplier = storedElement == 1 ? std::max(1.0, PassiveValue(castingDefinition, "ElementFireDamageMultiplier")) : 1.0;
        const std::int32_t applied = applyDamage(*actor, *target,
            static_cast<std::int32_t>(definition.amount * fireMultiplier + 0.5), definition.defenseIgnore, {});
        applyDelay(*actor, *target, std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeLossOnHit"))), {});
        if (target->health > 0)
        {
            target->modifierEffects.erase(std::remove_if(target->modifierEffects.begin(), target->modifierEffects.end(),
                [actor](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::IcebeamSlow && effect.causerId == actor->id; }), target->modifierEffects.end());
            ModifierEffectState ice; ice.id = action.abilityId + (alreadySlowed && AbilityValue(definition, "RootIfAlreadySlowed") > 0.0 ? ":Root" : ":Slow");
            ice.kind = ModifierEffectKind::IcebeamSlow; ice.causerId = actor->id; ice.isDebuff = true;
            ice.preventsMovement = alreadySlowed && AbilityValue(definition, "RootIfAlreadySlowed") > 0.0;
            const std::int32_t iceDurationAdd = storedElement == 2 ? std::max(0, static_cast<std::int32_t>(PassiveValue(castingDefinition, "PassiveIceBonusDuration"))) : 0;
            ice.remainingInitiative = std::max(1, static_cast<std::int32_t>(ice.preventsMovement
                ? AbilityValue(definition, "RootDurationInitiative") : AbilityValue(definition, "SlowDuration")) + iceDurationAdd);
            ice.moveCostMultiplierBasisPoints = ice.preventsMovement ? 10000 : static_cast<std::int32_t>((1.0 + std::max(0.0, AbilityValue(definition, "SlowAmount"))) * 10000.0 + 0.5);
            ice.firstMoveInitiativeLoss = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "FirstMoveInitiativeLoss")));
            ice.firstMovePenaltyAvailable = ice.firstMoveInitiativeLoss > 0;
            ice.maximumMovedFields = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "SlowEndsAfterMovedFields")));
            ice.frostbreakDamage = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "FrostbreakDamage"))); addEffect(*target, ice);
            const double brittle = AbilityValue(definition, "BrittleIncomingDamageMultiplier");
            if (brittle > 1.0) { ModifierEffectState effect; effect.id = action.abilityId + ":Brittle"; effect.causerId = actor->id; effect.isDebuff = true;
                effect.incomingDamageMultiplierBasisPoints = static_cast<std::int32_t>(brittle * 10000.0 + 0.5);
                effect.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "BrittleDurationInitiative"))); addEffect(*target, effect); }
        }
        const TileState* targetTile = FindTile(target->tileId);
        const double splashSlow = AbilityValue(definition, "SplashSlowAmount");
        const std::int32_t splashDuration = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "SplashSlowDurationInitiative")));
        if (targetTile && splashSlow > 0.0 && splashDuration > 0)
        {
            for (CharacterState& candidate : state_.characters)
            {
                const TileState* tile = FindTile(candidate.tileId);
                if (!candidate.alive || candidate.id == target->id || candidate.team == actor->team || !tile
                    || HexDistance(targetTile->coord, tile->coord) != 1) continue;
                ModifierEffectState slow; slow.id = action.abilityId + ":Splash"; slow.kind = ModifierEffectKind::IcebeamSlow;
                slow.causerId = actor->id; slow.isDebuff = true; slow.remainingInitiative = splashDuration;
                slow.moveCostMultiplierBasisPoints = static_cast<std::int32_t>((1.0 + splashSlow) * 10000.0 + 0.5);
                addEffect(candidate, slow);
            }
        }
        const double chainMultiplier = AbilityValue(definition, "ChainDamageMultiplier");
        const std::int32_t chainRange = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "ChainSearchRange")));
        if (targetTile && chainMultiplier > 0.0)
        {
            CharacterState* chain = nullptr; std::int32_t best = chainRange + 1;
            for (CharacterState& candidate : state_.characters)
            {
                const TileState* tile = FindTile(candidate.tileId); const std::int32_t distance = tile ? HexDistance(targetTile->coord, tile->coord) : best;
                if (candidate.alive && candidate.id != target->id && candidate.team != actor->team && candidate.team != Team::Neutral
                    && tile && distance <= chainRange && (distance < best || (distance == best && (!chain || candidate.id < chain->id))))
                { chain = &candidate; best = distance; }
            }
            if (chain)
            {
                applyDamage(*actor, *chain, static_cast<std::int32_t>(definition.amount * fireMultiplier * chainMultiplier + 0.5), definition.defenseIgnore, action.abilityId + ":Chain");
                const double chainSlow = AbilityValue(definition, "ChainSlowAmount");
                if (chain->health > 0 && chainSlow > 0.0)
                { ModifierEffectState slow; slow.id = action.abilityId + ":ChainSlow"; slow.kind = ModifierEffectKind::IcebeamSlow; slow.causerId = actor->id; slow.isDebuff = true;
                  slow.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "ChainSlowDurationInitiative")));
                  slow.moveCostMultiplierBasisPoints = static_cast<std::int32_t>((1.0 + chainSlow) * 10000.0 + 0.5); addEffect(*chain, slow); }
                finalizeDeath(*actor, *chain);
            }
        }
        (void)applied; finalizeDeath(*actor, *target);
    }
    else if (definition.operation == AbilityOperation::Lightningstrike)
    {
        TileState* origin = FindTile(actor->tileId); TileState* end = FindTile(action.targetTileId);
        std::vector<CharacterState*> hits;
        if (origin && end)
        {
            const std::int32_t total = HexDistance(origin->coord, end->coord);
            for (CharacterState& candidate : state_.characters)
            {
                TileState* tile = FindTile(candidate.tileId);
                if (!candidate.alive || candidate.team == actor->team || candidate.team == Team::Neutral || !tile
                    || HexDistance(origin->coord, tile->coord) + HexDistance(tile->coord, end->coord) != total) continue;
                hits.push_back(&candidate);
            }
            std::sort(hits.begin(), hits.end(), [origin, this](const CharacterState* a, const CharacterState* b)
                { return HexDistance(origin->coord, FindTile(a->tileId)->coord) < HexDistance(origin->coord, FindTile(b->tileId)->coord); });
        }
        const double fireMultiplier = storedElement == 1 ? std::max(1.0, PassiveValue(castingDefinition, "ElementFireDamageMultiplier")) : 1.0;
        for (std::size_t index = 0; index < hits.size(); ++index)
        {
            const double scaling = 1.0 + index * std::max(0.0, AbilityValue(definition, "DamageMultiplierAddPerPreviousTarget"));
            applyDamage(*actor, *hits[index], static_cast<std::int32_t>(definition.amount * fireMultiplier * scaling + 0.5), definition.defenseIgnore, {});
            applyDelay(*actor, *hits[index], std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeLossOnHit"))), {});
            if (hits[index]->health > 0 && AbilityValue(definition, "StaticChargeMoveDamage") > 0.0)
            { ModifierEffectState charge; charge.id = action.abilityId + ":Static"; charge.kind = ModifierEffectKind::StaticCharge; charge.causerId = actor->id; charge.isDebuff = true;
                charge.triggerOnMove = true; charge.triggerDamageOnMove = static_cast<std::int32_t>(AbilityValue(definition, "StaticChargeMoveDamage"));
                charge.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "StaticChargeDurationInitiative"))); addEffect(*hits[index], charge); }
            finalizeDeath(*actor, *hits[index]);
        }
        const double forkMultiplier = AbilityValue(definition, "EndpointForkDamageMultiplier");
        if (end && forkMultiplier > 0.0)
        {
            for (CharacterState& candidate : state_.characters)
            {
                const TileState* tile = FindTile(candidate.tileId);
                if (!candidate.alive || candidate.team == actor->team || candidate.team == Team::Neutral || !tile
                    || HexDistance(end->coord, tile->coord) != 1 || std::find(hits.begin(), hits.end(), &candidate) != hits.end()) continue;
                applyDamage(*actor, candidate, static_cast<std::int32_t>(definition.amount * fireMultiplier * forkMultiplier + 0.5), definition.defenseIgnore, action.abilityId + ":Fork");
                finalizeDeath(*actor, candidate);
            }
        }
        const std::int32_t finalRange = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "FinalChainRange")));
        const double finalMultiplier = AbilityValue(definition, "FinalChainDamageMultiplier");
        if (!hits.empty() && finalRange > 0 && finalMultiplier > 0.0)
        {
            const TileState* lastTile = FindTile(hits.back()->tileId); CharacterState* chain = nullptr; std::int32_t best = finalRange + 1;
            for (CharacterState& candidate : state_.characters)
            {
                const TileState* tile = FindTile(candidate.tileId); const std::int32_t distance = lastTile && tile ? HexDistance(lastTile->coord, tile->coord) : best;
                if (candidate.alive && candidate.team != actor->team && candidate.team != Team::Neutral && tile
                    && std::find(hits.begin(), hits.end(), &candidate) == hits.end() && distance <= finalRange
                    && (distance < best || (distance == best && (!chain || candidate.id < chain->id)))) { chain = &candidate; best = distance; }
            }
            if (chain) { applyDamage(*actor, *chain, static_cast<std::int32_t>(definition.amount * fireMultiplier * finalMultiplier + 0.5), definition.defenseIgnore, action.abilityId + ":FinalChain"); finalizeDeath(*actor, *chain); }
        }
        if (hits.size() >= 2 && AbilityValue(definition, "InitiativeGainOnMultiHit") > 0.0)
            actor->initiative += static_cast<std::int32_t>(AbilityValue(definition, "InitiativeGainOnMultiHit"));
        const std::int32_t reduction = std::min(std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "MaximumHitCooldownReduction"))),
            static_cast<std::int32_t>(hits.size()) * std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "CooldownReductionPerEnemyHit"))));
        abilityState->cooldownRemaining = std::max(0, abilityState->cooldownRemaining - reduction);
    }
    else if (definition.operation == AbilityOperation::Rockblast)
    {
        TileState* center = FindTile(action.targetTileId); CharacterState* primary = center ? FindCharacter(center->occupantId) : nullptr;
        if (primary && primary->team != actor->team)
        { applyDamage(*actor, *primary, definition.amount, definition.defenseIgnore, {}); applyDelay(*actor, *primary,
            std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeLossOnHit"))), {}); }
        auto terrainByWord = [this](const std::string& word, const DefinitionId& fallback)
        { for (const auto& entry : definitions_.terrains) { std::string id = entry.first; std::transform(id.begin(), id.end(), id.begin(),
                [](const unsigned char value) { return static_cast<char>(std::tolower(value)); });
            if (id.find(word) != std::string::npos || (word == "rock" && id.rfind("terrain:6:", 0) == 0)
                || (word == "rubble" && id.rfind("terrain:7:", 0) == 0)) return entry.first; } return fallback; };
        const DefinitionId rocks = terrainByWord("rock", center ? center->terrainId : DefinitionId{});
        const DefinitionId rubble = terrainByWord("rubble", center ? center->terrainId : DefinitionId{});
        const bool wasRock = center && center->terrainId == rocks;
        std::vector<TileState*> created;
        if (center) center->terrainId = wasRock ? rubble : rocks;
        if (center && !wasRock) created.push_back(center);
        if (wasRock && center && AbilityValue(definition, "ShatterSplashDamageMultiplier") > 0.0)
        {
            for (CharacterState& candidate : state_.characters)
            { const TileState* tile = FindTile(candidate.tileId); if (candidate.alive && candidate.team != actor->team && candidate.team != Team::Neutral && tile
                && HexDistance(center->coord, tile->coord) == 1) { applyDamage(*actor, candidate, static_cast<std::int32_t>(definition.amount * AbilityValue(definition, "ShatterSplashDamageMultiplier") + 0.5), 0, action.abilityId + ":Shatter"); finalizeDeath(*actor, candidate); } }
        }
        std::int32_t remaining = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "AdditionalRockCount")));
        if (center) for (TileState& tile : state_.tiles) if (remaining > 0 && tile.occupantId == InvalidEntityId
            && HexDistance(center->coord, tile.coord) == 1) { tile.terrainId = rocks; created.push_back(&tile); --remaining; }
        const double cover = AbilityValue(definition, "SelfIncomingDamageMultiplierAfterRockCreate");
        if (!created.empty() && cover > 0.0 && cover < 1.0)
        { ModifierEffectState effect; effect.id = action.abilityId + ":Cover"; effect.causerId = actor->id; effect.incomingDamageMultiplierBasisPoints = static_cast<std::int32_t>(cover * 10000.0 + 0.5);
          effect.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "RockCoverDurationInitiative"))); addEffect(*actor, effect); }
        if (primary && primary->health > 0 && AbilityValue(definition, "RootIfSurroundedByRocks") > 0.0 && center)
        {
            const std::int32_t adjacent = static_cast<std::int32_t>(std::count_if(state_.tiles.begin(), state_.tiles.end(), [center, &rocks](const TileState& tile)
                { return tile.terrainId == rocks && HexDistance(center->coord, tile.coord) == 1; }));
            if (adjacent >= std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "RequiredAdjacentRocks"))))
            { ModifierEffectState root; root.id = action.abilityId + ":Prison"; root.causerId = actor->id; root.isDebuff = true; root.preventsMovement = true;
              root.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "RockPrisonDurationInitiative"))); addEffect(*primary, root); }
        }
        if (primary) finalizeDeath(*actor, *primary);
    }
    else if (definition.operation == AbilityOperation::ArcaneShift)
    {
        TileState* origin = FindTile(actor->tileId); TileState* destination = FindTile(action.targetTileId);
        CharacterState* ally = destination && destination->occupantId != InvalidEntityId ? FindCharacter(destination->occupantId) : nullptr;
        if (origin && destination)
        { if (ally) { origin->occupantId = ally->id; ally->tileId = origin->id; result.events.push_back(GameEvent{EventType::CharacterMoved, actor->id, ally->id, origin->id, action.abilityId, 0}); }
          else origin->occupantId = InvalidEntityId; destination->occupantId = actor->id; actor->tileId = destination->id;
          result.events.push_back(GameEvent{EventType::CharacterMoved, actor->id, actor->id, destination->id, action.abilityId, 0}); }
        std::vector<CharacterState*> affected;
        for (CharacterState& enemy : state_.characters)
        { TileState* tile = FindTile(enemy.tileId); if (!tile || !enemy.alive || enemy.team == actor->team || enemy.team == Team::Neutral) continue;
          const bool nearStart = origin && HexDistance(origin->coord, tile->coord) == 1;
          const bool nearEnd = destination && HexDistance(destination->coord, tile->coord) == 1;
          if (!nearStart && !nearEnd) continue;
          if (nearStart) applyDelay(*actor, enemy, std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeLossAroundStart"))), {});
          if (AbilityValue(definition, "SpatialRuptureDamage") > 0.0) applyDamage(*actor, enemy, static_cast<std::int32_t>(AbilityValue(definition, "SpatialRuptureDamage")), 0, {});
          if (nearEnd && AbilityValue(definition, "DamageAroundDestination") > 0.0) applyDamage(*actor, enemy, static_cast<std::int32_t>(AbilityValue(definition, "DamageAroundDestination")), 0, {});
          const std::int32_t trailCost = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "MoveCostAddNearShiftEndpoints")));
          if (trailCost > 0) { ModifierEffectState trail; trail.id = action.abilityId + ":Trail"; trail.causerId = actor->id; trail.isDebuff = true; trail.moveCostAdd = trailCost;
            trail.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "ShiftEndpointDebuffDurationInitiative"))); addEffect(enemy, trail); }
          affected.push_back(&enemy); }
        if (affected.empty() && AbilityValue(definition, "InitiativeRefundIfNoAdjacentEnemy") > 0.0)
            actor->initiative += static_cast<std::int32_t>(AbilityValue(definition, "InitiativeRefundIfNoAdjacentEnemy"));
        double protection = AbilityValue(definition, "NegateNextHit") > 0.0 ? 0.0 : AbilityValue(definition, "NextHitIncomingDamageMultiplier");
        if (protection >= 0.0 && protection < 1.0 && AbilityValue(definition, "NextHitProtectionDurationInitiative") > 0.0)
        { ModifierEffectState effect; effect.id = action.abilityId + ":Protection"; effect.kind = ModifierEffectKind::OneHitProtection; effect.causerId = actor->id;
          effect.incomingDamageMultiplierBasisPoints = static_cast<std::int32_t>(protection * 10000.0 + 0.5); effect.flags = 2u;
          effect.remainingInitiative = static_cast<std::int32_t>(AbilityValue(definition, "NextHitProtectionDurationInitiative")); addEffect(*actor, effect); }
        for (CharacterState* enemy : affected) finalizeDeath(*actor, *enemy);
    }
    else if (definition.operation == AbilityOperation::Dash)
    {
        TileState* start = FindTile(actor->tileId); TileState* targetTile = FindTile(target->tileId); TileState* destination = nullptr;
        if (start && targetTile)
        {
            std::int32_t best = std::numeric_limits<std::int32_t>::max();
            for (TileState& tile : state_.tiles)
            { const auto terrain = definitions_.terrains.find(tile.terrainId); if (HexDistance(targetTile->coord, tile.coord) != 1
                || tile.occupantId != InvalidEntityId || tile.blocksMovement || terrain == definitions_.terrains.end() || !terrain->second.walkable) continue;
              const std::int32_t distance = HexDistance(start->coord, tile.coord); if (distance < best || (distance == best && (!destination || tile.id < destination->id))) { destination = &tile; best = distance; } }
        }
        if (!destination) return Failure("dash_no_destination", "Dash has no free destination beside the target.");
        const std::int32_t travel = start && targetTile ? HexDistance(start->coord, targetTile->coord) : 0;
        start->occupantId = InvalidEntityId; destination->occupantId = actor->id; actor->tileId = destination->id;
        result.events.push_back(GameEvent{EventType::CharacterMoved, actor->id, actor->id, destination->id, action.abilityId, 0});
        std::int32_t damage = definition.amount;
        if (travel >= std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "MomentumRequiredDistance"))))
            damage += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "MomentumDamage")));
        const std::int32_t applied = applyDamage(*actor, *target, damage, definition.defenseIgnore, {});
        GainBlood(*actor, static_cast<std::int32_t>(applied * std::max(0.0, AbilityValue(definition, "BonusBloodFractionOfDamage")) + 0.5), true, action.abilityId, result.events);
        const std::int32_t duration = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "EffectDurationInitiative")));
        const double guard = AbilityValue(definition, "SelfIncomingDamageMultiplierAfterUse");
        if (guard > 0.0 && guard < 1.0 && duration > 0) { ModifierEffectState effect; effect.id = action.abilityId + ":Guard"; effect.causerId = actor->id;
            effect.incomingDamageMultiplierBasisPoints = static_cast<std::int32_t>(guard * 10000.0 + 0.5); effect.remainingInitiative = duration; addEffect(*actor, effect); }
        const std::int32_t moveCost = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "MoveCostAddOnTarget")));
        if (moveCost > 0 && target->health > 0) { ModifierEffectState effect; effect.id = action.abilityId + ":Cripple"; effect.causerId = actor->id; effect.isDebuff = true;
            effect.moveCostAdd = moveCost; effect.remainingInitiative = std::max(1, duration); addEffect(*target, effect); }
        const double splash = AbilityValue(definition, "ImpactSplashDamageMultiplier");
        if (targetTile && splash > 0.0) for (CharacterState& other : state_.characters)
        { const TileState* tile = FindTile(other.tileId); if (other.alive && other.id != target->id && other.team != actor->team && other.team != Team::Neutral && tile
            && HexDistance(targetTile->coord, tile->coord) == 1) { applyDamage(*actor, other, static_cast<std::int32_t>(definition.amount * splash + 0.5), definition.defenseIgnore, action.abilityId + ":Splash"); finalizeDeath(*actor, other); } }
        const double refundThreshold = AbilityValue(definition, "InitiativeRefundHealthThreshold");
        if (target->health > 0 && refundThreshold > 0.0 && target->health <= definitions_.characters.at(target->definitionId).stats.maxHealth * refundThreshold)
        { const std::int32_t refund = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeRefundVsLowHealth"))); actor->initiative += refund;
          if (refund > 0) result.events.push_back(GameEvent{EventType::InitiativeChanged, actor->id, actor->id, actor->tileId, action.abilityId, refund}); }
        const double markMultiplier = AbilityValue(definition, "NextHitDamageMultiplier");
        if (target->health > 0 && markMultiplier > 0.0) { ModifierEffectState mark; mark.id = action.abilityId + ":Mark"; mark.kind = ModifierEffectKind::BloodMark;
            mark.causerId = actor->id; mark.isDebuff = true; mark.incomingDamageMultiplierFromCauserBasisPoints = static_cast<std::int32_t>(markMultiplier * 10000.0 + 0.5);
            mark.consumeOnDamageFromCauser = true; mark.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "MarkDurationInitiative"))); addEffect(*target, mark); }
        const bool killed = target->health <= 0; finalizeDeath(*actor, *target);
        if (killed) abilityState->cooldownRemaining = std::max(0, abilityState->cooldownRemaining - std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "CooldownReductionOnKill"))));
        if (!killed && AbilityValue(definition, "CarryThrough") > 0.0 && start && targetTile)
        { TileState* carry = nullptr; std::int32_t best = HexDistance(start->coord, targetTile->coord); for (TileState& tile : state_.tiles)
          { const auto terrain = definitions_.terrains.find(tile.terrainId); const std::int32_t distance = HexDistance(start->coord, tile.coord); if (HexDistance(targetTile->coord, tile.coord) == 1
            && tile.occupantId == InvalidEntityId && !tile.blocksMovement && terrain != definitions_.terrains.end() && terrain->second.walkable && distance > best) { carry = &tile; best = distance; } }
          if (carry) { destination->occupantId = InvalidEntityId; carry->occupantId = actor->id; actor->tileId = carry->id; result.events.push_back(GameEvent{EventType::CharacterMoved, actor->id, actor->id, carry->id, action.abilityId + ":Carry", 0}); } }
    }
    else if (definition.operation == AbilityOperation::Decimate)
    {
        const CharacterDefinition& targetDefinition = definitions_.characters.at(target->definitionId);
        const std::int32_t healthBefore = target->health; const double fraction = targetDefinition.stats.maxHealth > 0 ? static_cast<double>(target->health) / targetDefinition.stats.maxHealth : 1.0;
        std::int32_t damage = definition.amount + static_cast<std::int32_t>(targetDefinition.stats.maxHealth * std::max(0.0, AbilityValue(definition, "PercentDamage")) + 0.5);
        if (AbilityValue(definition, "HighHealthThreshold") > 0.0 && fraction >= AbilityValue(definition, "HighHealthThreshold")) damage = static_cast<std::int32_t>(damage * std::max(1.0, AbilityValue(definition, "DamageMultiplierVsHighHealth")) + 0.5);
        if (AbilityValue(definition, "LowHealthThreshold") > 0.0 && fraction <= AbilityValue(definition, "LowHealthThreshold")) damage = static_cast<std::int32_t>(damage * std::max(1.0, AbilityValue(definition, "DamageMultiplierVsLowHealth")) + 0.5);
        std::int32_t ignore = definition.defenseIgnore; const std::int32_t required = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "RequiredBloodForDefenseIgnore")));
        if (required > 0 && actor->resourceCurrent >= required) ignore += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DefenseIgnoreIfBloodAtLeast")));
        const std::int32_t bloodCost = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "BloodCostForBonusDamage")));
        if (bloodCost > 0 && actor->resourceCurrent >= bloodCost) { SpendBlood(*actor, bloodCost, action.abilityId, result.events); damage += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "BonusDamageForBlood"))); }
        const std::int32_t applied = applyDamage(*actor, *target, damage, ignore, {});
        applyDelay(*actor, *target, std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeLossOnHit"))), {});
        const std::int32_t reduction = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DefenseReduction")));
        if (target->health > 0 && reduction > 0) { ModifierEffectState effect; effect.id = action.abilityId + ":Break"; effect.causerId = actor->id; effect.isDebuff = true; effect.onlyAffectCauserDamage = true;
            effect.defenseIgnoreAdd = reduction; effect.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "DebuffDurationInitiative"))); addEffect(*target, effect); }
        const std::int32_t healing = std::min(std::max(0, static_cast<std::int32_t>(applied * AbilityValue(definition, "LifestealFractionOnHit") + 0.5)), std::max(0, castingDefinition.stats.maxHealth - actor->health));
        actor->health += healing; if (healing > 0) result.events.push_back(GameEvent{EventType::HealingApplied, actor->id, actor->id, actor->tileId, action.abilityId, healing});
        const bool killed = target->health <= 0;
        if (killed) { const std::int32_t gain = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeGainOnKill"))); actor->initiative += gain;
            GainBlood(*actor, static_cast<std::int32_t>(std::max(0, applied - healthBefore) * AbilityValue(definition, "OverkillBloodFraction") + 0.5), true, action.abilityId, result.events); }
        const TileState* targetTile = FindTile(target->tileId); const double cleave = AbilityValue(definition, "CleaveDamageMultiplier");
        if (targetTile && cleave > 0.0) for (CharacterState& other : state_.characters) { const TileState* tile = FindTile(other.tileId); if (other.alive && other.id != target->id && other.team != actor->team && other.team != Team::Neutral && tile
            && HexDistance(targetTile->coord, tile->coord) == 1) { applyDamage(*actor, other, static_cast<std::int32_t>(definition.amount * cleave + 0.5), ignore, action.abilityId + ":Cleave"); finalizeDeath(*actor, other); } }
        finalizeDeath(*actor, *target);
    }
    else if (definition.operation == AbilityOperation::SpinToWin)
    {
        const TileState* center = FindTile(actor->tileId); std::vector<CharacterState*> hit; std::int32_t totalDamage = 0;
        for (CharacterState& enemy : state_.characters) { const TileState* tile = FindTile(enemy.tileId); if (!center || !tile || !enemy.alive || enemy.team == actor->team || enemy.team == Team::Neutral
            || HexDistance(center->coord, tile->coord) > std::max(1, definition.range)) continue; std::int32_t damage = definition.amount;
          const CharacterDefinition& enemyDefinition = definitions_.characters.at(enemy.definitionId); const double fraction = enemyDefinition.stats.maxHealth > 0 ? static_cast<double>(enemy.health) / enemyDefinition.stats.maxHealth : 1.0;
          if (AbilityValue(definition, "LowHealthThreshold") > 0.0 && fraction <= AbilityValue(definition, "LowHealthThreshold")) damage = static_cast<std::int32_t>(damage * std::max(1.0, AbilityValue(definition, "DamageMultiplierVsLowHealth")) + 0.5);
          totalDamage += applyDamage(*actor, enemy, damage, definition.defenseIgnore, {}); hit.push_back(&enemy);
          if (enemy.health > 0 && AbilityValue(definition, "BleedDamagePerTurn") > 0.0) { ModifierEffectState bleed; bleed.id = action.abilityId + ":Bleed"; bleed.kind = ModifierEffectKind::RendBleed; bleed.causerId = actor->id; bleed.isDebuff = true;
            bleed.valueA = static_cast<std::int32_t>(AbilityValue(definition, "BleedDamagePerTurn")); bleed.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "BleedDurationInitiative"))); addEffect(enemy, bleed); } }
        GainBlood(*actor, static_cast<std::int32_t>(totalDamage * AbilityValue(definition, "BonusBloodFractionOfDamage") + 0.5), true, action.abilityId, result.events);
        const std::int32_t heal = std::min(std::max(0, static_cast<std::int32_t>(hit.size()) * static_cast<std::int32_t>(AbilityValue(definition, "HealPerTarget"))), std::max(0, castingDefinition.stats.maxHealth - actor->health));
        actor->health += heal; if (heal > 0) result.events.push_back(GameEvent{EventType::HealingApplied, actor->id, actor->id, actor->tileId, action.abilityId, heal});
        if (hit.size() > 1) { const std::int32_t gain = std::min(20, static_cast<std::int32_t>(hit.size() - 1) * std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativePerExtraTarget")))); actor->initiative += gain;
            const double guard = AbilityValue(definition, "SelfIncomingDamageMultiplierIfMultiHit"); if (guard > 0.0 && guard < 1.0) { ModifierEffectState effect; effect.id = action.abilityId + ":Guard"; effect.causerId = actor->id;
              effect.incomingDamageMultiplierBasisPoints = static_cast<std::int32_t>(guard * 10000.0 + 0.5); effect.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "GuardDurationInitiative"))); addEffect(*actor, effect); } }
        const std::int32_t reduction = std::min(std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "MaxCooldownReduction"))), static_cast<std::int32_t>(hit.size()) * std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "CooldownReductionPerTarget"))));
        abilityState->cooldownRemaining = std::max(0, abilityState->cooldownRemaining - reduction);
        if (hit.size() >= static_cast<std::size_t>(std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "RedMistRequiredTargets"))))) { GainBlood(*actor, std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "RedMistBloodGain"))), true, action.abilityId, result.events);
            actor->initiative += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "RedMistInitiativeGain"))); }
        if (AbilityValue(definition, "EchoDamageMultiplier") > 0.0) { ModifierEffectState echo; echo.id = action.abilityId + ":Echo"; echo.kind = ModifierEffectKind::DelayedArea; echo.causerId = actor->id; echo.sourceAbilityId = action.abilityId;
            echo.areaCenterTileId = actor->tileId; echo.areaRadius = std::max(1, definition.range); echo.valueA = static_cast<std::int32_t>(definition.amount * AbilityValue(definition, "EchoDamageMultiplier") + 0.5);
            echo.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "EchoDelayInitiative"))); addEffect(*actor, echo); }
        for (CharacterState* enemy : hit) finalizeDeath(*actor, *enemy);
    }
    else if (definition.operation == AbilityOperation::BloodHeal)
    {
        const std::int32_t consumed = SpendBlood(*actor, std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "MaximumBloodConsumed"))), action.abilityId, result.events);
        std::int32_t bloodHealing = consumed * std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "HealFromBlood")));
        const double fraction = castingDefinition.stats.maxHealth > 0 ? static_cast<double>(actor->health) / castingDefinition.stats.maxHealth : 1.0;
        if (AbilityValue(definition, "CriticalHealthThreshold") > 0.0 && fraction <= AbilityValue(definition, "CriticalHealthThreshold")) bloodHealing = static_cast<std::int32_t>(bloodHealing * std::max(1.0, AbilityValue(definition, "BloodHealMultiplierWhileCritical")) + 0.5);
        const std::int32_t requested = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "BaseHeal"))) + bloodHealing;
        const std::int32_t healed = std::min(requested, std::max(0, castingDefinition.stats.maxHealth - actor->health)); actor->health += healed;
        if (healed > 0) result.events.push_back(GameEvent{EventType::HealingApplied, actor->id, actor->id, actor->tileId, action.abilityId, healed});
        GainBlood(*actor, static_cast<std::int32_t>(consumed * AbilityValue(definition, "BloodRetainFraction") + 0.5), true, action.abilityId + ":Retain", result.events);
        if (AbilityValue(definition, "CleanseOneDebuff") > 0.0) { const auto debuff = std::find_if(actor->modifierEffects.begin(), actor->modifierEffects.end(), [](const ModifierEffectState& effect) { return effect.isDebuff; });
            if (debuff != actor->modifierEffects.end()) { result.events.push_back(GameEvent{EventType::EffectRemoved, actor->id, actor->id, actor->tileId, debuff->id, 0}); actor->modifierEffects.erase(debuff); } }
        const std::int32_t duration = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "EffectDurationInitiative")));
        if (duration > 0) { ModifierEffectState buff; buff.id = action.abilityId + ":Buff"; buff.causerId = actor->id; buff.remainingInitiative = duration;
            const double incoming = AbilityValue(definition, "IncomingDamageMultiplierAfterUse"); const double outgoing = AbilityValue(definition, "OutgoingDamageMultiplierAfterUse");
            buff.incomingDamageMultiplierBasisPoints = incoming > 0.0 ? static_cast<std::int32_t>(incoming * 10000.0 + 0.5) : 10000;
            buff.outgoingDamageMultiplierBasisPoints = outgoing > 0.0 ? static_cast<std::int32_t>(outgoing * 10000.0 + 0.5) : 10000;
            buff.healFractionFromDamageDealtBasisPoints = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "LifestealFractionAfterUse") * 10000.0 + 0.5)); addEffect(*actor, buff); }
        const TileState* center = FindTile(actor->tileId); if (center) for (CharacterState& enemy : state_.characters) { const TileState* tile = FindTile(enemy.tileId); if (!enemy.alive || enemy.team == actor->team || enemy.team == Team::Neutral || !tile || HexDistance(center->coord, tile->coord) != 1) continue;
            applyDelay(*actor, enemy, std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "AdjacentInitiativeLoss"))), action.abilityId);
            if (healed > 0 && AbilityValue(definition, "AdjacentDamageFractionOfHealing") > 0.0) { applyDamage(*actor, enemy, static_cast<std::int32_t>(healed * AbilityValue(definition, "AdjacentDamageFractionOfHealing") + 0.5), 0, action.abilityId + ":Wave"); finalizeDeath(*actor, enemy); } }
    }
    else if (definition.operation == AbilityOperation::Rend)
    {
        ModifierEffectState* existing = nullptr; for (ModifierEffectState& effect : target->modifierEffects) if (effect.kind == ModifierEffectKind::RendBleed && effect.causerId == actor->id) { existing = &effect; break; }
        if (existing && AbilityValue(definition, "ImmediateBleedTickOnReapply") > 0.0) applyDamage(*actor, *target, existing->valueA, 0, action.abilityId + ":Reapply");
        target->modifierEffects.erase(std::remove_if(target->modifierEffects.begin(), target->modifierEffects.end(), [actor](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::RendBleed && effect.causerId == actor->id; }), target->modifierEffects.end());
        applyDamage(*actor, *target, definition.amount, definition.defenseIgnore, {});
        if (target->health > 0 && AbilityValue(definition, "BleedDamagePerTurn") > 0.0) { ModifierEffectState bleed; bleed.id = action.abilityId + ":Bleed"; bleed.kind = ModifierEffectKind::RendBleed; bleed.causerId = actor->id; bleed.isDebuff = true;
            bleed.valueA = static_cast<std::int32_t>(AbilityValue(definition, "BleedDamagePerTurn")); bleed.valueB = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "BloodPerBleedTick")));
            bleed.triggerOnMove = AbilityValue(definition, "MovementTriggerDamage") > 0.0; bleed.triggerDamageOnMove = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "MovementTriggerDamage")));
            bleed.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "BleedDurationInitiative"))); addEffect(*target, bleed); }
        GainBlood(*actor, std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ImmediateBloodGain"))), true, action.abilityId, result.events);
        const std::int32_t moveCost = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "MoveCostAddOnTarget"))); const std::int32_t duration = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "DebuffDurationInitiative")));
        if (target->health > 0 && moveCost > 0) { ModifierEffectState effect; effect.id = action.abilityId + ":Hamstring"; effect.causerId = actor->id; effect.isDebuff = true; effect.moveCostAdd = moveCost; effect.remainingInitiative = duration; addEffect(*target, effect); }
        const double markMultiplier = AbilityValue(definition, "IncomingDamageMultiplierFromCauser"); if (target->health > 0 && markMultiplier > 0.0) { ModifierEffectState mark; mark.id = action.abilityId + ":Mark"; mark.kind = ModifierEffectKind::BloodMark; mark.causerId = actor->id; mark.isDebuff = true;
            mark.incomingDamageMultiplierFromCauserBasisPoints = static_cast<std::int32_t>(markMultiplier * 10000.0 + 0.5); mark.consumeOnDamageFromCauser = true; mark.remainingInitiative = duration; addEffect(*target, mark); }
        finalizeDeath(*actor, *target);
    }
    else if (definition.operation >= AbilityOperation::BuildTurret
        && definition.operation <= AbilityOperation::BuildSpotterBeacon)
    {
        TileState* buildTile = FindTile(action.targetTileId);
        const std::int32_t configuredMaximum = static_cast<std::int32_t>(PassiveValue(castingDefinition, "MaxDevices"));
        const std::int32_t maximumDevices = std::max(1, (configuredMaximum > 0 ? configuredMaximum : 3)
            + static_cast<std::int32_t>(PassiveValue(castingDefinition, "MaxDevicesAdd")));
        std::vector<EntityId> ownedDevices;
        for (const DeviceState& device : state_.devices) if (device.ownerId == actor->id) ownedDevices.push_back(device.id);
        while (static_cast<std::int32_t>(ownedDevices.size()) >= maximumDevices)
        {
            RemoveDevice(ownedDevices.front(), result.events);
            ownedDevices.erase(ownedDevices.begin());
            const std::int32_t refund = std::max(0, static_cast<std::int32_t>(
                PassiveValue(castingDefinition, "InitiativeRefundOnDeviceReplace")));
            actor->initiative += refund;
            if (refund > 0) result.events.push_back(GameEvent{EventType::InitiativeChanged, actor->id,
                actor->id, actor->tileId, action.abilityId + ":Replace", refund});
        }
        EntityId nextId = 1;
        for (const TileState& tile : state_.tiles) nextId = std::max(nextId, tile.id + 1);
        for (const CharacterState& character : state_.characters) nextId = std::max(nextId, character.id + 1);
        for (const DeviceState& device : state_.devices) nextId = std::max(nextId, device.id + 1);
        DeviceState device; device.id = nextId; device.ownerId = actor->id; device.tileId = buildTile->id;
        device.sourceAbilityId = action.abilityId;
        device.type = static_cast<DeviceType>(static_cast<std::uint8_t>(definition.operation)
            - static_cast<std::uint8_t>(AbilityOperation::BuildTurret));
        const double healthMultiplier = 1.0 + std::max(0.0, PassiveValue(castingDefinition, "DeviceHealthMultiplierAdd"));
        const double lifetimeMultiplier = 1.0 + std::max(0.0, PassiveValue(castingDefinition, "DeviceLifetimeMultiplierAdd"));
        device.maximumHealth = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "Health") * healthMultiplier + 0.5));
        device.health = device.maximumHealth;
        device.remainingInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "DurationInitiative") * lifetimeMultiplier + 0.5));
        const double intervalReduction = std::clamp(PassiveValue(castingDefinition, "DeviceTriggerIntervalReduction"), 0.0, 0.8);
        device.triggerIntervalInitiative = std::max(0, static_cast<std::int32_t>(
            AbilityValue(definition, "TriggerIntervalInitiative") * (1.0 - intervalReduction) + 0.5));
        device.amount = std::max(0, definition.amount);
        device.damageType = definition.damageType; device.defenseIgnore = definition.defenseIgnore;
        if (device.type == DeviceType::Turret)
        {
            device.range = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "TurretRange")));
            device.initiativeLoss = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeLossOnHit")));
            device.secondaryMultiplierBasisPoints = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "SecondaryTargetDamageMultiplier") * 10000.0 + 0.5));
            device.secondaryAmount = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "LowHealthThreshold") * 10000.0 + 0.5));
            device.multiplierBasisPoints = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "LowHealthDamageMultiplier") * 10000.0 + 0.5));
            device.flags |= 4u;
        }
        else if (device.type == DeviceType::Mine)
        {
            device.range = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "AOERange")));
            device.armDelayInitiative = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "ArmDelayInitiative")));
            device.enemyRevealInitiative = std::max(20, device.armDelayInitiative);
            device.initiativeLoss = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeLossOnHit")));
            device.effectDurationInitiative = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "RootDurationInitiative")));
            if (AbilityValue(definition, "PreserveOnManualTrigger") > 0.0) device.flags |= 1u;
        }
        else if (device.type == DeviceType::ShieldEmitter)
        {
            device.range = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "AuraRange")));
            device.amount = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "ShieldAmount")));
            device.effectDurationInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "ShieldDurationInitiative")));
            device.secondaryAmount = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "HealAmount")));
            device.initiativeLoss = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeGain")));
            device.multiplierBasisPoints = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "SoloShieldMultiplier") * 10000.0 + 0.5));
            if (AbilityValue(definition, "CleanseOneDebuff") > 0.0) device.flags |= 2u;
        }
        else
        {
            device.range = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "AuraRange")));
            device.effectDurationInitiative = std::max(1, static_cast<std::int32_t>(AbilityValue(definition, "DebuffDurationInitiative")));
            device.multiplierBasisPoints = std::max(10000, static_cast<std::int32_t>(AbilityValue(definition, "DamageTakenMultiplier") * 10000.0 + 0.5));
            device.secondaryMultiplierBasisPoints = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "OutgoingDamageMultiplier") * 10000.0 + 0.5));
            device.initiativeLoss = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeLoss")));
            device.moveCostAdd = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "MoveCostAdd")));
        }
        state_.devices.push_back(device); buildTile->fieldObjectId = device.id;
        buildTile->fieldObjectHealth = device.health;
        result.events.push_back(GameEvent{EventType::DeviceCreated, actor->id, device.id,
            device.tileId, action.abilityId, device.health});
    }
    else if (definition.operation == AbilityOperation::OverloadDevices)
    {
        const TileState* actorTile = FindTile(actor->tileId);
        std::vector<EntityId> affected;
        for (const DeviceState& device : state_.devices)
        {
            const TileState* tile = FindTile(device.tileId);
            if (device.ownerId == actor->id && actorTile && tile
                && (definition.range <= 0 || HexDistance(actorTile->coord, tile->coord) <= definition.range))
                affected.push_back(device.id);
        }
        std::int32_t triggered = 0;
        for (const EntityId deviceId : affected)
        {
            DeviceState* device = FindDevice(deviceId); if (!device) continue;
            device->health = std::min(device->maximumHealth, device->health
                + std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "RepairDevices"))));
            device->remainingInitiative += std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "LifetimeExtensionInitiative")));
            if (TileState* tile = FindTile(device->tileId)) tile->fieldObjectHealth = device->health;
            if (TriggerDevice(deviceId, true, result.events))
            {
                ++triggered;
                if (AbilityValue(definition, "DoubleTrigger") > 0.0 && FindDevice(deviceId))
                    TriggerDevice(deviceId, true, result.events);
            }
            device = FindDevice(deviceId);
            if (device && AbilityValue(definition, "DestroyAfterOverload") > 0.0)
            {
                const TileState* center = FindTile(device->tileId);
                const std::int32_t explosion = std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "DestroyExplosionDamage")));
                if (center && explosion > 0) for (CharacterState& enemy : state_.characters)
                {
                    const TileState* tile = FindTile(enemy.tileId);
                    if (enemy.alive && enemy.team != actor->team && enemy.team != Team::Neutral && tile
                        && HexDistance(center->coord, tile->coord) <= 1)
                    { applyDamage(*actor, enemy, explosion, 0, action.abilityId + ":Meltdown"); finalizeDeath(*actor, enemy); }
                }
                RemoveDevice(deviceId, result.events);
            }
        }
        const std::int32_t gain = triggered * std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "InitiativeGainPerDevice")));
        actor->initiative += gain;
        abilityState->cooldownRemaining = std::max(0, abilityState->cooldownRemaining - triggered
            * std::max(0, static_cast<std::int32_t>(AbilityValue(definition, "CooldownReductionPerDevice"))));
    }
    else if (definition.operation == AbilityOperation::DirectHeal)
    {
        const CharacterDefinition& targetDefinition = definitions_.characters.at(target->definitionId);
        const std::int32_t maximumHealth = std::max(0, targetDefinition.stats.maxHealth);
        const std::int32_t healing = std::min(std::max(0, definition.amount), std::max(0, maximumHealth - target->health));
        target->health += healing;
        result.events.push_back(GameEvent{
            EventType::HealingApplied, actor->id, target->id, target->tileId, action.abilityId, healing});
    }
    else
    {
        const CharacterDefinition& targetDefinition = definitions_.characters.at(target->definitionId);
        std::vector<GameEvent> auraEvents;
        std::vector<GameEvent> modifierEvents;
        const std::int32_t applied = ResolveDamage(*actor, *target, std::max(0, definition.amount),
            std::max(0, definition.defenseIgnore), auraEvents, modifierEvents);
        result.events.push_back(GameEvent{
            EventType::DamageApplied, actor->id, target->id, target->tileId, action.abilityId, applied});
        result.events.insert(result.events.end(), auraEvents.begin(), auraEvents.end());
        result.events.insert(result.events.end(), modifierEvents.begin(), modifierEvents.end());

        const std::int32_t immediateInitiativeLoss = std::max(
            0, static_cast<std::int32_t>(AbilityValue(definition, "ImmediateInitiativeLoss")));
        if (immediateInitiativeLoss > 0)
        {
            const double delayMultiplier = PassiveValue(targetDefinition, "DelayReceivedMultiplier");
            const std::int32_t adjustedLoss = delayMultiplier > 0.0
                ? std::max(0, static_cast<std::int32_t>(immediateInitiativeLoss * delayMultiplier + 0.5))
                : immediateInitiativeLoss;
            const std::int32_t previousInitiative = target->initiative;
            target->initiative = std::max(0, target->initiative - adjustedLoss);
            result.events.push_back(GameEvent{EventType::InitiativeChanged, actor->id, target->id,
                target->tileId, action.abilityId, target->initiative - previousInitiative});
        }

        const std::int32_t initiativeGain = std::max({0,
            static_cast<std::int32_t>(AbilityValue(definition, "InitiativeGainOnHit")),
            static_cast<std::int32_t>(AbilityValue(definition, "InitiativeGainOnCondemn"))});
        if (initiativeGain > 0)
        {
            actor->initiative += initiativeGain;
            result.events.push_back(GameEvent{EventType::InitiativeChanged, actor->id, actor->id,
                actor->tileId, action.abilityId, initiativeGain});
        }

        const std::int32_t selfHeal = std::max({0,
            static_cast<std::int32_t>(AbilityValue(definition, "SelfHealOnHit")),
            static_cast<std::int32_t>(AbilityValue(definition, "SelfHealOnCondemn"))});
        if (selfHeal > 0)
        {
            const CharacterDefinition& actorDefinition = definitions_.characters.at(actor->definitionId);
            const std::int32_t maximumHealth = std::max(0, actorDefinition.stats.maxHealth);
            const std::int32_t healing = std::min(selfHeal, std::max(0, maximumHealth - actor->health));
            actor->health += healing;
            result.events.push_back(GameEvent{EventType::HealingApplied, actor->id, actor->id,
                actor->tileId, action.abilityId, healing});
        }
        const std::int32_t silenceDuration = std::max(
            0, static_cast<std::int32_t>(AbilityValue(definition, "SilenceDurationInitiative")));
        if (silenceDuration > 0 && target->alive)
        {
            ModifierEffectState silence;
            silence.id = action.abilityId + ":Silence";
            silence.causerId = actor->id;
            silence.remainingInitiative = silenceDuration;
            silence.preventsAbilityUse = true;
            target->modifierEffects.erase(std::remove_if(target->modifierEffects.begin(), target->modifierEffects.end(),
                [&silence](const ModifierEffectState& existing) { return existing.id == silence.id; }),
                target->modifierEffects.end());
            target->modifierEffects.push_back(silence);
            result.events.push_back(GameEvent{EventType::EffectAdded, actor->id, target->id,
                target->tileId, silence.id, silence.remainingInitiative});
        }
        if (applied > 0 && definition.damageType != DamageType::Physical)
        {
            for (auto effect = target->modifierEffects.begin(); effect != target->modifierEffects.end();)
            {
                if (effect->causerId != actor->id || effect->frostbreakDamage <= 0)
                {
                    ++effect;
                    continue;
                }
                const std::int32_t frostbreakDamage = effect->frostbreakDamage;
                const DefinitionId effectId = effect->id;
                effect = target->modifierEffects.erase(effect);
                result.events.push_back(GameEvent{
                    EventType::EffectRemoved, actor->id, target->id, target->tileId, effectId, 0});
                if (target->health <= 0)
                {
                    continue;
                }
                std::vector<GameEvent> frostAuraEvents;
                std::vector<GameEvent> frostModifierEvents;
                const std::int32_t frostApplied = ResolveDamage(
                    *actor, *target, frostbreakDamage, 0, frostAuraEvents, frostModifierEvents);
                result.events.push_back(GameEvent{EventType::DamageApplied, actor->id, target->id,
                    target->tileId, "IcebeamFrostbreak", frostApplied});
                result.events.insert(result.events.end(), frostAuraEvents.begin(), frostAuraEvents.end());
                result.events.insert(result.events.end(), frostModifierEvents.begin(), frostModifierEvents.end());
            }
        }
        if (target->health <= 0)
        {
            target->health = 0;
            target->alive = false;
            if (TileState* tile = FindTile(target->tileId)) tile->occupantId = InvalidEntityId;
            result.events.push_back(GameEvent{
                EventType::CharacterDied, actor->id, target->id, target->tileId, action.abilityId, 0});

            Team soleTeam = Team::Neutral;
            bool multipleTeams = false;
            for (const CharacterState& character : state_.characters)
            {
                if (!character.alive || character.team == Team::Neutral) continue;
                if (soleTeam == Team::Neutral) soleTeam = character.team;
                else if (soleTeam != character.team) { multipleTeams = true; break; }
            }
            if (!multipleTeams && soleTeam != Team::Neutral)
            {
                state_.status = MatchStatus::Completed;
                state_.winner = soleTeam;
                state_.activeCharacterId = InvalidEntityId;
                result.events.push_back(GameEvent{
                    EventType::MatchEnded, actor->id, InvalidEntityId, InvalidEntityId, action.abilityId,
                    static_cast<std::int64_t>(soleTeam)});
            }
        }
    }
    if (mageCast)
    {
        std::int32_t castElement = 0;
        if (definition.damageType == DamageType::Fire) castElement = 1;
        else if (definition.damageType == DamageType::Ice) castElement = 2;
        else if (definition.damageType == DamageType::Lightning) castElement = 3;
        else if (definition.damageType == DamageType::Ground) castElement = 4;
        if (storedElement != 0) actor->usedElementMask |= (1u << static_cast<std::uint32_t>(storedElement));
        if (!actor->elementSwapRefundUsed && storedElement != 0 && castElement != 0 && storedElement != castElement)
        {
            const std::int32_t refund = std::max(0, static_cast<std::int32_t>(PassiveValue(castingDefinition, "DifferentElementInitiativeRefund")));
            actor->initiative += refund; actor->elementSwapRefundUsed = refund > 0;
            if (refund > 0) result.events.push_back(GameEvent{EventType::InitiativeChanged, actor->id, actor->id,
                actor->tileId, action.abilityId + ":ElementSwap", refund});
        }
        if (storedElement == 3)
        {
            const double multiplier = PassiveValue(castingDefinition, "ElementLightningCooldownMultiplier");
            if (multiplier > 0.0 && multiplier < 1.0)
                abilityState->cooldownRemaining = std::max(0, static_cast<std::int32_t>(abilityState->cooldownRemaining * multiplier + 0.5));
        }
        if (storedElement == 4)
        {
            const std::int32_t hitCount = static_cast<std::int32_t>(std::count_if(result.events.begin(), result.events.end(),
                [actor](const GameEvent& event) { return event.type == EventType::DamageApplied && event.sourceId == actor->id && event.amount > 0; }));
            const std::int32_t perHit = std::max(0, static_cast<std::int32_t>(PassiveValue(castingDefinition, "ElementRockDefensePerEnemyHit")));
            const std::int32_t maximum = std::max(1, static_cast<std::int32_t>(PassiveValue(castingDefinition, "ElementRockMaxDefenseStacks"))) * perHit;
            actor->rockDefenseBonus = std::min(maximum, actor->rockDefenseBonus + hitCount * perHit);
            actor->rockDefenseRemainingInitiative = std::max(0, static_cast<std::int32_t>(PassiveValue(castingDefinition, "ElementRockDefenseDurationInitiative")));
        }
        const double protection = PassiveValue(castingDefinition, "ElementProtectionIncomingDamageMultiplier");
        if (storedElement != 0 && protection > 0.0 && protection < 1.0)
        {
            ModifierEffectState effect; effect.id = "Character4ElementProtection"; effect.kind = ModifierEffectKind::OneHitProtection;
            effect.causerId = actor->id; effect.incomingDamageMultiplierBasisPoints = static_cast<std::int32_t>(protection * 10000.0 + 0.5);
            effect.flags = 2u; effect.remainingInitiative = std::max(1, static_cast<std::int32_t>(PassiveValue(castingDefinition, "ElementProtectionDurationInitiative")));
            addEffect(*actor, effect); actor->protectionReady = true; actor->protectionRemainingInitiative = effect.remainingInitiative;
        }
        actor->echoElement = PassiveValue(castingDefinition, "ElementEcho") > 0.0 ? storedElement : 0;
        if (castElement == 0) actor->elementCycleMask = 0;
        else
        {
            const std::uint32_t bit = 1u << static_cast<std::uint32_t>(castElement);
            actor->elementCycleMask = (actor->elementCycleMask & bit) != 0u ? bit : actor->elementCycleMask | bit;
        }
        actor->lastCastElement = castElement;
        if (actor->firstCastElement == 0) actor->firstCastElement = castElement;
        actor->activeElement = castElement;
    }
    ++state_.step;
    return result;
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

bool Game::RollBasisPoints(const std::int32_t chanceBasisPoints) noexcept
{
    const std::int32_t chance = std::clamp(chanceBasisPoints, 0, 10000);
    return chance > 0 && static_cast<std::int32_t>(NextRandom() % 10000ull) < chance;
}

ActionResult Game::ApplyEndTurn(const GameAction& action)
{
    std::vector<GameEvent> expirationEvents;
    if (action.actorId != InvalidEntityId)
    {
        if (CharacterState* endingCharacter = FindCharacter(action.actorId))
        {
            const CharacterDefinition& endingDefinition = definitions_.characters.at(endingCharacter->definitionId);
            if (PassiveValue(endingDefinition, "CoreAmmoCharacter") > 0.0)
            {
                endingCharacter->heldPositionFromPreviousTurn = !endingCharacter->movedSinceLastTurn;
                const bool blockRegeneration = std::any_of(endingCharacter->modifierEffects.begin(), endingCharacter->modifierEffects.end(),
                    [](const ModifierEffectState& effect) { return effect.kind == ModifierEffectKind::Concentrate && (effect.flags & 16u) != 0u; });
                if (!endingCharacter->movedSinceLastTurn && !blockRegeneration)
                {
                    const std::int32_t restored = std::max(0, static_cast<std::int32_t>(
                        PassiveValue(endingDefinition, "BaseAmmoRegen")
                        + PassiveValue(endingDefinition, "AmmoRegenAdd")));
                    const std::int32_t before = endingCharacter->resourceCurrent;
                    endingCharacter->resourceCurrent = std::min(endingCharacter->resourceMaximum,
                        endingCharacter->resourceCurrent + restored);
                    if (before != endingCharacter->resourceCurrent)
                        expirationEvents.push_back(GameEvent{EventType::ResourceChanged, endingCharacter->id,
                            endingCharacter->id, endingCharacter->tileId, "AmmoRegen",
                            endingCharacter->resourceCurrent - before});
                }
            }
            if (endingCharacter->resourceMaximum > 0)
            {
                endingCharacter->stationaryGuardActive = !endingCharacter->movedSinceLastTurn;
            }
            for (auto effect = endingCharacter->modifierEffects.begin(); effect != endingCharacter->modifierEffects.end();)
            {
                if (effect->expireOnTurnEnd && effect->remainingTurns > 0)
                {
                    --effect->remainingTurns;
                }
                if (effect->expireOnTurnEnd && effect->remainingTurns <= 0 && effect->remainingInitiative <= 0)
                {
                    if (effect->initiativeLossOnExpire > 0)
                    {
                        endingCharacter->initiative = std::max(0, endingCharacter->initiative - effect->initiativeLossOnExpire);
                        expirationEvents.push_back(GameEvent{
                            EventType::InitiativeChanged, endingCharacter->id, endingCharacter->id,
                            endingCharacter->tileId, effect->id, -effect->initiativeLossOnExpire});
                    }
                    expirationEvents.push_back(GameEvent{
                        EventType::EffectRemoved, endingCharacter->id, endingCharacter->id,
                        endingCharacter->tileId, effect->id, 0});
                    effect = endingCharacter->modifierEffects.erase(effect);
                }
                else
                {
                    ++effect;
                }
            }
        }
    }
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
    result.events = std::move(expirationEvents);
    if (selected)
    {
        const CharacterDefinition& definition = definitions_.characters.at(selected->definitionId);
        for (const ModifierEffectState& effect : selected->modifierEffects)
        {
            if ((effect.kind != ModifierEffectKind::Bleed && effect.kind != ModifierEffectKind::FireballBurn
                    && effect.kind != ModifierEffectKind::RendBleed)
                || effect.valueA <= 0) continue;
            const std::int32_t applied = std::min(selected->health, effect.valueA);
            selected->health -= applied;
            result.events.push_back(GameEvent{EventType::DamageApplied, effect.causerId, selected->id,
                selected->tileId, effect.id, applied});
            if (effect.kind == ModifierEffectKind::RendBleed && applied > 0 && effect.valueB > 0)
                if (CharacterState* causer = FindCharacter(effect.causerId)) GainBlood(*causer, effect.valueB, true, effect.id, result.events);
        }
        if (selected->health <= 0)
        {
            selected->health = 0; selected->alive = false;
            if (TileState* tile = FindTile(selected->tileId)) tile->occupantId = InvalidEntityId;
            result.events.push_back(GameEvent{EventType::CharacterDied, InvalidEntityId, selected->id,
                selected->tileId, "Bleed", 0});
            result.message = "Selected character died before its turn.";
            ++state_.step;
            return result;
        }
        const std::int32_t delay = std::max(0, definition.stats.turnDelay);
        if (delay > 0)
        {
            selected->initiative = std::max(0, selected->initiative - delay);
            result.events.push_back(GameEvent{
                EventType::InitiativeChanged, selected->id, selected->id, selected->tileId, {}, -delay});
        }
        if (selected->resourceMaximum > 0)
        {
            selected->movedSinceLastTurn = false;
            selected->stationaryGuardActive = false;
            selected->firstResourceSpendRefundAvailable = true;
            if (PassiveValue(definition, "CoreAmmoCharacter") > 0.0)
            {
                selected->attackedThisTurn = false;
                selected->spentResourceThisTurn = false;
                selected->usedFreeBasicAttackThisTurn = false;
                selected->triggeredEmptyResourceThisTurn = false;
                selected->refundedLastResourceKillThisTurn = false;
                selected->usedRangeRefundThisTurn = false;
                selected->usedKillCooldownReductionThisTurn = false;
                selected->startedTurnWithFullResource = selected->resourceCurrent >= selected->resourceMaximum;
                if (selected->startedTurnWithFullResource)
                {
                    const std::int32_t reduction = std::max(0, static_cast<std::int32_t>(
                        PassiveValue(definition, "FullAmmoTurnStartAbilityCooldownReduction")));
                    for (AbilityState& ability : selected->abilities)
                    {
                        const std::int32_t before = ability.cooldownRemaining;
                        ability.cooldownRemaining = std::max(0, before - reduction);
                        if (before != ability.cooldownRemaining) result.events.push_back(GameEvent{
                            EventType::CooldownChanged, selected->id, selected->id, selected->tileId,
                            ability.definitionId, ability.cooldownRemaining - before});
                    }
                    const std::int32_t turnDelayAdd = static_cast<std::int32_t>(
                        PassiveValue(definition, "TurnDelayAddWhileFullAmmo"));
                    selected->initiative = std::max(0, selected->initiative - std::max(0, turnDelayAdd));
                }
                if (PassiveValue(definition, "CoreComboCharacter") > 0.0)
                {
                    ++selected->turnSequence;
                    selected->movedFieldsThisTurn = 0;
                    selected->resourceSpentThisTurnCount = 0;
                    selected->attackedThisTurn = false;
                    selected->openingRewardUsedThisTurn = false;
                    selected->overflowRewardUsedThisTurn = false;
                    selected->freeAdjacentMoveAvailable = false;
                    selected->freeAdjacentMoveUsedThisTurn = false;
                    selected->nextDamageAfterMoveReady = false;
                    selected->nextReductionAfterMoveReady = false;
                    if (selected->resourceCurrent <= 0)
                        GainCombo(*selected, std::max(0, static_cast<std::int32_t>(
                            PassiveValue(definition, "ComboOnEmptyTurnStart"))), "EmptyComboTurnStart", result.events);
                    for (auto effect = selected->modifierEffects.begin(); effect != selected->modifierEffects.end();)
                    {
                        if (effect->kind == ModifierEffectKind::HookAnchor)
                        {
                            result.events.push_back(GameEvent{EventType::EffectRemoved, selected->id, selected->id,
                                selected->tileId, effect->id, 0});
                            effect = selected->modifierEffects.erase(effect);
                        }
                        else ++effect;
                    }
                }
                if (selected->resourceCurrent <= 0 && PassiveValue(definition, "RefillAmmoOnEmptyTurnStart") > 0.0)
                {
                    const std::int32_t gain = selected->resourceMaximum;
                    selected->resourceCurrent = gain;
                    result.events.push_back(GameEvent{EventType::ResourceChanged, selected->id, selected->id,
                        selected->tileId, "FullReload", gain});
                    const std::int32_t penalty = std::max(0, static_cast<std::int32_t>(
                        PassiveValue(definition, "RefillAmmoInitiativePenalty")));
                    selected->initiative = std::max(0, selected->initiative - penalty);
                    if (penalty > 0) result.events.push_back(GameEvent{EventType::InitiativeChanged,
                        selected->id, selected->id, selected->tileId, "FullReload", -penalty});
                }
            }
            if (selected->resourceCurrent <= 0)
            {
                const std::int32_t gain = std::max(0, static_cast<std::int32_t>(
                    PassiveValue(definition, "ZeroChainTurnStartGainChain")));
                if (gain > 0)
                {
                    const std::int32_t previous = selected->resourceCurrent;
                    selected->resourceCurrent = std::min(selected->resourceMaximum, selected->resourceCurrent + gain);
                    result.events.push_back(GameEvent{EventType::ResourceChanged, selected->id, selected->id,
                        selected->tileId, "NativeResource", selected->resourceCurrent - previous});
                }
                const std::int32_t penalty = std::max(0, static_cast<std::int32_t>(
                    PassiveValue(definition, "ZeroChainTurnStartInitiativePenalty")));
                if (penalty > 0)
                {
                    const std::int32_t previous = selected->initiative;
                    selected->initiative = std::max(0, selected->initiative - penalty);
                    result.events.push_back(GameEvent{EventType::InitiativeChanged, selected->id, selected->id,
                        selected->tileId, "ZeroResourceTurnStart", selected->initiative - previous});
                }
            }
        }
        if (PassiveValue(definition, "CoreMageCharacter") > 0.0)
        {
            selected->usedElementMask = 0;
            selected->firstCastElement = 0;
            selected->elementCostReductionUsed = false;
            selected->elementSwapRefundUsed = false;
        }
        if (PassiveValue(definition, "CoreBloodCharacter") > 0.0)
        {
            selected->firstBloodDamageReductionAvailable = true;
            selected->basicAttackBloodAvailable = true;
            selected->bloodCapTriggerAvailable = true;
        }
        const std::int32_t deviceRepair = std::max(0, static_cast<std::int32_t>(
            PassiveValue(definition, "DeviceRepairOnTurnStart")));
        if (deviceRepair > 0)
        {
            for (DeviceState& device : state_.devices)
            {
                if (device.ownerId != selected->id) continue;
                device.health = std::min(device.maximumHealth, device.health + deviceRepair);
                if (TileState* tile = FindTile(device.tileId)) tile->fieldObjectHealth = device.health;
            }
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
            if (character.specialCooldownRemaining > 0)
            {
                --character.specialCooldownRemaining;
            }
            if (character.secondaryCooldownRemaining > 0)
                --character.secondaryCooldownRemaining;
            if (character.elementSwapCooldownRemaining > 0) --character.elementSwapCooldownRemaining;
            if (character.rockDefenseRemainingInitiative > 0 && --character.rockDefenseRemainingInitiative == 0)
                character.rockDefenseBonus = 0;
            if (character.protectionRemainingInitiative > 0 && --character.protectionRemainingInitiative == 0)
                character.protectionReady = false;
            if (character.resourceMaximum > 0 && character.resourceRechargeInterval > 0)
            {
                ++character.resourceRechargeProgress;
                while (character.resourceRechargeProgress >= character.resourceRechargeInterval)
                {
                    character.resourceRechargeProgress -= character.resourceRechargeInterval;
                    if (character.resourceCurrent < character.resourceMaximum)
                    {
                        ++character.resourceCurrent;
                        result.events.push_back(GameEvent{EventType::ResourceChanged, character.id, character.id,
                            character.tileId, "NativeResource", 1});
                    }
                    else
                    {
                        const CharacterDefinition& definition = definitions_.characters.at(character.definitionId);
                        const std::int32_t overflowGain = std::max(0, static_cast<std::int32_t>(
                            PassiveValue(definition, "ChainOverflowInitiativeGain")));
                        if (overflowGain > 0)
                        {
                            character.initiative += overflowGain;
                            result.events.push_back(GameEvent{EventType::InitiativeChanged, character.id, character.id,
                                character.tileId, "ResourceOverflow", overflowGain});
                        }
                    }
                }
            }
            for (const ModifierEffectState& area : character.modifierEffects)
            {
                if ((area.areaDamage <= 0 && area.areaInitiativeLossPerPulse <= 0)
                    || area.remainingInitiative <= 0)
                {
                    continue;
                }
                const EntityId centerTileId = area.areaFollowsCauser ? character.tileId : area.areaCenterTileId;
                const TileState* centerTile = FindTile(centerTileId);
                if (!centerTile)
                {
                    continue;
                }
                for (CharacterState& target : state_.characters)
                {
                    if (!target.alive || target.team == Team::Neutral || target.team == character.team)
                    {
                        continue;
                    }
                    const TileState* targetTile = FindTile(target.tileId);
                    if (!targetTile || HexDistance(centerTile->coord, targetTile->coord) > area.areaRadius)
                    {
                        continue;
                    }
                    std::vector<GameEvent> auraEvents;
                    std::vector<GameEvent> modifierEvents;
                    const std::int32_t applied = ResolveDamage(
                        character, target, area.areaDamage, 0, auraEvents, modifierEvents);
                    result.events.push_back(GameEvent{EventType::DamageApplied, character.id, target.id,
                        target.tileId, area.id, applied});
                    result.events.insert(result.events.end(), auraEvents.begin(), auraEvents.end());
                    result.events.insert(result.events.end(), modifierEvents.begin(), modifierEvents.end());
                    if (area.areaInitiativeLossPerPulse > 0 && target.alive)
                    {
                        const CharacterDefinition& targetDefinition = definitions_.characters.at(target.definitionId);
                        const double delayMultiplier = PassiveValue(targetDefinition, "DelayReceivedMultiplier");
                        const std::int32_t adjustedLoss = delayMultiplier > 0.0
                            ? std::max(0, static_cast<std::int32_t>(
                                area.areaInitiativeLossPerPulse * delayMultiplier + 0.5))
                            : area.areaInitiativeLossPerPulse;
                        const std::int32_t previous = target.initiative;
                        target.initiative = std::max(0, target.initiative - adjustedLoss);
                        result.events.push_back(GameEvent{EventType::InitiativeChanged, character.id, target.id,
                            target.tileId, area.id, target.initiative - previous});
                    }
                    if (target.health <= 0 && target.alive)
                    {
                        target.health = 0;
                        target.alive = false;
                        if (TileState* tile = FindTile(target.tileId)) tile->occupantId = InvalidEntityId;
                        result.events.push_back(GameEvent{EventType::CharacterDied, character.id, target.id,
                            target.tileId, area.id, 0});
                    }
                }
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
                const auto abilityDefinition = definitions_.abilities.find(ability.definitionId);
                if (abilityDefinition != definitions_.abilities.end()
                    && abilityDefinition->second.operation == AbilityOperation::Snipe)
                {
                    if (previousCooldown <= 0) ++ability.readyInitiativeElapsed;
                    else if (ability.cooldownRemaining > 0) ability.readyInitiativeElapsed = 0;
                }
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
            for (auto effect = character.modifierEffects.begin(); effect != character.modifierEffects.end();)
            {
                if (effect->kind == ModifierEffectKind::Concentrate)
                {
                    const AbilityDefinition& focusDefinition = definitions_.abilities.at(effect->sourceAbilityId);
                    ++effect->counterB;
                    const std::int32_t interval = std::max(1, static_cast<std::int32_t>(AbilityValue(focusDefinition, "ChargeInterval")));
                    const double perInterval = AbilityValue(focusDefinition, "ChargeBonusPerInterval");
                    const double maximum = AbilityValue(focusDefinition, "MaxChargeBonus");
                    if (perInterval > 0.0) effect->outgoingDamageMultiplierBasisPoints = effect->valueB + static_cast<std::int32_t>(
                        std::min(maximum, (effect->counterB / interval) * perInterval) * 10000.0 + 0.5);
                    const std::int32_t reloadDelay = std::max(1, static_cast<std::int32_t>(AbilityValue(focusDefinition, "ReloadDelayInitiative")));
                    if (effect->valueC > 0 && effect->counterB >= reloadDelay)
                    {
                        const std::int32_t before = character.resourceCurrent;
                        character.resourceCurrent = std::min(character.resourceMaximum, character.resourceCurrent + effect->valueC);
                        effect->valueC = 0;
                        if (before != character.resourceCurrent) result.events.push_back(GameEvent{EventType::ResourceChanged,
                            character.id, character.id, character.tileId, effect->id, character.resourceCurrent - before});
                    }
                }
                if (effect->remainingInitiative > 0)
                {
                    --effect->remainingInitiative;
                }
                if (effect->remainingInitiative <= 0 && effect->remainingTurns <= 0)
                {
                    if (effect->kind == ModifierEffectKind::DelayedArea && effect->valueA > 0)
                    {
                        CharacterState* causer = FindCharacter(effect->causerId);
                        const TileState* center = FindTile(effect->areaCenterTileId);
                        if (causer && center)
                        {
                            for (CharacterState& target : state_.characters)
                            {
                                const TileState* tile = FindTile(target.tileId);
                                if (!target.alive || target.team == causer->team || target.team == Team::Neutral || !tile
                                    || HexDistance(center->coord, tile->coord) > effect->areaRadius) continue;
                                std::vector<GameEvent> auraEvents; std::vector<GameEvent> modifierEvents;
                                const std::int32_t applied = ResolveDamage(*causer, target, effect->valueA, 0, auraEvents, modifierEvents);
                                result.events.push_back(GameEvent{EventType::DamageApplied, causer->id, target.id,
                                    target.tileId, effect->id, applied});
                                result.events.insert(result.events.end(), auraEvents.begin(), auraEvents.end());
                                result.events.insert(result.events.end(), modifierEvents.begin(), modifierEvents.end());
                                if (target.health <= 0 && target.alive)
                                {
                                    target.health = 0; target.alive = false;
                                    if (TileState* occupied = FindTile(target.tileId)) occupied->occupantId = InvalidEntityId;
                                    result.events.push_back(GameEvent{EventType::CharacterDied, causer->id, target.id,
                                        target.tileId, effect->id, 0});
                                }
                            }
                        }
                    }
                    if (effect->kind == ModifierEffectKind::DeathMark && character.alive && effect->valueC > 0)
                    {
                        if (CharacterState* causer = FindCharacter(effect->causerId))
                        {
                            for (AbilityState& ability : causer->abilities)
                            {
                                const auto abilityDefinition = definitions_.abilities.find(ability.definitionId);
                                if (abilityDefinition == definitions_.abilities.end()
                                    || abilityDefinition->second.operation != AbilityOperation::DeathMark) continue;
                                const std::int32_t previous = ability.cooldownRemaining;
                                ability.cooldownRemaining = std::max(0, previous - effect->valueC);
                                if (previous != ability.cooldownRemaining) result.events.push_back(GameEvent{
                                    EventType::CooldownChanged, causer->id, causer->id, causer->tileId,
                                    ability.definitionId, ability.cooldownRemaining - previous});
                            }
                        }
                    }
                    if (effect->initiativeLossOnExpire > 0)
                    {
                        character.initiative = std::max(0, character.initiative - effect->initiativeLossOnExpire);
                        result.events.push_back(GameEvent{
                            EventType::InitiativeChanged, character.id, character.id,
                            character.tileId, effect->id, -effect->initiativeLossOnExpire});
                    }
                    result.events.push_back(GameEvent{
                        EventType::EffectRemoved, character.id, character.id, character.tileId, effect->id, 0});
                    effect = character.modifierEffects.erase(effect);
                }
                else
                {
                    ++effect;
                }
            }
        }
        std::vector<EntityId> deviceIds;
        deviceIds.reserve(state_.devices.size());
        for (const DeviceState& device : state_.devices) deviceIds.push_back(device.id);
        for (const EntityId deviceId : deviceIds)
        {
            DeviceState* device = FindDevice(deviceId);
            if (!device) continue;
            if (device->remainingInitiative > 0) --device->remainingInitiative;
            if (device->armDelayInitiative > 0) --device->armDelayInitiative;
            if (device->enemyRevealInitiative > 0) --device->enemyRevealInitiative;
            if (device->remainingInitiative <= 0)
            {
                RemoveDevice(deviceId, result.events);
                continue;
            }
            if (device->triggerIntervalInitiative > 0)
            {
                ++device->triggerProgressInitiative;
                while (true)
                {
                    device = FindDevice(deviceId);
                    if (!device || device->triggerProgressInitiative < device->triggerIntervalInitiative) break;
                    device->triggerProgressInitiative -= device->triggerIntervalInitiative;
                    TriggerDevice(deviceId, false, result.events);
                }
            }
        }
        ++state_.globalInitiative;
        result.events.push_back(GameEvent{
            EventType::GlobalInitiativeAdvanced, InvalidEntityId, InvalidEntityId, InvalidEntityId, {}, 1});
        result.message = "Global initiative advanced.";

        Team soleTeam = Team::Neutral;
        bool multipleTeams = false;
        for (const CharacterState& character : state_.characters)
        {
            if (!character.alive || character.team == Team::Neutral) continue;
            if (soleTeam == Team::Neutral) soleTeam = character.team;
            else if (soleTeam != character.team) { multipleTeams = true; break; }
        }
        if (!multipleTeams && soleTeam != Team::Neutral)
        {
            state_.status = MatchStatus::Completed;
            state_.winner = soleTeam;
            state_.activeCharacterId = InvalidEntityId;
            result.events.push_back(GameEvent{EventType::MatchEnded, InvalidEntityId, InvalidEntityId,
                InvalidEntityId, {}, static_cast<std::int64_t>(soleTeam)});
        }
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

    for (const AbilityState& ability : character->abilities)
    {
        const auto definition = definitions_.abilities.find(ability.definitionId);
        if (definition != definitions_.abilities.end()
            && (definition->second.operation == AbilityOperation::GroundAura
                || definition->second.operation == AbilityOperation::HookShot
                || definition->second.operation == AbilityOperation::Fireball
                || definition->second.operation == AbilityOperation::Lightningstrike
                || definition->second.operation == AbilityOperation::Rockblast
                || definition->second.operation == AbilityOperation::ArcaneShift
                || definition->second.operation == AbilityOperation::BuildTurret
                || definition->second.operation == AbilityOperation::BuildMine
                || definition->second.operation == AbilityOperation::BuildShieldEmitter
                || definition->second.operation == AbilityOperation::BuildSpotterBeacon))
        {
            for (const TileState& tile : state_.tiles)
            {
                GameAction action;
                action.type = ActionType::UseAbility;
                action.actorId = actorId;
                action.targetTileId = tile.id;
                action.abilityId = ability.definitionId;
                if (ValidateAbility(action).success)
                {
                    actions.push_back(std::move(action));
                }
            }
            continue;
        }
        for (const CharacterState& target : state_.characters)
        {
            GameAction action;
            action.type = ActionType::UseAbility;
            action.actorId = actorId;
            action.targetEntityId = target.id;
            action.targetTileId = target.tileId;
            action.abilityId = ability.definitionId;
            if (ValidateAbility(action).success)
            {
                actions.push_back(std::move(action));
            }
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

const DeviceState* Game::FindDevice(const EntityId deviceId) const noexcept
{
    const auto device = std::find_if(state_.devices.begin(), state_.devices.end(),
        [deviceId](const DeviceState& candidate) { return candidate.id == deviceId; });
    return device == state_.devices.end() ? nullptr : &*device;
}

DeviceState* Game::FindDevice(const EntityId deviceId) noexcept
{
    return const_cast<DeviceState*>(static_cast<const Game*>(this)->FindDevice(deviceId));
}

void Game::RemoveDevice(const EntityId deviceId, std::vector<GameEvent>& events)
{
    const auto device = std::find_if(state_.devices.begin(), state_.devices.end(),
        [deviceId](const DeviceState& candidate) { return candidate.id == deviceId; });
    if (device == state_.devices.end()) return;
    if (TileState* tile = FindTile(device->tileId))
    {
        if (tile->fieldObjectId == deviceId) tile->fieldObjectId = InvalidEntityId;
        tile->fieldObjectHealth = 0;
    }
    events.push_back(GameEvent{EventType::DeviceRemoved, device->ownerId, device->id,
        device->tileId, device->sourceAbilityId, 0});
    state_.devices.erase(device);
}

bool Game::TriggerDevice(const EntityId deviceId, const bool manual, std::vector<GameEvent>& events)
{
    DeviceState* device = FindDevice(deviceId);
    if (!device || device->health <= 0) return false;
    CharacterState* owner = FindCharacter(device->ownerId);
    const TileState* center = FindTile(device->tileId);
    if (!owner || !owner->alive || !center) return false;
    const EntityId ownerId = owner->id;
    const EntityId tileId = device->tileId;
    const DefinitionId sourceId = device->sourceAbilityId;
    const auto damageTarget = [this, owner, &events, &sourceId](CharacterState& target,
        const std::int32_t amount, const std::int32_t defenseIgnore)
    {
        std::vector<GameEvent> auraEvents; std::vector<GameEvent> modifierEvents;
        const std::int32_t applied = ResolveDamage(*owner, target, amount, defenseIgnore, auraEvents, modifierEvents);
        events.push_back(GameEvent{EventType::DamageApplied, owner->id, target.id, target.tileId, sourceId, applied});
        events.insert(events.end(), auraEvents.begin(), auraEvents.end());
        events.insert(events.end(), modifierEvents.begin(), modifierEvents.end());
        if (target.health <= 0 && target.alive)
        {
            target.health = 0; target.alive = false;
            if (TileState* tile = FindTile(target.tileId)) tile->occupantId = InvalidEntityId;
            events.push_back(GameEvent{EventType::CharacterDied, owner->id, target.id, target.tileId, sourceId, 0});
        }
        return applied;
    };
    bool triggered = false;
    if (device->type == DeviceType::Turret)
    {
        std::vector<CharacterState*> targets;
        for (CharacterState& candidate : state_.characters)
        {
            const TileState* tile = FindTile(candidate.tileId);
            if (candidate.alive && candidate.team != owner->team && candidate.team != Team::Neutral && tile
                && HexDistance(center->coord, tile->coord) <= device->range) targets.push_back(&candidate);
        }
        std::sort(targets.begin(), targets.end(), [this, center](const CharacterState* left, const CharacterState* right)
        {
            const std::int32_t leftDistance = HexDistance(center->coord, FindTile(left->tileId)->coord);
            const std::int32_t rightDistance = HexDistance(center->coord, FindTile(right->tileId)->coord);
            return leftDistance != rightDistance ? leftDistance < rightDistance : left->id < right->id;
        });
        if (!targets.empty())
        {
            auto turretDamage = [this, device](const CharacterState& target, std::int32_t amount)
            {
                const CharacterDefinition& targetDefinition = definitions_.characters.at(target.definitionId);
                if (device->multiplierBasisPoints > 0 && targetDefinition.stats.maxHealth > 0
                    && target.health * 10000 <= targetDefinition.stats.maxHealth * device->secondaryAmount)
                    amount = static_cast<std::int32_t>(ApplyBasisPoints(amount, device->multiplierBasisPoints));
                return amount;
            };
            damageTarget(*targets[0], turretDamage(*targets[0], device->amount), device->defenseIgnore);
            if (targets[0]->alive && device->initiativeLoss > 0)
                targets[0]->initiative = std::max(0, targets[0]->initiative - device->initiativeLoss);
            if (targets.size() > 1 && device->secondaryMultiplierBasisPoints > 0)
                damageTarget(*targets[1], turretDamage(*targets[1], static_cast<std::int32_t>(
                    ApplyBasisPoints(device->amount, device->secondaryMultiplierBasisPoints))), device->defenseIgnore);
            triggered = true;
        }
    }
    else if (device->type == DeviceType::Mine)
    {
        if (!manual && device->armDelayInitiative > 0) return false;
        for (CharacterState& candidate : state_.characters)
        {
            const TileState* tile = FindTile(candidate.tileId);
            if (!candidate.alive || candidate.team == owner->team || candidate.team == Team::Neutral || !tile
                || HexDistance(center->coord, tile->coord) > device->range) continue;
            damageTarget(candidate, device->amount, device->defenseIgnore);
            if (candidate.alive && device->initiativeLoss > 0)
                candidate.initiative = std::max(0, candidate.initiative - device->initiativeLoss);
            if (candidate.alive && device->effectDurationInitiative > 0)
            {
                ModifierEffectState root; root.id = sourceId + ":Root"; root.causerId = ownerId;
                root.isDebuff = true; root.preventsMovement = true;
                root.remainingInitiative = device->effectDurationInitiative;
                candidate.modifierEffects.push_back(root);
                events.push_back(GameEvent{EventType::EffectAdded, ownerId, candidate.id, candidate.tileId,
                    root.id, root.remainingInitiative});
            }
        }
        triggered = true;
        if (!manual || (device->flags & 1u) == 0u) RemoveDevice(deviceId, events);
    }
    else if (device->type == DeviceType::ShieldEmitter)
    {
        std::vector<CharacterState*> allies;
        for (CharacterState& candidate : state_.characters)
        {
            const TileState* tile = FindTile(candidate.tileId);
            if (candidate.alive && candidate.team == owner->team && tile
                && HexDistance(center->coord, tile->coord) <= device->range) allies.push_back(&candidate);
        }
        for (CharacterState* ally : allies)
        {
            ModifierEffectState shield; shield.id = sourceId + ":Shield"; shield.kind = ModifierEffectKind::EngineerShield;
            shield.causerId = ownerId; shield.remainingInitiative = device->effectDurationInitiative;
            shield.valueA = static_cast<std::int32_t>(ApplyBasisPoints(device->amount,
                allies.size() == 1 && device->multiplierBasisPoints > 0 ? device->multiplierBasisPoints : 10000));
            ally->modifierEffects.erase(std::remove_if(ally->modifierEffects.begin(), ally->modifierEffects.end(),
                [&shield](const ModifierEffectState& effect) { return effect.id == shield.id; }), ally->modifierEffects.end());
            ally->modifierEffects.push_back(shield);
            const CharacterDefinition& allyDefinition = definitions_.characters.at(ally->definitionId);
            const std::int32_t healed = std::min(device->secondaryAmount, std::max(0, allyDefinition.stats.maxHealth - ally->health));
            ally->health += healed; ally->initiative += std::max(0, device->initiativeLoss);
            if ((device->flags & 2u) != 0u)
            {
                const auto debuff = std::find_if(ally->modifierEffects.begin(), ally->modifierEffects.end(),
                    [](const ModifierEffectState& effect) { return effect.isDebuff; });
                if (debuff != ally->modifierEffects.end()) ally->modifierEffects.erase(debuff);
            }
            events.push_back(GameEvent{EventType::EffectAdded, ownerId, ally->id, ally->tileId,
                shield.id, shield.valueA});
        }
        triggered = !allies.empty();
    }
    else if (device->type == DeviceType::SpotterBeacon)
    {
        for (CharacterState& enemy : state_.characters)
        {
            const TileState* tile = FindTile(enemy.tileId);
            if (!enemy.alive || enemy.team == owner->team || enemy.team == Team::Neutral || !tile
                || HexDistance(center->coord, tile->coord) > device->range) continue;
            ModifierEffectState mark; mark.id = sourceId + ":Spotted"; mark.kind = ModifierEffectKind::EngineerSpotted;
            mark.causerId = ownerId; mark.isDebuff = true; mark.remainingInitiative = device->effectDurationInitiative;
            mark.incomingDamageMultiplierBasisPoints = device->multiplierBasisPoints > 0 ? device->multiplierBasisPoints : 10000;
            mark.outgoingDamageMultiplierBasisPoints = device->secondaryMultiplierBasisPoints > 0 ? device->secondaryMultiplierBasisPoints : 10000;
            mark.moveCostAdd = device->moveCostAdd;
            enemy.modifierEffects.erase(std::remove_if(enemy.modifierEffects.begin(), enemy.modifierEffects.end(),
                [&mark](const ModifierEffectState& effect) { return effect.id == mark.id; }), enemy.modifierEffects.end());
            enemy.modifierEffects.push_back(mark);
            enemy.initiative = std::max(0, enemy.initiative - device->initiativeLoss);
            events.push_back(GameEvent{EventType::EffectAdded, ownerId, enemy.id, enemy.tileId,
                mark.id, mark.remainingInitiative});
            triggered = true;
        }
    }
    if (triggered) events.push_back(GameEvent{EventType::DeviceTriggered, ownerId, deviceId, tileId, sourceId, 1});
    return triggered;
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
    std::int64_t cost = std::max<std::int64_t>(terrainDefinition->second.movementCost, scaledCost);
    for (const ModifierEffectState& effect : character.modifierEffects)
    {
        cost = std::max<std::int64_t>(1,
            (cost * std::max(0, effect.moveCostMultiplierBasisPoints) + 5000) / 10000);
        cost = std::max<std::int64_t>(1, cost + effect.moveCostAdd);
        if (effect.blockMovementBonuses)
        {
            cost = std::max(cost, baseCost);
        }
    }
    return cost > std::numeric_limits<std::int32_t>::max()
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(std::max<std::int64_t>(1, cost));
}

std::int32_t Game::BasicAttackCost(const CharacterState& character) const noexcept
{
    const auto definition = definitions_.characters.find(character.definitionId);
    if (definition == definitions_.characters.end())
    {
        return std::numeric_limits<std::int32_t>::max();
    }
    std::int64_t cost = std::max(0, definition->second.stats.attackCost);
    if (PassiveValue(definition->second, "CoreComboCharacter") > 0.0 && !character.attackedThisTurn)
        cost = std::max<std::int64_t>(1, cost - std::max(0, static_cast<std::int32_t>(
            PassiveValue(definition->second, "FirstBasicAttackCostReduction"))));
    for (const ModifierEffectState& effect : character.modifierEffects)
    {
        if (effect.basicAttackCostAvailable)
        {
            cost = std::max<std::int64_t>(0, cost + effect.basicAttackCostAdd);
        }
    }
    return cost > std::numeric_limits<std::int32_t>::max()
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(cost);
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
    if (character->movementBlocked || std::any_of(
            character->modifierEffects.begin(), character->modifierEffects.end(),
            [](const ModifierEffectState& effect) { return effect.preventsMovement; }))
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
    if (destination->fieldObjectId != InvalidEntityId)
    {
        const DeviceState* device = FindDevice(destination->fieldObjectId);
        if (device && device->type == DeviceType::Turret)
            return Failure("destination_blocked_by_device", "A blocking device occupies the destination.");
    }

    const std::int32_t cost = (character->hasFreePreparedStep
        || (character->freeAdjacentMoveAvailable && !character->freeAdjacentMoveUsedThisTurn))
        ? 0 : MovementCost(*character, *destination);
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
        HashValue(hash, tile.fieldObjectId);
        HashValue(hash, tile.blocksMovement);
        HashValue(hash, tile.fieldObjectHealth);
        HashValue(hash, tile.terrainObjectHealth);
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
        HashValue(hash, character.resourceCurrent);
        HashValue(hash, character.resourceMaximum);
        HashValue(hash, character.resourceRechargeProgress);
        HashValue(hash, character.resourceRechargeInterval);
        HashValue(hash, character.specialCooldownRemaining);
        HashValue(hash, character.firstResourceSpendRefundAvailable);
        HashValue(hash, character.movedSinceLastTurn);
        HashValue(hash, character.stationaryGuardActive);
        HashValue(hash, character.attackedThisTurn); HashValue(hash, character.spentResourceThisTurn);
        HashValue(hash, character.startedTurnWithFullResource); HashValue(hash, character.heldPositionFromPreviousTurn);
        HashValue(hash, character.usedFreeBasicAttackThisTurn); HashValue(hash, character.triggeredEmptyResourceThisTurn);
        HashValue(hash, character.refundedLastResourceKillThisTurn); HashValue(hash, character.usedRangeRefundThisTurn);
        HashValue(hash, character.usedKillCooldownReductionThisTurn); HashValue(hash, character.stationaryShotStacks);
        HashValue(hash, character.hasFreePreparedStep);
        HashValue(hash, character.movedFieldsThisTurn); HashValue(hash, character.resourceSpentThisTurnCount);
        HashValue(hash, character.secondaryCooldownRemaining); HashValue(hash, character.turnSequence);
        HashValue(hash, character.openingRewardUsedThisTurn); HashValue(hash, character.overflowRewardUsedThisTurn);
        HashValue(hash, character.freeAdjacentMoveAvailable); HashValue(hash, character.freeAdjacentMoveUsedThisTurn);
        HashValue(hash, character.nextDamageAfterMoveReady); HashValue(hash, character.nextReductionAfterMoveReady);
        HashValue(hash, character.startInvisibilityApplied);
        HashValue(hash, character.activeElement); HashValue(hash, character.usedElementMask);
        HashValue(hash, character.lastCastElement); HashValue(hash, character.firstCastElement);
        HashValue(hash, character.elementSwapCooldownRemaining); HashValue(hash, character.rockDefenseBonus);
        HashValue(hash, character.rockDefenseRemainingInitiative); HashValue(hash, character.protectionRemainingInitiative);
        HashValue(hash, character.echoElement); HashValue(hash, character.elementCycleMask);
        HashValue(hash, character.elementCostReductionUsed); HashValue(hash, character.elementSwapRefundUsed);
        HashValue(hash, character.storedElementEmpowered); HashValue(hash, character.protectionReady);
        HashValue(hash, character.firstBloodDamageReductionAvailable); HashValue(hash, character.basicAttackBloodAvailable);
        HashValue(hash, character.bloodCapTriggerAvailable);
        for (const AbilityState& ability : character.abilities)
        {
            HashString(hash, ability.definitionId);
            HashValue(hash, ability.cooldownRemaining);
            HashValue(hash, ability.cooldownDuration);
            HashValue(hash, ability.currentCharges);
            HashValue(hash, ability.maxCharges);
            HashValue(hash, ability.usesCharges);
            HashValue(hash, ability.readyInitiativeElapsed);
        }
        for (const ModifierEffectState& effect : character.modifierEffects)
        {
            HashString(hash, effect.id); HashValue(hash, effect.causerId); HashValue(hash, effect.preferredTargetId);
            HashValue(hash, effect.remainingInitiative); HashValue(hash, effect.remainingTurns);
            HashValue(hash, effect.incomingDamageMultiplierBasisPoints);
            HashValue(hash, effect.outgoingDamageMultiplierBasisPoints);
            HashValue(hash, effect.defenseIgnoreAdd);
            HashValue(hash, effect.healFractionFromDamageDealtBasisPoints);
            HashValue(hash, effect.moveCostAdd);
            HashValue(hash, effect.moveCostMultiplierBasisPoints);
            HashValue(hash, effect.initiativeLossOnMove);
            HashValue(hash, effect.firstMoveInitiativeLoss);
            HashValue(hash, effect.maximumMovedFields);
            HashValue(hash, effect.movedFields);
            HashValue(hash, effect.frostbreakDamage);
            HashValue(hash, effect.basicAttackCostAdd);
            HashValue(hash, effect.basicAttackDamageMultiplierBasisPoints);
            HashValue(hash, effect.incomingDamageMultiplierFromCauserBasisPoints);
            HashValue(hash, effect.initiativeLossOnExpire);
            HashValue(hash, effect.incomingEffectDurationAdd);
            HashValue(hash, effect.outgoingDamageMultiplierAgainstOtherTargetsBasisPoints);
            HashValue(hash, effect.initiativeLossWhenIgnoringPreferredTarget);
            HashValue(hash, effect.initiativeGainForCauserWhenAttacked);
            HashValue(hash, effect.triggerDamageOnMove); HashValue(hash, effect.triggerDelayOnMove);
            HashValue(hash, effect.triggerOutgoingDamageMultiplierBasisPoints);
            HashValue(hash, effect.triggerSlowMultiplierBasisPoints);
            HashValue(hash, effect.triggerSlowDurationInitiative);
            HashValue(hash, effect.triggerAreaRadius); HashValue(hash, effect.triggerAreaInitiativeLoss);
            HashValue(hash, effect.triggerInitiativeGainForCauser);
            HashValue(hash, effect.redirectPreMitigationBasisPoints);
            HashValue(hash, effect.redirectedDamageMultiplierBasisPoints);
            HashValue(hash, effect.protectorHealFractionBasisPoints);
            HashValue(hash, effect.protectedInitiativePerHit);
            HashValue(hash, effect.areaCenterTileId);
            HashValue(hash, effect.areaRadius);
            HashValue(hash, effect.areaDamage);
            HashValue(hash, effect.areaDamageType);
            HashValue(hash, effect.areaInitiativeLossPerPulse);
            HashValue(hash, effect.areaFollowsCauser);
            HashValue(hash, effect.onlyAffectCauserDamage);
            HashValue(hash, effect.onlyAgainstRangedSources);
            HashValue(hash, effect.blockMovementBonuses);
            HashValue(hash, effect.basicAttackCostAvailable);
            HashValue(hash, effect.consumeOnBasicAttack); HashValue(hash, effect.consumeOnOutgoingDamage);
            HashValue(hash, effect.preventsAbilityUse);
            HashValue(hash, effect.preventsMovement);
            HashValue(hash, effect.firstMovePenaltyAvailable);
            HashValue(hash, effect.expireOnTurnEnd);
            HashValue(hash, effect.isDebuff); HashValue(hash, effect.triggerOnMove);
            HashValue(hash, effect.consumeOnDamageFromCauser);
            HashString(hash, effect.sourceAbilityId); HashValue(hash, effect.kind);
            HashValue(hash, effect.counterA); HashValue(hash, effect.counterB); HashValue(hash, effect.counterC);
            HashValue(hash, effect.valueA); HashValue(hash, effect.valueB); HashValue(hash, effect.valueC);
            HashValue(hash, effect.flags);
        }
        for (const RecentAuraPreventionState& prevention : character.recentAuraPreventions)
        {
            HashValue(hash, prevention.sourceId);
            HashValue(hash, prevention.globalInitiative);
        }
    }
    for (const DeviceState& device : state_.devices)
    {
        HashValue(hash, device.id); HashValue(hash, device.ownerId); HashValue(hash, device.tileId);
        HashString(hash, device.sourceAbilityId); HashValue(hash, device.type);
        HashValue(hash, device.health); HashValue(hash, device.maximumHealth); HashValue(hash, device.remainingInitiative);
        HashValue(hash, device.triggerIntervalInitiative); HashValue(hash, device.triggerProgressInitiative);
        HashValue(hash, device.armDelayInitiative); HashValue(hash, device.enemyRevealInitiative);
        HashValue(hash, device.range); HashValue(hash, device.amount); HashValue(hash, device.effectDurationInitiative);
        HashValue(hash, device.defenseIgnore); HashValue(hash, device.initiativeLoss); HashValue(hash, device.moveCostAdd);
        HashValue(hash, device.secondaryAmount); HashValue(hash, device.multiplierBasisPoints);
        HashValue(hash, device.secondaryMultiplierBasisPoints); HashValue(hash, device.damageType); HashValue(hash, device.flags);
    }
    return hash;
}
}
