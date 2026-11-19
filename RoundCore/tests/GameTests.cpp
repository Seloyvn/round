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
    packageAbility.values = {{"BehindDistance", 2.0}, {"DamageMultiplier", 1.5}};
    packageSource.definitions.abilities.emplace(packageAbility.id, packageAbility);
    packageSource.definitions.characters.at("Tank").abilityIds.push_back(packageAbility.id);
    packageSource.initialState.characters[0].abilities.push_back(AbilityState{packageAbility.id, 0, 8, 1, 1, false});
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
