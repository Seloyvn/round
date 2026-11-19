#include "round/core/Game.h"

#include <cstdlib>
#include <iostream>
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
        TileState{5, HexCoord{-1, 0}, "Mountain", InvalidEntityId, false}};
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

    const std::vector<GameAction> legalMoves = first.LegalActions(10);
    Require(legalMoves.size() == 1 && legalMoves[0].targetTileId == 3,
        "legal actions should contain only affordable, adjacent, unoccupied, walkable tiles");

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
    turnSetup.initialState.activeCharacterId = 10;
    turnSetup.initialState.characters[0].initiative = 20;
    turnSetup.initialState.characters[1].initiative = 95;
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

    endTurn.actorId = InvalidEntityId;
    for (int index = 0; index < 5; ++index)
    {
        Require(turns.ApplyAction(endTurn).success, "turn selection continuation should succeed without an active actor");
    }
    Require(turns.State().activeCharacterId == 20, "the first ready character should become active");
    Require(turns.State().characters[1].initiative == 90,
        "selecting a character should apply its configured turn delay in Core");

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
