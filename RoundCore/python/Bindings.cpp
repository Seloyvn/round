#include "round/core/VectorEnvironment.h"
#include "round/core/Package.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace roundcore;

PYBIND11_MODULE(roundcore_python, module)
{
    module.doc() = "Standalone Python bindings for the authoritative RoundCore rules";
    module.attr("GLOBAL_STRIDE") = NumericObservation::GlobalStride;
    module.attr("TILE_STRIDE") = NumericObservation::TileStride;
    module.attr("CHARACTER_STRIDE") = NumericObservation::CharacterStride;
    module.attr("ABILITY_STRIDE") = NumericObservation::AbilityStride;
    module.attr("MODIFIER_EFFECT_STRIDE") = NumericObservation::ModifierEffectStride;
    module.attr("ACTION_STRIDE") = NumericObservation::ActionStride;
    module.attr("ACTION_SELECTION_STRIDE") = NumericObservation::ActionSelectionStride;
    module.attr("MATCH_PACKAGE_VERSION") = MatchPackageVersion;
    module.def("serialize_match_setup", [](const MatchSetup& setup)
    {
        const std::vector<std::uint8_t> bytes = SerializeMatchSetup(setup);
        return py::bytes(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    });
    module.def("deserialize_match_setup", [](const py::bytes& data)
    {
        const std::string bytes = data;
        return DeserializeMatchSetup(std::vector<std::uint8_t>(bytes.begin(), bytes.end()));
    });

    py::enum_<Team>(module, "Team")
        .value("NEUTRAL", Team::Neutral).value("TEAM_A", Team::TeamA).value("TEAM_B", Team::TeamB);
    py::enum_<ActionType>(module, "ActionType")
        .value("MOVE", ActionType::Move).value("BASIC_ATTACK", ActionType::BasicAttack)
        .value("USE_ABILITY", ActionType::UseAbility).value("CLAIM_OBJECTIVE", ActionType::ClaimObjective)
        .value("WAIT", ActionType::Wait).value("END_TURN", ActionType::EndTurn);
    py::enum_<AbilityOperation>(module, "AbilityOperation")
        .value("UNSUPPORTED", AbilityOperation::Unsupported)
        .value("DIRECT_DAMAGE", AbilityOperation::DirectDamage)
        .value("DIRECT_HEAL", AbilityOperation::DirectHeal)
        .value("SELF_MODIFIER", AbilityOperation::SelfModifier)
        .value("TARGET_MODIFIER", AbilityOperation::TargetModifier)
        .value("GROUND_AURA", AbilityOperation::GroundAura)
        .value("KNOCKBACK", AbilityOperation::Knockback)
        .value("DELAY_STRIKE", AbilityOperation::DelayStrike)
        .value("SHACKLE", AbilityOperation::Shackle)
        .value("BIND", AbilityOperation::Bind)
        .value("TAUNT", AbilityOperation::Taunt)
        .value("SNIPE", AbilityOperation::Snipe)
        .value("CONCENTRATE", AbilityOperation::Concentrate)
        .value("AIM", AbilityOperation::Aim)
        .value("PIERCING_SHOT", AbilityOperation::PiercingShot)
        .value("SUPPRESSING_FIRE", AbilityOperation::SuppressingFire);
    py::enum_<ModifierEffectKind>(module, "ModifierEffectKind")
        .value("GENERIC", ModifierEffectKind::Generic)
        .value("CONCENTRATE", ModifierEffectKind::Concentrate)
        .value("AIM_FIELD", ModifierEffectKind::AimField)
        .value("SUPPRESSING_FIRE", ModifierEffectKind::SuppressingFire)
        .value("PIERCING_MARK", ModifierEffectKind::PiercingMark)
        .value("TRACKING_MARK", ModifierEffectKind::TrackingMark);
    py::enum_<AbilityTargetRelation>(module, "AbilityTargetRelation")
        .value("SELF", AbilityTargetRelation::Self).value("ALLY", AbilityTargetRelation::Ally)
        .value("ENEMY", AbilityTargetRelation::Enemy).value("ANY_CHARACTER", AbilityTargetRelation::AnyCharacter);
    py::enum_<DamageType>(module, "DamageType")
        .value("PHYSICAL", DamageType::Physical).value("MAGICAL", DamageType::Magical)
        .value("FIRE", DamageType::Fire).value("ICE", DamageType::Ice)
        .value("LIGHTNING", DamageType::Lightning).value("HOLY", DamageType::Holy)
        .value("GROUND", DamageType::Ground).value("TRUE_DAMAGE", DamageType::TrueDamage)
        .value("NATURE", DamageType::Nature);
    py::enum_<MatchStatus>(module, "MatchStatus")
        .value("NOT_STARTED", MatchStatus::NotStarted).value("RUNNING", MatchStatus::Running)
        .value("COMPLETED", MatchStatus::Completed);
    py::enum_<EventType>(module, "EventType")
        .value("MATCH_RESET", EventType::MatchReset).value("CHARACTER_MOVED", EventType::CharacterMoved)
        .value("DAMAGE_APPLIED", EventType::DamageApplied).value("HEALING_APPLIED", EventType::HealingApplied)
        .value("INITIATIVE_CHANGED", EventType::InitiativeChanged).value("COOLDOWN_CHANGED", EventType::CooldownChanged)
        .value("ABILITY_CHARGES_CHANGED", EventType::AbilityChargesChanged).value("EFFECT_ADDED", EventType::EffectAdded)
        .value("RESOURCE_CHANGED", EventType::ResourceChanged)
        .value("EFFECT_REMOVED", EventType::EffectRemoved).value("TERRAIN_CHANGED", EventType::TerrainChanged)
        .value("GLOBAL_INITIATIVE_ADVANCED", EventType::GlobalInitiativeAdvanced)
        .value("CHARACTER_DIED", EventType::CharacterDied).value("ACTIVE_CHARACTER_CHANGED", EventType::ActiveCharacterChanged)
        .value("OBJECTIVE_CHANGED", EventType::ObjectiveChanged).value("MATCH_ENDED", EventType::MatchEnded);

    py::class_<HexCoord>(module, "HexCoord")
        .def(py::init<>()).def_readwrite("q", &HexCoord::q).def_readwrite("r", &HexCoord::r);
    py::class_<StatBlock>(module, "StatBlock")
        .def(py::init<>())
        .def_readwrite("max_health", &StatBlock::maxHealth).def_readwrite("attack", &StatBlock::attack)
        .def_readwrite("defense", &StatBlock::defense).def_readwrite("max_initiative", &StatBlock::maxInitiative)
        .def_readwrite("vision_range", &StatBlock::visionRange).def_readwrite("attack_range", &StatBlock::attackRange)
        .def_readwrite("move_cost", &StatBlock::moveCost).def_readwrite("attack_cost", &StatBlock::attackCost)
        .def_readwrite("turn_delay", &StatBlock::turnDelay);
    py::class_<AbilityDefinition>(module, "AbilityDefinition")
        .def(py::init<>()).def_readwrite("id", &AbilityDefinition::id)
        .def_readwrite("operation", &AbilityDefinition::operation)
        .def_readwrite("target_relation", &AbilityDefinition::targetRelation)
        .def_readwrite("initiative_cost", &AbilityDefinition::initiativeCost)
        .def_readwrite("range", &AbilityDefinition::range)
        .def_readwrite("amount", &AbilityDefinition::amount)
        .def_readwrite("defense_ignore", &AbilityDefinition::defenseIgnore)
        .def_readwrite("damage_type", &AbilityDefinition::damageType)
        .def_readwrite("requires_visibility", &AbilityDefinition::requiresVisibility)
        .def_readwrite("values", &AbilityDefinition::values);
    py::class_<CharacterDefinition>(module, "CharacterDefinition")
        .def(py::init<>()).def_readwrite("id", &CharacterDefinition::id)
        .def_readwrite("stats", &CharacterDefinition::stats)
        .def_readwrite("ability_ids", &CharacterDefinition::abilityIds)
        .def_readwrite("passive_values", &CharacterDefinition::passiveValues)
        .def_readwrite("critical_chance_basis_points", &CharacterDefinition::criticalChanceBasisPoints)
        .def_readwrite("critical_damage_multiplier_basis_points", &CharacterDefinition::criticalDamageMultiplierBasisPoints);
    py::class_<TerrainDefinition>(module, "TerrainDefinition")
        .def(py::init<>()).def_readwrite("id", &TerrainDefinition::id)
        .def_readwrite("walkable", &TerrainDefinition::walkable)
        .def_readwrite("blocks_vision", &TerrainDefinition::blocksVision)
        .def_readwrite("stops_movement", &TerrainDefinition::stopsMovement)
        .def_readwrite("movement_cost", &TerrainDefinition::movementCost)
        .def_readwrite("movement_cost_multiplier_basis_points", &TerrainDefinition::movementCostMultiplierBasisPoints)
        .def_readwrite("values", &TerrainDefinition::values)
        .def_readwrite("vision_cost", &TerrainDefinition::visionCost)
        .def_readwrite("vision_range_modifier", &TerrainDefinition::visionRangeModifier);
    py::class_<ContentDefinitions>(module, "ContentDefinitions")
        .def(py::init<>()).def_readwrite("schema_version", &ContentDefinitions::schemaVersion)
        .def_readwrite("content_version", &ContentDefinitions::contentVersion)
        .def_readwrite("characters", &ContentDefinitions::characters)
        .def_readwrite("abilities", &ContentDefinitions::abilities)
        .def_readwrite("terrains", &ContentDefinitions::terrains);

    py::class_<TileState>(module, "TileState")
        .def(py::init<>()).def_readwrite("id", &TileState::id).def_readwrite("coord", &TileState::coord)
        .def_readwrite("terrain_id", &TileState::terrainId).def_readwrite("occupant_id", &TileState::occupantId)
        .def_readwrite("blocks_movement", &TileState::blocksMovement)
        .def_readwrite("field_object_health", &TileState::fieldObjectHealth)
        .def_readwrite("terrain_object_health", &TileState::terrainObjectHealth);
    py::class_<AbilityState>(module, "AbilityState")
        .def(py::init<>()).def_readwrite("definition_id", &AbilityState::definitionId)
        .def_readwrite("cooldown_remaining", &AbilityState::cooldownRemaining)
        .def_readwrite("cooldown_duration", &AbilityState::cooldownDuration)
        .def_readwrite("current_charges", &AbilityState::currentCharges)
        .def_readwrite("max_charges", &AbilityState::maxCharges)
        .def_readwrite("uses_charges", &AbilityState::usesCharges)
        .def_readwrite("ready_initiative_elapsed", &AbilityState::readyInitiativeElapsed);
    py::class_<ModifierEffectState>(module, "ModifierEffectState")
        .def(py::init<>()).def_readwrite("id", &ModifierEffectState::id)
        .def_readwrite("causer_id", &ModifierEffectState::causerId)
        .def_readwrite("preferred_target_id", &ModifierEffectState::preferredTargetId)
        .def_readwrite("remaining_initiative", &ModifierEffectState::remainingInitiative)
        .def_readwrite("remaining_turns", &ModifierEffectState::remainingTurns)
        .def_readwrite("incoming_damage_multiplier_basis_points", &ModifierEffectState::incomingDamageMultiplierBasisPoints)
        .def_readwrite("outgoing_damage_multiplier_basis_points", &ModifierEffectState::outgoingDamageMultiplierBasisPoints)
        .def_readwrite("defense_ignore_add", &ModifierEffectState::defenseIgnoreAdd)
        .def_readwrite("heal_fraction_from_damage_dealt_basis_points", &ModifierEffectState::healFractionFromDamageDealtBasisPoints)
        .def_readwrite("move_cost_add", &ModifierEffectState::moveCostAdd)
        .def_readwrite("move_cost_multiplier_basis_points", &ModifierEffectState::moveCostMultiplierBasisPoints)
        .def_readwrite("initiative_loss_on_move", &ModifierEffectState::initiativeLossOnMove)
        .def_readwrite("first_move_initiative_loss", &ModifierEffectState::firstMoveInitiativeLoss)
        .def_readwrite("maximum_moved_fields", &ModifierEffectState::maximumMovedFields)
        .def_readwrite("moved_fields", &ModifierEffectState::movedFields)
        .def_readwrite("frostbreak_damage", &ModifierEffectState::frostbreakDamage)
        .def_readwrite("basic_attack_cost_add", &ModifierEffectState::basicAttackCostAdd)
        .def_readwrite("basic_attack_damage_multiplier_basis_points", &ModifierEffectState::basicAttackDamageMultiplierBasisPoints)
        .def_readwrite("incoming_damage_multiplier_from_causer_basis_points", &ModifierEffectState::incomingDamageMultiplierFromCauserBasisPoints)
        .def_readwrite("initiative_loss_on_expire", &ModifierEffectState::initiativeLossOnExpire)
        .def_readwrite("incoming_effect_duration_add", &ModifierEffectState::incomingEffectDurationAdd)
        .def_readwrite("outgoing_damage_multiplier_against_other_targets_basis_points", &ModifierEffectState::outgoingDamageMultiplierAgainstOtherTargetsBasisPoints)
        .def_readwrite("initiative_loss_when_ignoring_preferred_target", &ModifierEffectState::initiativeLossWhenIgnoringPreferredTarget)
        .def_readwrite("initiative_gain_for_causer_when_attacked", &ModifierEffectState::initiativeGainForCauserWhenAttacked)
        .def_readwrite("trigger_damage_on_move", &ModifierEffectState::triggerDamageOnMove)
        .def_readwrite("trigger_delay_on_move", &ModifierEffectState::triggerDelayOnMove)
        .def_readwrite("trigger_outgoing_damage_multiplier_basis_points", &ModifierEffectState::triggerOutgoingDamageMultiplierBasisPoints)
        .def_readwrite("trigger_slow_multiplier_basis_points", &ModifierEffectState::triggerSlowMultiplierBasisPoints)
        .def_readwrite("trigger_slow_duration_initiative", &ModifierEffectState::triggerSlowDurationInitiative)
        .def_readwrite("trigger_area_radius", &ModifierEffectState::triggerAreaRadius)
        .def_readwrite("trigger_area_initiative_loss", &ModifierEffectState::triggerAreaInitiativeLoss)
        .def_readwrite("trigger_initiative_gain_for_causer", &ModifierEffectState::triggerInitiativeGainForCauser)
        .def_readwrite("redirect_pre_mitigation_basis_points", &ModifierEffectState::redirectPreMitigationBasisPoints)
        .def_readwrite("redirected_damage_multiplier_basis_points", &ModifierEffectState::redirectedDamageMultiplierBasisPoints)
        .def_readwrite("protector_heal_fraction_basis_points", &ModifierEffectState::protectorHealFractionBasisPoints)
        .def_readwrite("protected_initiative_per_hit", &ModifierEffectState::protectedInitiativePerHit)
        .def_readwrite("area_center_tile_id", &ModifierEffectState::areaCenterTileId)
        .def_readwrite("area_radius", &ModifierEffectState::areaRadius)
        .def_readwrite("area_damage", &ModifierEffectState::areaDamage)
        .def_readwrite("area_damage_type", &ModifierEffectState::areaDamageType)
        .def_readwrite("area_initiative_loss_per_pulse", &ModifierEffectState::areaInitiativeLossPerPulse)
        .def_readwrite("area_follows_causer", &ModifierEffectState::areaFollowsCauser)
        .def_readwrite("only_affect_causer_damage", &ModifierEffectState::onlyAffectCauserDamage)
        .def_readwrite("only_against_ranged_sources", &ModifierEffectState::onlyAgainstRangedSources)
        .def_readwrite("block_movement_bonuses", &ModifierEffectState::blockMovementBonuses)
        .def_readwrite("basic_attack_cost_available", &ModifierEffectState::basicAttackCostAvailable)
        .def_readwrite("consume_on_basic_attack", &ModifierEffectState::consumeOnBasicAttack)
        .def_readwrite("consume_on_outgoing_damage", &ModifierEffectState::consumeOnOutgoingDamage)
        .def_readwrite("prevents_ability_use", &ModifierEffectState::preventsAbilityUse)
        .def_readwrite("prevents_movement", &ModifierEffectState::preventsMovement)
        .def_readwrite("first_move_penalty_available", &ModifierEffectState::firstMovePenaltyAvailable)
        .def_readwrite("expire_on_turn_end", &ModifierEffectState::expireOnTurnEnd)
        .def_readwrite("is_debuff", &ModifierEffectState::isDebuff)
        .def_readwrite("trigger_on_move", &ModifierEffectState::triggerOnMove)
        .def_readwrite("consume_on_damage_from_causer", &ModifierEffectState::consumeOnDamageFromCauser)
        .def_readwrite("source_ability_id", &ModifierEffectState::sourceAbilityId)
        .def_readwrite("kind", &ModifierEffectState::kind)
        .def_readwrite("counter_a", &ModifierEffectState::counterA)
        .def_readwrite("counter_b", &ModifierEffectState::counterB)
        .def_readwrite("counter_c", &ModifierEffectState::counterC)
        .def_readwrite("value_a", &ModifierEffectState::valueA)
        .def_readwrite("value_b", &ModifierEffectState::valueB)
        .def_readwrite("value_c", &ModifierEffectState::valueC)
        .def_readwrite("flags", &ModifierEffectState::flags);
    py::class_<RecentAuraPreventionState>(module, "RecentAuraPreventionState")
        .def(py::init<>())
        .def_readwrite("source_id", &RecentAuraPreventionState::sourceId)
        .def_readwrite("global_initiative", &RecentAuraPreventionState::globalInitiative);
    py::class_<CharacterState>(module, "CharacterState")
        .def(py::init<>()).def_readwrite("id", &CharacterState::id)
        .def_readwrite("definition_id", &CharacterState::definitionId).def_readwrite("team", &CharacterState::team)
        .def_readwrite("tile_id", &CharacterState::tileId).def_readwrite("health", &CharacterState::health)
        .def_readwrite("initiative", &CharacterState::initiative).def_readwrite("alive", &CharacterState::alive)
        .def_readwrite("movement_blocked", &CharacterState::movementBlocked)
        .def_readwrite("can_take_turn", &CharacterState::canTakeTurn)
        .def_readwrite("initiative_gain_per_tick", &CharacterState::initiativeGainPerTick)
        .def_readwrite("delayed_action_initiative", &CharacterState::delayedActionInitiative)
        .def_readwrite("resource_current", &CharacterState::resourceCurrent)
        .def_readwrite("resource_maximum", &CharacterState::resourceMaximum)
        .def_readwrite("resource_recharge_progress", &CharacterState::resourceRechargeProgress)
        .def_readwrite("resource_recharge_interval", &CharacterState::resourceRechargeInterval)
        .def_readwrite("special_cooldown_remaining", &CharacterState::specialCooldownRemaining)
        .def_readwrite("first_resource_spend_refund_available", &CharacterState::firstResourceSpendRefundAvailable)
        .def_readwrite("moved_since_last_turn", &CharacterState::movedSinceLastTurn)
        .def_readwrite("stationary_guard_active", &CharacterState::stationaryGuardActive)
        .def_readwrite("attacked_this_turn", &CharacterState::attackedThisTurn)
        .def_readwrite("spent_resource_this_turn", &CharacterState::spentResourceThisTurn)
        .def_readwrite("started_turn_with_full_resource", &CharacterState::startedTurnWithFullResource)
        .def_readwrite("held_position_from_previous_turn", &CharacterState::heldPositionFromPreviousTurn)
        .def_readwrite("used_free_basic_attack_this_turn", &CharacterState::usedFreeBasicAttackThisTurn)
        .def_readwrite("triggered_empty_resource_this_turn", &CharacterState::triggeredEmptyResourceThisTurn)
        .def_readwrite("refunded_last_resource_kill_this_turn", &CharacterState::refundedLastResourceKillThisTurn)
        .def_readwrite("used_range_refund_this_turn", &CharacterState::usedRangeRefundThisTurn)
        .def_readwrite("used_kill_cooldown_reduction_this_turn", &CharacterState::usedKillCooldownReductionThisTurn)
        .def_readwrite("stationary_shot_stacks", &CharacterState::stationaryShotStacks)
        .def_readwrite("has_free_prepared_step", &CharacterState::hasFreePreparedStep)
        .def_readwrite("moved_fields_this_turn", &CharacterState::movedFieldsThisTurn)
        .def_readwrite("resource_spent_this_turn_count", &CharacterState::resourceSpentThisTurnCount)
        .def_readwrite("secondary_cooldown_remaining", &CharacterState::secondaryCooldownRemaining)
        .def_readwrite("turn_sequence", &CharacterState::turnSequence)
        .def_readwrite("opening_reward_used_this_turn", &CharacterState::openingRewardUsedThisTurn)
        .def_readwrite("overflow_reward_used_this_turn", &CharacterState::overflowRewardUsedThisTurn)
        .def_readwrite("free_adjacent_move_available", &CharacterState::freeAdjacentMoveAvailable)
        .def_readwrite("free_adjacent_move_used_this_turn", &CharacterState::freeAdjacentMoveUsedThisTurn)
        .def_readwrite("next_damage_after_move_ready", &CharacterState::nextDamageAfterMoveReady)
        .def_readwrite("next_reduction_after_move_ready", &CharacterState::nextReductionAfterMoveReady)
        .def_readwrite("start_invisibility_applied", &CharacterState::startInvisibilityApplied)
        .def_readwrite("abilities", &CharacterState::abilities)
        .def_readwrite("modifier_effects", &CharacterState::modifierEffects)
        .def_readwrite("recent_aura_preventions", &CharacterState::recentAuraPreventions);
    py::class_<GameState>(module, "GameState")
        .def(py::init<>()).def_readwrite("schema_version", &GameState::schemaVersion)
        .def_readwrite("match_id", &GameState::matchId).def_readwrite("seed", &GameState::seed)
        .def_readwrite("step", &GameState::step).def_readwrite("status", &GameState::status)
        .def_readwrite("winner", &GameState::winner).def_readwrite("active_character_id", &GameState::activeCharacterId)
        .def_readwrite("global_initiative", &GameState::globalInitiative)
        .def_readwrite("tiles", &GameState::tiles).def_readwrite("characters", &GameState::characters);
    py::class_<MatchSetup>(module, "MatchSetup")
        .def(py::init<>()).def_readwrite("definitions", &MatchSetup::definitions)
        .def_readwrite("initial_state", &MatchSetup::initialState);

    py::class_<GameAction>(module, "GameAction")
        .def(py::init<>()).def_readwrite("type", &GameAction::type).def_readwrite("actor_id", &GameAction::actorId)
        .def_readwrite("target_entity_id", &GameAction::targetEntityId)
        .def_readwrite("target_tile_id", &GameAction::targetTileId).def_readwrite("ability_id", &GameAction::abilityId)
        .def_readwrite("selections", &GameAction::selections);
    py::class_<GameEvent>(module, "GameEvent")
        .def_readonly("type", &GameEvent::type).def_readonly("source_id", &GameEvent::sourceId)
        .def_readonly("target_id", &GameEvent::targetId)
        .def_readonly("tile_id", &GameEvent::tileId).def_readonly("definition_id", &GameEvent::definitionId)
        .def_readonly("amount", &GameEvent::amount);
    py::class_<ActionResult>(module, "ActionResult")
        .def_readonly("success", &ActionResult::success).def_readonly("error_code", &ActionResult::errorCode)
        .def_readonly("message", &ActionResult::message).def_readonly("events", &ActionResult::events);
    py::class_<DecisionSnapshot>(module, "DecisionSnapshot")
        .def_readonly("state", &DecisionSnapshot::state).def_readonly("legal_actions", &DecisionSnapshot::legalActions);
    py::class_<ObservationSchema>(module, "ObservationSchema")
        .def_readonly("schema_version", &ObservationSchema::schemaVersion)
        .def_readonly("content_version", &ObservationSchema::contentVersion);
    py::class_<NumericObservation>(module, "NumericObservation")
        .def_readonly("schema_version", &NumericObservation::schemaVersion)
        .def_readonly("content_version", &NumericObservation::contentVersion)
        .def_readonly("global_values", &NumericObservation::global).def_readonly("tiles", &NumericObservation::tiles)
        .def_readonly("characters", &NumericObservation::characters).def_readonly("abilities", &NumericObservation::abilities)
        .def_readonly("modifier_effects", &NumericObservation::modifierEffects)
        .def_readonly("legal_actions", &NumericObservation::legalActions)
        .def_readonly("action_selections", &NumericObservation::actionSelections);

    module.def("build_observation_schema", &BuildObservationSchema);
    py::class_<Game>(module, "Game")
        .def(py::init<>()).def("reset", &Game::Reset).def("apply_decision_index", &Game::ApplyDecisionIndex,
            py::arg("legal_action_index"), py::arg("maximum_initiative_ticks") = 10000)
        .def("observe_decision", &Game::ObserveDecision).def("observe_team_decision", &Game::ObserveTeamDecision)
        .def("observe_numeric", &Game::ObserveNumeric).def("observe_team_numeric", &Game::ObserveTeamNumeric)
        .def("visible_tile_ids", &Game::VisibleTileIds).def("team_visible_tile_ids", &Game::TeamVisibleTileIds)
        .def("is_terminal", &Game::IsTerminal).def("state_hash", &Game::StateHash)
        .def("content_version", [](const Game& game) { return game.Definitions().contentVersion; });
    py::class_<VectorStepResult>(module, "VectorStepResult")
        .def_readonly("results", &VectorStepResult::results).def_readonly("observations", &VectorStepResult::observations);
    py::class_<VectorEnvironment>(module, "VectorEnvironment")
        .def(py::init<>()).def("reset", &VectorEnvironment::Reset).def("step", &VectorEnvironment::Step)
        .def("observe", &VectorEnvironment::Observe).def("observe_numeric", &VectorEnvironment::ObserveNumeric)
        .def("observe_team_numeric", &VectorEnvironment::ObserveTeamNumeric)
        .def("size", &VectorEnvironment::Size);
}
