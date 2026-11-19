#include "round/core/VectorEnvironment.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
using namespace roundcore;

MatchSetup BuildMatch(const std::uint64_t seed)
{
    MatchSetup setup;
    setup.definitions.contentVersion = "vector-test-v1";
    setup.definitions.terrains.emplace("Arena", TerrainDefinition{"Arena", true, false, false, 1, 10000, {}});
    setup.definitions.characters.emplace(
        "Fighter", CharacterDefinition{"Fighter", StatBlock{10, 10, 0, 20, 5, 1, 2, 5, 5}, {}, {}});
    setup.initialState.matchId = seed;
    setup.initialState.seed = seed;
    setup.initialState.activeCharacterId = 100;
    setup.initialState.tiles = {
        TileState{1, HexCoord{0, 0}, "Arena", InvalidEntityId, false},
        TileState{2, HexCoord{1, 0}, "Arena", InvalidEntityId, false}};
    setup.initialState.characters = {
        CharacterState{100, "Fighter", Team::TeamA, 1, 10, 20, true, {}},
        CharacterState{200, "Fighter", Team::TeamB, 2, 10, 20, true, {}}};
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
    constexpr std::size_t environmentCount = 64;
    std::vector<MatchSetup> setups;
    for (std::size_t index = 0; index < environmentCount; ++index)
    {
        setups.push_back(BuildMatch(index + 1));
    }

    VectorEnvironment environments;
    const std::vector<ActionResult> resets = environments.Reset(std::move(setups));
    Require(environments.Size() == environmentCount, "vector reset should create every requested environment");
    Require(std::all_of(resets.begin(), resets.end(), [](const ActionResult& result) { return result.success; }),
        "every vector reset should succeed");

    const std::vector<DecisionSnapshot> before = environments.Observe();
    std::vector<std::size_t> attackIndices;
    attackIndices.reserve(environmentCount);
    for (const DecisionSnapshot& observation : before)
    {
        const auto attack = std::find_if(observation.legalActions.begin(), observation.legalActions.end(),
            [](const GameAction& action) { return action.type == ActionType::BasicAttack; });
        Require(attack != observation.legalActions.end(), "each test environment should expose an attack");
        attackIndices.push_back(static_cast<std::size_t>(std::distance(observation.legalActions.begin(), attack)));
    }

    const VectorStepResult step = environments.Step(attackIndices);
    Require(step.results.size() == environmentCount && step.observations.size() == environmentCount,
        "a vector step should return one result and observation per environment");
    Require(std::all_of(step.results.begin(), step.results.end(), [](const ActionResult& result) { return result.success; }),
        "every batched attack should succeed");
    Require(std::all_of(step.observations.begin(), step.observations.end(), [](const DecisionSnapshot& observation)
        { return observation.state.status == MatchStatus::Completed && observation.legalActions.empty(); }),
        "each independent environment should reach its own terminal state");

    const ObservationSchema schema = BuildObservationSchema(environments.At(0).Definitions());
    const std::vector<NumericObservation> numeric = environments.ObserveNumeric(schema);
    Require(numeric.size() == environmentCount, "numeric vector observations should preserve batch size");

    std::cout << "RoundCore vector environment tests passed\n";
    return EXIT_SUCCESS;
}
