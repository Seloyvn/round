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
    setup.definitions.terrains.emplace("Meadow", TerrainDefinition{"Meadow", true, false, false, 5, {}});
    setup.definitions.characters.emplace("Tank", CharacterDefinition{"Tank", StatBlock{100, 10, 5, 100, 5, 1, 10, 20, 10}, {}, {}});

    setup.initialState.matchId = 42;
    setup.initialState.seed = 1234;
    setup.initialState.activeCharacterId = 10;
    setup.initialState.tiles = {
        TileState{1, HexCoord{0, 0}, "Meadow", InvalidEntityId},
        TileState{2, HexCoord{1, 0}, "Meadow", InvalidEntityId}};
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
