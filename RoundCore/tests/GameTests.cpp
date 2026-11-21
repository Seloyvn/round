#include "round/core/Game.h"
#include "round/core/Package.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
using namespace roundcore;

MatchSetup ValidSetup()
{
    MatchSetup setup;
    setup.definitions.contentVersion = "test-content";
    setup.definitions.terrains.emplace("Meadow", TerrainDefinition{"Meadow", true, false, false, 1, 10000, {}});
    setup.definitions.terrains.emplace("Rubble", TerrainDefinition{"Rubble", true, false, false, 1, 15000, {}});
    setup.definitions.terrains.emplace("Mountain", TerrainDefinition{"Mountain", false, true, false, 1, 10000, {}});
    setup.definitions.characters.emplace("Tank", CharacterDefinition{"Tank", StatBlock{100, 10, 5, 100, 5, 1, 10, 20, 10}, {}, {}});

    setup.initialState.matchId = 42;
    setup.initialState.seed = 1234;
    setup.initialState.activeCharacterId = 10;
    setup.initialState.tiles = {
        TileState{1, HexCoord{0, 0}, "Meadow", InvalidEntityId, false},
        TileState{2, HexCoord{1, 0}, "Meadow", InvalidEntityId, false},
        TileState{3, HexCoord{0, 1}, "Rubble", InvalidEntityId, false},
        TileState{4, HexCoord{2, 0}, "Meadow", InvalidEntityId, false},
        TileState{5, HexCoord{-1, 0}, "Mountain", InvalidEntityId, false},
        TileState{6, HexCoord{-2, 0}, "Meadow", InvalidEntityId, false}};
    setup.initialState.characters = {
        CharacterState{10, "Tank", Team::TeamA, 1, 100, 100, true, {}},
        CharacterState{20, "Tank", Team::TeamB, 2, 100, 100, true, {}}};
    return setup;
}

void Require(const bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace roundcore;

    MatchSetup packageSource = ValidSetup();
    AbilityDefinition packageAbility;
    packageAbility.id = "PackageBolt";
    packageAbility.operation = AbilityOperation::DirectDamage;
    packageAbility.targetRelation = AbilityTargetRelation::Enemy;
    packageAbility.initiativeCost = 7;
    packageAbility.range = 3;
    packageAbility.amount = 19;
    packageAbility.defenseIgnore = 4;
    packageAbility.damageType = DamageType::Ice;
    packageAbility.values = {{"BehindDistance", 2.0}, {"DamageMultiplier", 1.5}};
    packageSource.definitions.abilities.emplace(packageAbility.id, packageAbility);
    packageSource.definitions.characters.at("Tank").abilityIds.push_back(packageAbility.id);
    packageSource.initialState.characters[0].abilities.push_back(AbilityState{packageAbility.id, 0, 8, 1, 1, false});
    ModifierEffectState packageModifier;
    packageModifier.id = "PackageModifier";
    packageModifier.causerId = 20;
    packageModifier.remainingInitiative = 25;
    packageModifier.incomingDamageMultiplierBasisPoints = 9000;
    packageModifier.areaCenterTileId = 4;
    packageModifier.areaRadius = 2;
    packageModifier.areaDamage = 17;
    packageModifier.areaDamageType = DamageType::Holy;
    packageModifier.areaInitiativeLossPerPulse = 3;
    packageModifier.areaFollowsCauser = false;
    packageSource.initialState.characters[0].modifierEffects.push_back(packageModifier);
    const std::vector<std::uint8_t> packageBytes = SerializeMatchSetup(packageSource);
    const MatchSetup packageSetup = DeserializeMatchSetup(packageBytes);
    Game packageGame;
    Require(packageGame.Reset(packageSetup).success, "a serialized match package should load into Core");
    Game packageReference;
    Require(packageReference.Reset(packageSource).success && packageGame.StateHash() == packageReference.StateHash(),
        "a match package round trip must preserve deterministic initial state");
    const AbilityDefinition& loadedPackageAbility = packageGame.Definitions().abilities.at("PackageBolt");
    Require(loadedPackageAbility.operation == AbilityOperation::DirectDamage
            && loadedPackageAbility.initiativeCost == 7 && loadedPackageAbility.amount == 19
            && loadedPackageAbility.damageType == DamageType::Ice
            && loadedPackageAbility.values.at("BehindDistance") == 2.0
            && loadedPackageAbility.values.at("DamageMultiplier") == 1.5,
        "a match package must preserve executable ability contracts");
    std::vector<std::uint8_t> invalidPackage = packageBytes;
    invalidPackage[0] = 0;
    bool rejectedInvalidPackage = false;
    try { static_cast<void>(DeserializeMatchSetup(invalidPackage)); }
    catch (const std::invalid_argument&) { rejectedInvalidPackage = true; }
    Require(rejectedInvalidPackage, "invalid match package headers must be rejected");

    Game first;
    const ActionResult firstReset = first.Reset(ValidSetup());
    Require(firstReset.success, "valid match setup should reset");
    Require(first.State().tiles[0].occupantId == 10, "reset should derive first tile occupancy");
    Require(first.State().tiles[1].occupantId == 20, "reset should derive second tile occupancy");
    Require(firstReset.events.size() == 1 && firstReset.events[0].type == EventType::MatchReset,
        "reset should emit a match reset event");

    Game second;
    Require(second.Reset(ValidSetup()).success, "same setup should reset a second game");
    Require(first.StateHash() == second.StateHash(), "same setup and seed should produce the same state hash");
    Game randomFirst = first.Clone();
    Game randomSecond = second.Clone();
    Require(randomFirst.RollBasisPoints(10000) && randomSecond.RollBasisPoints(10000),
        "a guaranteed critical roll should succeed through Core");
    Require(randomFirst.State().randomState == randomSecond.State().randomState,
        "identical Core critical rolls must advance the deterministic stream identically");

    const std::vector<EntityId> visibleTiles = first.VisibleTileIds(10);
    Require(std::find(visibleTiles.begin(), visibleTiles.end(), 5) != visibleTiles.end(),
        "vision-blocking terrain should remain visible itself");
    Require(std::find(visibleTiles.begin(), visibleTiles.end(), 6) == visibleTiles.end(),
        "vision-blocking terrain should hide tiles behind it");
    Require(first.TeamVisibleTileIds(Team::TeamA) == visibleTiles,
        "team visibility should combine its living characters in stable tile order");

    MatchSetup hiddenSetup = ValidSetup();
    hiddenSetup.definitions.characters.at("Tank").stats.attackRange = 5;
    hiddenSetup.initialState.characters[1].tileId = 6;
    ModifierEffectState hiddenCauserModifier;
    hiddenCauserModifier.id = "HiddenCauser";
    hiddenCauserModifier.causerId = 20;
    hiddenCauserModifier.remainingInitiative = 10;
    hiddenSetup.initialState.characters[0].modifierEffects.push_back(hiddenCauserModifier);
    Game hidden;
    Require(hidden.Reset(std::move(hiddenSetup)).success, "hidden-information setup should reset");
    const DecisionSnapshot teamObservation = hidden.ObserveTeamDecision(Team::TeamA);
    Require(teamObservation.state.seed == 0 && teamObservation.state.randomState == 0
            && teamObservation.state.matchId == 0,
        "team observations must not expose deterministic random prediction inputs");
    Require(std::none_of(teamObservation.state.tiles.begin(), teamObservation.state.tiles.end(),
            [](const TileState& tile) { return tile.id == 6; }),
        "team observations must omit hidden tiles");
    Require(std::none_of(teamObservation.state.characters.begin(), teamObservation.state.characters.end(),
            [](const CharacterState& character) { return character.id == 20; }),
        "team observations must omit enemies behind vision blockers");
    Require(teamObservation.state.characters[0].modifierEffects[0].causerId == InvalidEntityId,
        "visible effects must not reveal a hidden causer through team observations");
    GameAction hiddenAttack;
    hiddenAttack.type = ActionType::BasicAttack;
    hiddenAttack.actorId = 10;
    hiddenAttack.targetEntityId = 20;
    const ActionResult hiddenAttackResult = hidden.ValidateAction(hiddenAttack);
    Require(!hiddenAttackResult.success && hiddenAttackResult.errorCode == "target_not_visible",
        "hidden enemies must not be legal basic-attack targets even when they are in numeric range");
    const ObservationSchema hiddenSchema = BuildObservationSchema(hidden.Definitions());
    const NumericObservation hiddenNumeric = hidden.ObserveTeamNumeric(Team::TeamA, hiddenSchema);
    Require(hiddenNumeric.tiles.size() / NumericObservation::TileStride == teamObservation.state.tiles.size()
            && hiddenNumeric.characters.size() / NumericObservation::CharacterStride
                == teamObservation.state.characters.size(),
        "numeric team observations must retain the same visibility filtering");

    const DecisionSnapshot initialObservation = first.ObserveDecision();
    Require(initialObservation.state.activeCharacterId == 10 && initialObservation.legalActions.size() == 3,
        "decision observations should snapshot state together with its legal action set");
    const ActionResult invalidDecisionIndex = first.ApplyDecisionIndex(initialObservation.legalActions.size());
    Require(!invalidDecisionIndex.success
            && invalidDecisionIndex.errorCode == "legal_action_index_out_of_range",
        "decision action indices outside the current legal set must be rejected");

    Game cloned = first.Clone();
    Require(cloned.StateHash() == first.StateHash(), "a cloned game should initially be identical");
    Require(cloned.ApplyDecisionIndex(0).success, "a cloned game should accept its own legal action indices");
    Require(cloned.StateHash() != first.StateHash(), "advancing a clone must not mutate its source game");

    const ObservationSchema observationSchema = BuildObservationSchema(first.Definitions());
    const NumericObservation numericObservation = first.ObserveNumeric(observationSchema);
    Require(numericObservation.contentVersion == "test-content"
            && numericObservation.global.size() == NumericObservation::GlobalStride,
        "numeric observations should retain their versioned content contract");
    Require(numericObservation.tiles.size() == first.State().tiles.size() * NumericObservation::TileStride
            && numericObservation.characters.size()
                == first.State().characters.size() * NumericObservation::CharacterStride,
        "numeric observations should emit fixed-stride tile and character records");
    Require(numericObservation.legalActions.size()
            == first.LegalActions(first.State().activeCharacterId).size() * NumericObservation::ActionStride,
        "numeric observations should encode the complete current legal action set");

    const std::vector<GameAction> legalMoves = first.LegalActions(10);
    Require(legalMoves.size() == 3,
        "legal actions should include movement, basic attack, and end turn choices");
    Require(std::any_of(legalMoves.begin(), legalMoves.end(), [](const GameAction& action)
        { return action.type == ActionType::Move && action.targetTileId == 3; }),
        "legal movement choices should contain only affordable, adjacent, unoccupied, walkable tiles");

    GameAction move;
    move.type = ActionType::Move;
    move.actorId = 10;
    move.targetTileId = 3;
    Require(first.ValidateAction(move).success, "legal movement should validate without executing");
    Require(first.State().characters[0].tileId == 1 && first.State().step == 0,
        "validation must not mutate match state");
    const ActionResult moveResult = first.ApplyAction(move);
    Require(moveResult.success, "a legal adjacent move should execute");
    Require(first.State().characters[0].tileId == 3, "movement should update the character tile");
    Require(first.State().characters[0].initiative == 85, "rubble should apply its fixed-point movement multiplier");
    Require(first.State().tiles[0].occupantId == InvalidEntityId && first.State().tiles[2].occupantId == 10,
        "movement should atomically update origin and destination occupancy");
    Require(moveResult.events.size() == 2
            && moveResult.events[0].type == EventType::CharacterMoved
            && moveResult.events[1].type == EventType::InitiativeChanged,
        "movement should emit presentation-ready movement and initiative events");

    GameAction nonAdjacent = move;
    nonAdjacent.targetTileId = 4;
    const ActionResult nonAdjacentResult = second.ApplyAction(nonAdjacent);
    Require(!nonAdjacentResult.success && nonAdjacentResult.errorCode == "destination_not_adjacent",
        "non-adjacent move actions must be rejected");

    GameAction occupied = move;
    occupied.targetTileId = 2;
    const ActionResult occupiedResult = second.ApplyAction(occupied);
    Require(!occupiedResult.success && occupiedResult.errorCode == "destination_occupied",
        "occupied destinations must be rejected");

    GameAction mountain = move;
    mountain.targetTileId = 5;
    const ActionResult mountainResult = second.ApplyAction(mountain);
    Require(!mountainResult.success && mountainResult.errorCode == "destination_blocked",
        "unwalkable terrain must be rejected");

    MatchSetup poorSetup = ValidSetup();
    poorSetup.initialState.characters[0].initiative = 14;
    Game poor;
    Require(poor.Reset(std::move(poorSetup)).success, "low-initiative setup should still reset");
    const ActionResult unaffordable = poor.ApplyAction(move);
    Require(!unaffordable.success && unaffordable.errorCode == "insufficient_initiative",
        "unaffordable movement must be rejected without changing state");

    MatchSetup discountedMoveSetup = ValidSetup();
    discountedMoveSetup.initialState.characters[1].tileId = 4;
    ModifierEffectState moveDiscount;
    moveDiscount.id = "MoveDiscount";
    moveDiscount.remainingInitiative = 10;
    moveDiscount.moveCostAdd = -3;
    discountedMoveSetup.initialState.characters[0].modifierEffects.push_back(moveDiscount);
    Game discountedMove;
    Require(discountedMove.Reset(std::move(discountedMoveSetup)).success,
        "movement modifier setup should reset");
    GameAction discountedMoveAction;
    discountedMoveAction.type = ActionType::Move;
    discountedMoveAction.actorId = 10;
    discountedMoveAction.targetTileId = 2;
    Require(discountedMove.ApplyAction(discountedMoveAction).success
            && discountedMove.State().characters[0].initiative == 93,
        "move-cost modifiers should alter the authoritative Core movement cost");

    MatchSetup blockedDiscountSetup = ValidSetup();
    blockedDiscountSetup.initialState.characters[1].tileId = 4;
    moveDiscount.blockMovementBonuses = true;
    blockedDiscountSetup.initialState.characters[0].modifierEffects.push_back(moveDiscount);
    Game blockedDiscount;
    Require(blockedDiscount.Reset(std::move(blockedDiscountSetup)).success,
        "blocked movement bonus setup should reset");
    Require(blockedDiscount.ApplyAction(discountedMoveAction).success
            && blockedDiscount.State().characters[0].initiative == 90,
        "blocking movement bonuses should clamp discounted cost to the character base cost");

    MatchSetup slowMoveSetup = ValidSetup();
    slowMoveSetup.initialState.characters[1].tileId = 4;
    ModifierEffectState slowMoveEffect;
    slowMoveEffect.id = "Slow";
    slowMoveEffect.remainingInitiative = 20;
    slowMoveEffect.moveCostMultiplierBasisPoints = 15000;
    slowMoveEffect.initiativeLossOnMove = 4;
    slowMoveSetup.initialState.characters[0].modifierEffects.push_back(slowMoveEffect);
    Game slowMove;
    Require(slowMove.Reset(std::move(slowMoveSetup)).success, "slow movement setup should reset");
    const ActionResult slowMoveResult = slowMove.ApplyAction(discountedMoveAction);
    Require(slowMoveResult.success && slowMove.State().characters[0].initiative == 81,
        "slow should multiply movement cost and apply its per-field initiative loss in Core");
    Require(slowMoveResult.events.size() == 3
            && slowMoveResult.events[0].type == EventType::CharacterMoved
            && slowMoveResult.events[0].amount == 15
            && slowMoveResult.events[2].amount == -4,
        "slow movement should expose base movement cost and additional delay as separate events");

    MatchSetup icebeamMoveSetup = ValidSetup();
    icebeamMoveSetup.initialState.characters[1].tileId = 4;
    ModifierEffectState icebeamSlow;
    icebeamSlow.id = "IcebeamSlow";
    icebeamSlow.remainingInitiative = 20;
    icebeamSlow.firstMovePenaltyAvailable = true;
    icebeamSlow.firstMoveInitiativeLoss = 6;
    icebeamSlow.maximumMovedFields = 2;
    icebeamMoveSetup.initialState.characters[0].modifierEffects.push_back(icebeamSlow);
    Game icebeamMove;
    Require(icebeamMove.Reset(std::move(icebeamMoveSetup)).success,
        "Icebeam movement-effect setup should reset");
    Require(icebeamMove.ApplyAction(discountedMoveAction).success
            && icebeamMove.State().characters[0].initiative == 84
            && icebeamMove.State().characters[0].modifierEffects.size() == 1
            && !icebeamMove.State().characters[0].modifierEffects[0].firstMovePenaltyAvailable
            && icebeamMove.State().characters[0].modifierEffects[0].movedFields == 1,
        "Icebeam should consume its first-move penalty and track the first moved field in Core");
    GameAction icebeamReturnMove = discountedMoveAction;
    icebeamReturnMove.targetTileId = 1;
    const ActionResult icebeamReturnResult = icebeamMove.ApplyAction(icebeamReturnMove);
    Require(icebeamReturnResult.success && icebeamMove.State().characters[0].initiative == 74
            && icebeamMove.State().characters[0].modifierEffects.empty(),
        "Icebeam slow should remove itself after its configured number of moved fields");
    Require(std::any_of(icebeamReturnResult.events.begin(), icebeamReturnResult.events.end(),
            [](const GameEvent& event) { return event.type == EventType::EffectRemoved && event.definitionId == "IcebeamSlow"; }),
        "Icebeam movement expiry should emit a deterministic removal event");

    MatchSetup turnSetup = ValidSetup();
    AbilityDefinition testAbility; testAbility.id = "TestAbility";
    AbilityDefinition chargeAbility; chargeAbility.id = "ChargeAbility";
    turnSetup.definitions.abilities.emplace(testAbility.id, testAbility);
    turnSetup.definitions.abilities.emplace(chargeAbility.id, chargeAbility);
    turnSetup.definitions.characters.at("Tank").abilityIds = {"TestAbility", "ChargeAbility"};
    turnSetup.initialState.activeCharacterId = 10;
    turnSetup.initialState.characters[0].initiative = 20;
    turnSetup.initialState.characters[1].initiative = 95;
    turnSetup.initialState.characters[1].abilities.push_back(AbilityState{"TestAbility", 3, 5, 1, 1, false});
    turnSetup.initialState.characters[1].abilities.push_back(AbilityState{"ChargeAbility", 1, 5, 0, 2, true});
    ModifierEffectState turnEffect;
    turnEffect.id = "TurnEffect"; turnEffect.remainingTurns = 1; turnEffect.expireOnTurnEnd = true;
    turnSetup.initialState.characters[0].modifierEffects.push_back(turnEffect);
    ModifierEffectState initiativeEffect;
    initiativeEffect.id = "InitiativeEffect"; initiativeEffect.remainingInitiative = 1;
    turnSetup.initialState.characters[1].modifierEffects.push_back(initiativeEffect);
    Game turns;
    Require(turns.Reset(std::move(turnSetup)).success, "turn test setup should reset");

    GameAction endTurn;
    endTurn.type = ActionType::EndTurn;
    endTurn.actorId = 10;
    const ActionResult tickResult = turns.ApplyAction(endTurn);
    Require(tickResult.success && turns.State().activeCharacterId == InvalidEntityId,
        "ending a turn should advance exactly one global initiative tick when nobody is ready");
    Require(turns.State().globalInitiative == 1 && turns.State().characters[1].initiative == 96,
        "global initiative should advance deterministically by exported per-character gains");
    Require(turns.State().characters[1].abilities[0].cooldownRemaining == 2,
        "global initiative should reduce ability cooldowns in Core");
    Require(turns.State().characters[1].abilities[1].currentCharges == 1
            && turns.State().characters[1].abilities[1].cooldownRemaining == 5,
        "charge abilities should restore a charge and restart cooldown until fully charged");
    Require(turns.State().characters[0].modifierEffects.empty()
            && turns.State().characters[1].modifierEffects.empty(),
        "modifier effects should expire on their configured turn or initiative boundary");

    endTurn.actorId = InvalidEntityId;
    for (int index = 0; index < 5; ++index)
    {
        Require(turns.ApplyAction(endTurn).success, "turn selection continuation should succeed without an active actor");
    }
    Require(turns.State().activeCharacterId == 20, "the first ready character should become active");
    Require(turns.State().characters[1].initiative == 90,
        "selecting a character should apply its configured turn delay in Core");

    MatchSetup combatSetup = ValidSetup();
    combatSetup.initialState.characters[1].health = 10;
    Game combat;
    Require(combat.Reset(std::move(combatSetup)).success, "combat setup should reset");
    GameAction attack;
    attack.type = ActionType::BasicAttack;
    attack.actorId = 10;
    attack.targetEntityId = 20;
    const ActionResult attackResult = combat.ApplyAction(attack);
    Require(attackResult.success, "an affordable adjacent attack against an enemy should execute");
    Require(!combat.State().characters[1].alive && combat.State().characters[1].health == 0,
        "lethal basic attack damage should kill its target");
    Require(combat.State().tiles[1].occupantId == InvalidEntityId,
        "death should release the target tile in Core state");
    Require(combat.IsTerminal() && combat.State().winner == Team::TeamA,
        "eliminating the last opposing team should complete the match");

    MatchSetup auraSetup = ValidSetup();
    auraSetup.definitions.characters.at("Tank").stats.attack = 150;
    auraSetup.definitions.characters.at("Tank").stats.defense = 35;
    auraSetup.definitions.characters.at("Tank").stats.maxHealth = 500;
    CharacterDefinition protectorDefinition = auraSetup.definitions.characters.at("Tank");
    protectorDefinition.id = "protector";
    protectorDefinition.passiveValues["CoreIncomingDamageAuraReduction"] = 0.20;
    protectorDefinition.passiveValues["CoreIncomingDamageAuraRadius"] = 2.0;
    protectorDefinition.passiveValues["CoreIncomingDamageAuraAffectSelf"] = 0.0;
    auraSetup.definitions.characters.emplace(protectorDefinition.id, protectorDefinition);
    CharacterState protector = auraSetup.initialState.characters[1];
    protector.id = 30;
    protector.definitionId = protectorDefinition.id;
    protector.tileId = 3;
    auraSetup.initialState.characters[1].health = 500;
    auraSetup.initialState.tiles[2].occupantId = protector.id;
    auraSetup.initialState.characters.push_back(protector);
    Game auraCombat;
    Require(auraCombat.Reset(std::move(auraSetup)).success, "aura combat setup should reset");
    const ActionResult auraAttack = auraCombat.ApplyAction(attack);
    Require(auraAttack.success, "an attack against an aura-protected target should execute");
    Require(auraCombat.State().characters[1].health == 411,
        "the exported incoming-damage aura should reduce damage before defense mitigation");

    MatchSetup modifierSetup = ValidSetup();
    modifierSetup.definitions.characters.at("Tank").stats.attack = 20;
    modifierSetup.definitions.characters.at("Tank").stats.defense = 0;
    modifierSetup.initialState.characters[0].health = 50;
    ModifierEffectState outgoingModifier;
    outgoingModifier.id = "OutgoingModifier";
    outgoingModifier.remainingInitiative = 10;
    outgoingModifier.outgoingDamageMultiplierBasisPoints = 15000;
    outgoingModifier.healFractionFromDamageDealtBasisPoints = 1000;
    modifierSetup.initialState.characters[0].modifierEffects.push_back(outgoingModifier);
    ModifierEffectState incomingModifier;
    incomingModifier.id = "IncomingModifier";
    incomingModifier.causerId = 10;
    incomingModifier.remainingInitiative = 10;
    incomingModifier.incomingDamageMultiplierBasisPoints = 5000;
    incomingModifier.onlyAffectCauserDamage = true;
    modifierSetup.initialState.characters[1].modifierEffects.push_back(incomingModifier);
    Game modifierCombat;
    Require(modifierCombat.Reset(std::move(modifierSetup)).success, "modifier combat setup should reset");
    const ActionResult modifierAttack = modifierCombat.ApplyAction(attack);
    Require(modifierAttack.success, "an attack with shared modifier effects should execute");
    Require(modifierCombat.State().characters[0].health == 53,
        "outgoing modifier lifesteal should heal from modified pre-mitigation damage");
    Require(modifierCombat.State().characters[1].health == 85,
        "outgoing and incoming modifier multipliers should resolve in Actor order");

    MatchSetup rootDamageSetup = ValidSetup();
    rootDamageSetup.definitions.characters.at("Tank").stats.attack = 20;
    rootDamageSetup.definitions.characters.at("Tank").stats.defense = 0;
    ModifierEffectState damageRoot;
    damageRoot.id = "Root";
    damageRoot.causerId = 10;
    damageRoot.remainingInitiative = 10;
    damageRoot.incomingDamageMultiplierFromCauserBasisPoints = 15000;
    rootDamageSetup.initialState.characters[1].modifierEffects.push_back(damageRoot);
    Game rootDamage;
    Require(rootDamage.Reset(std::move(rootDamageSetup)).success, "root damage setup should reset");
    Require(rootDamage.ApplyAction(attack).success && rootDamage.State().characters[1].health == 70,
        "root should amplify incoming damage only from its exported causer");

    MatchSetup rootedMoveSetup = ValidSetup();
    rootedMoveSetup.initialState.characters[1].tileId = 4;
    ModifierEffectState movementRoot;
    movementRoot.id = "Root";
    movementRoot.remainingInitiative = 10;
    movementRoot.preventsMovement = true;
    rootedMoveSetup.initialState.characters[0].modifierEffects.push_back(movementRoot);
    Game rootedMove;
    Require(rootedMove.Reset(std::move(rootedMoveSetup)).success, "rooted movement setup should reset");
    GameAction rootedMoveAction;
    rootedMoveAction.type = ActionType::Move;
    rootedMoveAction.actorId = 10;
    rootedMoveAction.targetTileId = 2;
    const ActionResult rootedMoveResult = rootedMove.ValidateAction(rootedMoveAction);
    Require(!rootedMoveResult.success && rootedMoveResult.errorCode == "movement_blocked",
        "root should remove movement from the authoritative legal-action set");

    MatchSetup rootExpirySetup = ValidSetup();
    rootExpirySetup.initialState.activeCharacterId = 10;
    rootExpirySetup.initialState.characters[0].initiative = 20;
    rootExpirySetup.initialState.characters[1].initiative = 20;
    ModifierEffectState expiringRoot;
    expiringRoot.id = "Root";
    expiringRoot.remainingInitiative = 1;
    expiringRoot.initiativeLossOnExpire = 7;
    rootExpirySetup.initialState.characters[1].modifierEffects.push_back(expiringRoot);
    Game rootExpiry;
    Require(rootExpiry.Reset(std::move(rootExpirySetup)).success, "root expiry setup should reset");
    GameAction rootEndTurn;
    rootEndTurn.type = ActionType::EndTurn;
    rootEndTurn.actorId = 10;
    Require(rootExpiry.ApplyAction(rootEndTurn).success
            && rootExpiry.State().characters[1].initiative == 14
            && rootExpiry.State().characters[1].modifierEffects.empty(),
        "an expiring root should apply its initiative loss exactly once in Core");

    MatchSetup basicAttackEffectSetup = ValidSetup();
    basicAttackEffectSetup.definitions.characters.at("Tank").stats.attack = 20;
    basicAttackEffectSetup.definitions.characters.at("Tank").stats.defense = 0;
    ModifierEffectState attackTax;
    attackTax.id = "AttackTax";
    attackTax.remainingInitiative = 50;
    attackTax.basicAttackCostAdd = 5;
    attackTax.basicAttackCostAvailable = true;
    basicAttackEffectSetup.initialState.characters[0].modifierEffects.push_back(attackTax);
    ModifierEffectState oneHitReduction;
    oneHitReduction.id = "OneHitReduction";
    oneHitReduction.remainingInitiative = 50;
    oneHitReduction.basicAttackDamageMultiplierBasisPoints = 5000;
    oneHitReduction.consumeOnBasicAttack = true;
    basicAttackEffectSetup.initialState.characters[0].modifierEffects.push_back(oneHitReduction);
    Game basicAttackEffects;
    Require(basicAttackEffects.Reset(std::move(basicAttackEffectSetup)).success,
        "basic-attack effect setup should reset");
    Require(basicAttackEffects.ApplyAction(attack).success,
        "a taxed and damage-modified basic attack should execute");
    Require(basicAttackEffects.State().characters[0].initiative == 75
            && basicAttackEffects.State().characters[1].health == 90,
        "basic-attack cost and one-hit damage effects should affect the first attack");
    Require(basicAttackEffects.State().characters[0].modifierEffects.size() == 1
            && !basicAttackEffects.State().characters[0].modifierEffects[0].basicAttackCostAvailable,
        "one-hit effects should be consumed and one-use attack taxes should become unavailable");
    Require(basicAttackEffects.ApplyAction(attack).success
            && basicAttackEffects.State().characters[0].initiative == 55
            && basicAttackEffects.State().characters[1].health == 70,
        "subsequent attacks should use base cost and damage after one-use effects resolve");

    MatchSetup abilitySetup = ValidSetup();
    AbilityDefinition bolt;
    bolt.id = "DirectBolt";
    bolt.operation = AbilityOperation::DirectDamage;
    bolt.targetRelation = AbilityTargetRelation::Enemy;
    bolt.initiativeCost = 7;
    bolt.range = 2;
    bolt.amount = 21;
    bolt.defenseIgnore = 5;
    abilitySetup.definitions.abilities.emplace(bolt.id, bolt);
    abilitySetup.definitions.characters.at("Tank").abilityIds = {bolt.id};
    abilitySetup.initialState.characters[0].abilities.push_back(AbilityState{bolt.id, 0, 9, 1, 1, false});
    MatchSetup silencedSetup = abilitySetup;
    ModifierEffectState silence;
    silence.id = "Silence";
    silence.remainingInitiative = 50;
    silence.preventsAbilityUse = true;
    silencedSetup.initialState.characters[0].modifierEffects.push_back(silence);
    Game silenced;
    Require(silenced.Reset(std::move(silencedSetup)).success, "silenced ability setup should reset");
    GameAction blockedBolt;
    blockedBolt.type = ActionType::UseAbility;
    blockedBolt.actorId = 10;
    blockedBolt.targetEntityId = 20;
    blockedBolt.targetTileId = 2;
    blockedBolt.abilityId = bolt.id;
    const ActionResult blockedBoltResult = silenced.ValidateAction(blockedBolt);
    Require(!blockedBoltResult.success && blockedBoltResult.errorCode == "ability_use_blocked",
        "silence should reject otherwise legal ability commands in Core");
    const std::vector<GameAction> silencedActions = silenced.LegalActions(10);
    Require(std::none_of(silencedActions.begin(), silencedActions.end(),
            [](const GameAction& action) { return action.type == ActionType::UseAbility; }),
        "silence should remove abilities from the legal-action mask");
    MatchSetup modifiedAbilitySetup = abilitySetup;
    modifiedAbilitySetup.initialState.characters[0].health = 50;
    ModifierEffectState abilityOutgoing;
    abilityOutgoing.id = "AbilityOutgoing";
    abilityOutgoing.remainingInitiative = 20;
    abilityOutgoing.outgoingDamageMultiplierBasisPoints = 15000;
    abilityOutgoing.healFractionFromDamageDealtBasisPoints = 1000;
    modifiedAbilitySetup.initialState.characters[0].modifierEffects.push_back(abilityOutgoing);
    ModifierEffectState abilityIncoming;
    abilityIncoming.id = "AbilityIncoming";
    abilityIncoming.remainingInitiative = 20;
    abilityIncoming.incomingDamageMultiplierBasisPoints = 5000;
    modifiedAbilitySetup.initialState.characters[1].modifierEffects.push_back(abilityIncoming);
    Game modifiedAbility;
    Require(modifiedAbility.Reset(std::move(modifiedAbilitySetup)).success,
        "modified direct-ability setup should reset");
    Require(modifiedAbility.ApplyAction(blockedBolt).success
            && modifiedAbility.State().characters[0].health == 53
            && modifiedAbility.State().characters[1].health == 84,
        "direct abilities should use the shared outgoing, incoming, lifesteal, and defense damage pipeline");
    MatchSetup frostbreakSetup = abilitySetup;
    frostbreakSetup.definitions.abilities.at(bolt.id).damageType = DamageType::Ice;
    ModifierEffectState frostbreak;
    frostbreak.id = "IcebeamSlow";
    frostbreak.causerId = 10;
    frostbreak.remainingInitiative = 20;
    frostbreak.frostbreakDamage = 10;
    frostbreakSetup.initialState.characters[1].modifierEffects.push_back(frostbreak);
    Game frostbreakGame;
    Require(frostbreakGame.Reset(std::move(frostbreakSetup)).success, "Frostbreak setup should reset");
    const ActionResult frostbreakResult = frostbreakGame.ApplyAction(blockedBolt);
    Require(frostbreakResult.success && frostbreakGame.State().characters[1].health == 69
            && frostbreakGame.State().characters[1].modifierEffects.empty(),
        "non-physical causer damage should consume Icebeam slow and apply Frostbreak through shared damage");
    Require(std::any_of(frostbreakResult.events.begin(), frostbreakResult.events.end(),
            [](const GameEvent& event)
            {
                return event.type == EventType::DamageApplied
                    && event.definitionId == "IcebeamFrostbreak" && event.amount == 10;
            }),
        "Frostbreak should emit its own deterministic damage event");
    MatchSetup physicalFrostbreakSetup = abilitySetup;
    physicalFrostbreakSetup.initialState.characters[1].modifierEffects.push_back(frostbreak);
    Game physicalFrostbreak;
    Require(physicalFrostbreak.Reset(std::move(physicalFrostbreakSetup)).success,
        "physical Frostbreak control setup should reset");
    Require(physicalFrostbreak.ApplyAction(blockedBolt).success
            && physicalFrostbreak.State().characters[1].health == 79
            && physicalFrostbreak.State().characters[1].modifierEffects.size() == 1,
        "physical damage should not consume or trigger Frostbreak");
    MatchSetup followupAbilitySetup = abilitySetup;
    AbilityDefinition& followupBolt = followupAbilitySetup.definitions.abilities.at(bolt.id);
    followupBolt.values["ImmediateInitiativeLoss"] = 16.0;
    followupBolt.values["InitiativeGainOnHit"] = 12.0;
    followupBolt.values["SelfHealOnHit"] = 30.0;
    followupAbilitySetup.initialState.characters[0].health = 50;
    Game followupAbility;
    Require(followupAbility.Reset(std::move(followupAbilitySetup)).success,
        "direct ability follow-up setup should reset");
    const ActionResult followupResult = followupAbility.ApplyAction(blockedBolt);
    Require(followupResult.success
            && followupAbility.State().characters[0].health == 80
            && followupAbility.State().characters[0].initiative == 105
            && followupAbility.State().characters[1].initiative == 84,
        "data-driven direct-ability follow-ups should heal and adjust both characters' initiative");
    Require(std::count_if(followupResult.events.begin(), followupResult.events.end(),
            [](const GameEvent& event) { return event.type == EventType::InitiativeChanged; }) == 3
            && std::any_of(followupResult.events.begin(), followupResult.events.end(),
                [](const GameEvent& event) { return event.type == EventType::HealingApplied && event.amount == 30; }),
        "direct-ability follow-ups should emit deterministic initiative and healing events");
    MatchSetup condemnSetup = abilitySetup;
    AbilityDefinition& condemn = condemnSetup.definitions.abilities.at(bolt.id);
    condemn.values["RecentAllyDamageWindowInitiative"] = 25.0;
    condemn.values["SilenceDurationInitiative"] = 40.0;
    condemn.values["SelfHealOnCondemn"] = 20.0;
    condemn.values["InitiativeGainOnCondemn"] = 5.0;
    condemnSetup.initialState.characters[0].health = 50;
    condemnSetup.initialState.characters[0].recentAuraPreventions.push_back({20, 0});
    Game condemnGame;
    Require(condemnGame.Reset(std::move(condemnSetup)).success, "Condemn setup should reset");
    const ActionResult condemnResult = condemnGame.ApplyAction(blockedBolt);
    Require(condemnResult.success && condemnGame.State().characters[0].health == 70
            && condemnGame.State().characters[0].initiative == 98
            && condemnGame.State().characters[1].modifierEffects.size() == 1
            && condemnGame.State().characters[1].modifierEffects[0].preventsAbilityUse
            && condemnGame.State().characters[1].modifierEffects[0].remainingInitiative == 40,
        "Condemn should require aura history and resolve damage, rewards, and silence in Core");
    MatchSetup expiredCondemnSetup = abilitySetup;
    expiredCondemnSetup.definitions.abilities.at(bolt.id).values["RecentAllyDamageWindowInitiative"] = 25.0;
    expiredCondemnSetup.initialState.globalInitiative = 30;
    expiredCondemnSetup.initialState.characters[0].recentAuraPreventions.push_back({20, 0});
    Game expiredCondemn;
    Require(expiredCondemn.Reset(std::move(expiredCondemnSetup)).success,
        "expired Condemn setup should reset");
    const ActionResult expiredCondemnResult = expiredCondemn.ValidateAction(blockedBolt);
    Require(!expiredCondemnResult.success && expiredCondemnResult.errorCode == "missing_recent_aura_prevention",
        "Condemn should reject targets outside the editor-authored aura prevention window");
    Game abilities;
    Require(abilities.Reset(std::move(abilitySetup)).success, "direct ability setup should reset");
    const std::vector<GameAction> abilityActions = abilities.LegalActions(10);
    const auto legalBolt = std::find_if(abilityActions.begin(), abilityActions.end(), [](const GameAction& action)
    {
        return action.type == ActionType::UseAbility && action.abilityId == "DirectBolt" && action.targetEntityId == 20;
    });
    Require(legalBolt != abilityActions.end(), "a supported exported ability should appear in legal actions");
    const ActionResult boltResult = abilities.ApplyAction(*legalBolt);
    Require(boltResult.success && abilities.State().characters[0].initiative == 93,
        "ability execution should pay its exported initiative cost");
    Require(abilities.State().characters[0].abilities[0].cooldownRemaining == 9,
        "ability execution should start its exported cooldown");
    Require(abilities.State().characters[1].health == 79,
        "direct damage should use the exported amount and defense ignore");

    MatchSetup selfModifierSetup = ValidSetup();
    AbilityDefinition bulwark;
    bulwark.id = "Bulwark";
    bulwark.operation = AbilityOperation::SelfModifier;
    bulwark.targetRelation = AbilityTargetRelation::Self;
    bulwark.initiativeCost = 8;
    bulwark.range = 0;
    bulwark.values["DurationInitiative"] = 75.0;
    bulwark.values["SelfIncomingDamageMultiplier"] = 0.7;
    bulwark.values["OutgoingDamageMultiplier"] = 1.25;
    bulwark.values["DefenseIgnore"] = 30.0;
    bulwark.values["HealFractionFromDamageDealt"] = 0.15;
    bulwark.values["MoveCostAdd"] = -3.0;
    selfModifierSetup.definitions.abilities.emplace(bulwark.id, bulwark);
    selfModifierSetup.definitions.characters.at("Tank").abilityIds = {bulwark.id};
    selfModifierSetup.initialState.characters[0].abilities.push_back(
        AbilityState{bulwark.id, 0, 40, 1, 1, false});
    Game selfModifierGame;
    Require(selfModifierGame.Reset(std::move(selfModifierSetup)).success,
        "self modifier ability setup should reset");
    GameAction useBulwark;
    useBulwark.type = ActionType::UseAbility;
    useBulwark.actorId = 10;
    useBulwark.targetEntityId = 10;
    useBulwark.targetTileId = 1;
    useBulwark.abilityId = bulwark.id;
    const ActionResult bulwarkResult = selfModifierGame.ApplyAction(useBulwark);
    Require(bulwarkResult.success && selfModifierGame.State().characters[0].initiative == 92
            && selfModifierGame.State().characters[0].abilities[0].cooldownRemaining == 40
            && selfModifierGame.State().characters[0].modifierEffects.size() == 1,
        "self modifier abilities should pay their cost, start cooldown, and add one Core effect");
    const ModifierEffectState& bulwarkEffect = selfModifierGame.State().characters[0].modifierEffects[0];
    Require(bulwarkEffect.remainingInitiative == 75
            && bulwarkEffect.incomingDamageMultiplierBasisPoints == 7000
            && bulwarkEffect.outgoingDamageMultiplierBasisPoints == 12500
            && bulwarkEffect.defenseIgnoreAdd == 30
            && bulwarkEffect.healFractionFromDamageDealtBasisPoints == 1500
            && bulwarkEffect.moveCostAdd == -3,
        "self modifier abilities should preserve all editor-authored modifier values");
    Require(std::any_of(bulwarkResult.events.begin(), bulwarkResult.events.end(),
            [](const GameEvent& event) { return event.type == EventType::EffectAdded; }),
        "self modifier abilities should emit an effect-added event for Unreal presentation");

    MatchSetup redirectSetup = ValidSetup();
    CharacterState oathProtector = redirectSetup.initialState.characters[1];
    oathProtector.id = 30;
    oathProtector.tileId = 3;
    oathProtector.health = 100;
    redirectSetup.initialState.characters.push_back(oathProtector);
    ModifierEffectState oath;
    oath.id = "ShieldOath";
    oath.causerId = 30;
    oath.remainingInitiative = 80;
    oath.redirectPreMitigationBasisPoints = 5000;
    oath.redirectedDamageMultiplierBasisPoints = 8000;
    oath.protectorHealFractionBasisPoints = 2000;
    oath.protectedInitiativePerHit = 6;
    redirectSetup.initialState.characters[1].modifierEffects.push_back(oath);
    Game redirectGame;
    Require(redirectGame.Reset(std::move(redirectSetup)).success, "Shield Oath redirect setup should reset");
    const ActionResult redirectResult = redirectGame.ApplyAction(attack);
    Require(redirectResult.success
            && redirectGame.State().characters[1].health == 95
            && redirectGame.State().characters[1].initiative == 106
            && redirectGame.State().characters[2].health == 97,
        "Shield Oath should redirect pre-mitigation damage, heal the protector, and reward the protected ally");
    Require(std::any_of(redirectResult.events.begin(), redirectResult.events.end(),
            [](const GameEvent& event) { return event.type == EventType::DamageApplied && event.targetId == 30; }),
        "Shield Oath should emit redirected damage for Unreal presentation and training rewards");

    MatchSetup chainSetup = ValidSetup();
    chainSetup.definitions.characters.at("Tank").stats.defense = 0;
    chainSetup.definitions.characters.at("Tank").passiveValues["DamageMultiplierWhileHasChain"] = 0.5;
    chainSetup.definitions.characters.at("Tank").passiveValues["CheatDeathHealPerConsumedChain"] = 20.0;
    chainSetup.definitions.characters.at("Tank").passiveValues["CheatDeathCooldownInitiative"] = 10.0;
    chainSetup.initialState.characters[1].health = 4;
    chainSetup.initialState.characters[1].resourceCurrent = 2;
    chainSetup.initialState.characters[1].resourceMaximum = 2;
    chainSetup.initialState.characters[1].resourceRechargeInterval = 20;
    Game chainGame;
    Require(chainGame.Reset(std::move(chainSetup)).success, "Character 1 chain setup should reset");
    const ActionResult chainAttack = chainGame.ApplyAction(attack);
    Require(chainAttack.success && chainGame.State().characters[1].alive
            && chainGame.State().characters[1].health == 40
            && chainGame.State().characters[1].resourceCurrent == 0
            && chainGame.State().characters[1].specialCooldownRemaining == 10,
        "Character 1 should consume all chains to survive lethal Core damage when configured");
    Require(std::any_of(chainAttack.events.begin(), chainAttack.events.end(),
            [](const GameEvent& event) { return event.type == EventType::ResourceChanged && event.amount == -2; }),
        "Character 1 cheat death should emit its chain consumption");

    MatchSetup rechargeSetup = ValidSetup();
    rechargeSetup.initialState.activeCharacterId = InvalidEntityId;
    rechargeSetup.initialState.characters[0].initiative = 0;
    rechargeSetup.initialState.characters[1].initiative = 0;
    rechargeSetup.initialState.characters[0].resourceCurrent = 0;
    rechargeSetup.initialState.characters[0].resourceMaximum = 2;
    rechargeSetup.initialState.characters[0].resourceRechargeProgress = 1;
    rechargeSetup.initialState.characters[0].resourceRechargeInterval = 2;
    Game rechargeGame;
    Require(rechargeGame.Reset(std::move(rechargeSetup)).success, "Character 1 recharge setup should reset");
    GameAction rechargeTick;
    rechargeTick.type = ActionType::EndTurn;
    const ActionResult rechargeResult = rechargeGame.ApplyAction(rechargeTick);
    Require(rechargeResult.success && rechargeGame.State().characters[0].resourceCurrent == 1
            && rechargeGame.State().characters[0].resourceRechargeProgress == 0,
        "Character 1 chains should recharge on global initiative in Core");

    MatchSetup groundAuraSetup = ValidSetup();
    groundAuraSetup.initialState.activeCharacterId = InvalidEntityId;
    groundAuraSetup.initialState.characters[0].initiative = 0;
    groundAuraSetup.initialState.characters[1].initiative = 0;
    ModifierEffectState sanctifiedGround;
    sanctifiedGround.id = "SanctifyGround:GroundAura";
    sanctifiedGround.causerId = 10;
    sanctifiedGround.remainingInitiative = 2;
    sanctifiedGround.areaCenterTileId = 2;
    sanctifiedGround.areaRadius = 0;
    sanctifiedGround.areaDamage = 20;
    sanctifiedGround.areaDamageType = DamageType::Holy;
    sanctifiedGround.areaInitiativeLossPerPulse = 7;
    groundAuraSetup.initialState.characters[0].modifierEffects.push_back(sanctifiedGround);
    Game groundAuraGame;
    Require(groundAuraGame.Reset(std::move(groundAuraSetup)).success,
        "Sanctify Ground setup should reset");
    GameAction groundAuraTick;
    groundAuraTick.type = ActionType::EndTurn;
    const ActionResult groundAuraResult = groundAuraGame.ApplyAction(groundAuraTick);
    Require(groundAuraResult.success
            && groundAuraGame.State().characters[1].health == 81
            && groundAuraGame.State().characters[1].initiative == 1
            && groundAuraGame.State().characters[0].modifierEffects[0].remainingInitiative == 1,
        "ground auras should pulse once per global initiative, apply delay, and expire by initiative");
    Require(std::any_of(groundAuraResult.events.begin(), groundAuraResult.events.end(),
            [](const GameEvent& event) { return event.type == EventType::DamageApplied && event.targetId == 20; }),
        "ground aura pulses should emit damage events for Unreal and training");

    MatchSetup groundCastSetup = ValidSetup();
    AbilityDefinition groundCast;
    groundCast.id = "SanctifyGround";
    groundCast.operation = AbilityOperation::GroundAura;
    groundCast.targetRelation = AbilityTargetRelation::AnyCharacter;
    groundCast.initiativeCost = 9;
    groundCast.range = 2;
    groundCast.amount = 24;
    groundCast.damageType = DamageType::Holy;
    groundCast.values = {{"DurationInitiative", 60.0}, {"AuraRadius", 2.0},
        {"InitiativeLossPerPulse", 4.0}, {"FollowCaster", 0.0}};
    groundCastSetup.definitions.abilities.emplace(groundCast.id, groundCast);
    groundCastSetup.definitions.characters.at("Tank").abilityIds.push_back(groundCast.id);
    groundCastSetup.initialState.characters[0].abilities.push_back(AbilityState{groundCast.id, 0, 40, 1, 1, false});
    Game groundCastGame;
    Require(groundCastGame.Reset(std::move(groundCastSetup)).success, "ground aura cast setup should reset");
    GameAction groundCastAction;
    groundCastAction.type = ActionType::UseAbility;
    groundCastAction.actorId = 10;
    groundCastAction.targetTileId = 4;
    groundCastAction.abilityId = groundCast.id;
    const std::vector<GameAction> groundCastLegalActions = groundCastGame.LegalActions(10);
    Require(std::any_of(groundCastLegalActions.begin(), groundCastLegalActions.end(),
            [](const GameAction& action) { return action.type == ActionType::UseAbility && action.targetTileId != InvalidEntityId; }),
        "ground aura abilities should expose tile-target actions to training agents");
    const ActionResult groundCastResult = groundCastGame.ApplyAction(groundCastAction);
    const ModifierEffectState& castGround = groundCastGame.State().characters[0].modifierEffects[0];
    Require(groundCastResult.success && groundCastGame.State().characters[0].initiative == 91
            && castGround.areaCenterTileId == 4 && castGround.areaRadius == 2
            && castGround.areaDamage == 24 && castGround.areaDamageType == DamageType::Holy
            && castGround.areaInitiativeLossPerPulse == 4 && castGround.remainingInitiative == 60,
        "ground aura abilities should create their complete editor-authored zone contract in Core");

    const auto character1AbilitySetup = [](const AbilityDefinition& ability)
    {
        MatchSetup setup = ValidSetup();
        setup.definitions.abilities.emplace(ability.id, ability);
        setup.definitions.characters.at("Tank").abilityIds.push_back(ability.id);
        setup.initialState.characters[0].abilities.push_back(AbilityState{ability.id, 0, 20, 1, 1, false});
        setup.initialState.characters[0].resourceCurrent = 2;
        setup.initialState.characters[0].resourceMaximum = 3;
        return setup;
    };

    AbilityDefinition knockback; knockback.id = "Knockback"; knockback.operation = AbilityOperation::Knockback;
    knockback.range = 1; knockback.amount = 10; knockback.values = {{"KnockbackDistance", 1.0}, {"ChainCost", 1.0}};
    Game knockbackGame; Require(knockbackGame.Reset(character1AbilitySetup(knockback)).success, "knockback setup should reset");
    GameAction character1Cast{ActionType::UseAbility, 10, 20, InvalidEntityId, knockback.id};
    const ActionResult knockbackResult = knockbackGame.ApplyAction(character1Cast);
    Require(knockbackResult.success && knockbackGame.State().characters[1].tileId == 4
            && knockbackGame.State().characters[0].resourceCurrent == 1,
        "Character 1 Knockback should spend a chain and move the target through Core");

    AbilityDefinition delay; delay.id = "Delay"; delay.operation = AbilityOperation::DelayStrike;
    delay.range = 1; delay.values = {{"DelayAmount", 25.0}};
    Game delayGame; Require(delayGame.Reset(character1AbilitySetup(delay)).success, "delay setup should reset");
    character1Cast.abilityId = delay.id;
    Require(delayGame.ApplyAction(character1Cast).success && delayGame.State().characters[1].initiative == 75,
        "Character 1 Delay should reduce target initiative through Core");

    AbilityDefinition shackle; shackle.id = "Shackle"; shackle.operation = AbilityOperation::Shackle;
    shackle.range = 1; shackle.values = {{"SlowDuration", 2.0}, {"SlowStrength", 0.5}, {"InitiativeLossOnMove", 7.0}};
    Game shackleGame; Require(shackleGame.Reset(character1AbilitySetup(shackle)).success, "shackle setup should reset");
    character1Cast.abilityId = shackle.id;
    Require(shackleGame.ApplyAction(character1Cast).success
            && shackleGame.State().characters[1].modifierEffects[0].moveCostMultiplierBasisPoints == 15000
            && shackleGame.State().characters[1].modifierEffects[0].initiativeLossOnMove == 7,
        "Character 1 Shackle should create its movement debuff in Core");

    AbilityDefinition bind; bind.id = "Bind"; bind.operation = AbilityOperation::Bind; bind.range = 1;
    bind.amount = 20; bind.values = {{"DurationInitiative", 80.0}, {"DelayAmount", 10.0}};
    MatchSetup bindSetup = character1AbilitySetup(bind);
    bindSetup.initialState.characters[0].initiative = 0;
    Game bindGame; Require(bindGame.Reset(std::move(bindSetup)).success, "bind setup should reset");
    character1Cast.abilityId = bind.id;
    Require(bindGame.ApplyAction(character1Cast).success, "Character 1 Bind should be castable");
    GameAction endBindCasterTurn; endBindCasterTurn.type = ActionType::EndTurn; endBindCasterTurn.actorId = 10;
    Require(bindGame.ApplyDecision(endBindCasterTurn).success && bindGame.State().activeCharacterId == 20,
        "the bound target should receive the next test turn");
    GameAction boundMove; boundMove.type = ActionType::Move; boundMove.actorId = 20; boundMove.targetTileId = 4;
    const ActionResult bindMove = bindGame.ApplyAction(boundMove);
    Require(bindMove.success && bindGame.State().characters[1].health == 81
            && bindGame.State().characters[1].modifierEffects.empty(),
        "Character 1 Bind should trigger damage and consume itself when the target moves");

    AbilityDefinition taunt; taunt.id = "Taunt"; taunt.operation = AbilityOperation::Taunt; taunt.range = 1;
    taunt.values = {{"DurationInitiative", 80.0}, {"ReductionAmount", 0.25}};
    Game tauntGame; Require(tauntGame.Reset(character1AbilitySetup(taunt)).success, "taunt setup should reset");
    character1Cast.abilityId = taunt.id;
    Require(tauntGame.ApplyAction(character1Cast).success
            && tauntGame.State().characters[1].modifierEffects[0].preferredTargetId == 10
            && tauntGame.State().characters[1].modifierEffects[0].outgoingDamageMultiplierAgainstOtherTargetsBasisPoints == 7500,
        "Character 1 Taunt should persist its preferred target and off-target penalty in Core");

    MatchSetup overlapping = ValidSetup();
    overlapping.initialState.characters[1].tileId = 1;
    Game invalid;
    const ActionResult invalidReset = invalid.Reset(std::move(overlapping));
    Require(!invalidReset.success && invalidReset.errorCode == "duplicate_occupancy",
        "duplicate starting occupancy must be rejected");

    MatchSetup missingTerrain = ValidSetup();
    missingTerrain.initialState.tiles[0].terrainId = "Unknown";
    const ActionResult missingTerrainResult = invalid.Reset(std::move(missingTerrain));
    Require(!missingTerrainResult.success && missingTerrainResult.errorCode == "missing_terrain_definition",
        "missing terrain definitions must be rejected");

    std::cout << "RoundCore contract tests passed\n";
    return EXIT_SUCCESS;
}
