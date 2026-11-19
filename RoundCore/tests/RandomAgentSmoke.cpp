#include "round/core/Game.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace
{
using namespace roundcore;

std::uint64_t NextRandom(std::uint64_t& state)
{
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    return state * 2685821657736338717ull;
}

MatchSetup BuildMatch(const std::uint64_t seed)
{
    MatchSetup setup;
    setup.definitions.contentVersion = "random-agent-smoke-v1";
    setup.definitions.terrains.emplace("Arena", TerrainDefinition{"Arena", true, false, false, 1, 10000, {}});

    CharacterDefinition fighter;
    fighter.id = "Fighter";
    fighter.stats = StatBlock{30, 10, 0, 20, 5, 1, 2, 5, 5};
    setup.definitions.characters.emplace(fighter.id, fighter);

    setup.initialState.matchId = seed;
    setup.initialState.seed = seed;
    setup.initialState.activeCharacterId = 100;
    for (std::int32_t q = 0; q < 7; ++q)
    {
        setup.initialState.tiles.push_back(TileState{
            static_cast<EntityId>(q + 1), HexCoord{q, 0}, "Arena", InvalidEntityId, false});
    }

    CharacterState first;
    first.id = 100;
    first.definitionId = "Fighter";
    first.team = Team::TeamA;
    first.tileId = 1;
    first.health = 30;
    first.initiative = 20;
    setup.initialState.characters.push_back(first);

    CharacterState second = first;
    second.id = 200;
    second.team = Team::TeamB;
    second.tileId = 7;
    setup.initialState.characters.push_back(second);
    return setup;
}
}

int main()
{
    using namespace roundcore;
    constexpr std::uint64_t matchCount = 2000;
    constexpr std::uint32_t maximumDecisions = 10000;
    std::uint64_t randomState = 0x4d595df4d0f33173ull;
    std::uint64_t totalDecisions = 0;
    const auto started = std::chrono::steady_clock::now();

    for (std::uint64_t seed = 1; seed <= matchCount; ++seed)
    {
        Game game;
        if (!game.Reset(BuildMatch(seed)).success)
        {
            std::cerr << "Random-agent reset failed for seed " << seed << '\n';
            return EXIT_FAILURE;
        }

        for (std::uint32_t decision = 0; decision < maximumDecisions && !game.IsTerminal(); ++decision)
        {
            const DecisionSnapshot observation = game.ObserveDecision();
            if (observation.legalActions.empty())
            {
                std::cerr << "Random agent received no legal actions for seed " << seed << '\n';
                return EXIT_FAILURE;
            }
            const std::size_t actionIndex = static_cast<std::size_t>(
                NextRandom(randomState) % observation.legalActions.size());
            const ActionResult result = game.ApplyDecisionIndex(actionIndex);
            if (!result.success)
            {
                std::cerr << "Random action failed for seed " << seed << ": " << result.errorCode << '\n';
                return EXIT_FAILURE;
            }
            ++totalDecisions;
        }

        if (!game.IsTerminal())
        {
            std::cerr << "Random-agent match exceeded its decision limit for seed " << seed << '\n';
            return EXIT_FAILURE;
        }
    }

    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    std::cout << "RoundCore random baseline completed " << matchCount << " matches and "
              << totalDecisions << " decisions in " << seconds << " seconds ("
              << static_cast<std::uint64_t>(totalDecisions / seconds) << " decisions/s)\n";
    return EXIT_SUCCESS;
}
