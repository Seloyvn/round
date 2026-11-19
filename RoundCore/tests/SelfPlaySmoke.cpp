#include "round/core/Game.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace
{
using namespace roundcore;

MatchSetup BuildMatch(const std::uint64_t seed)
{
    MatchSetup setup;
    setup.definitions.contentVersion = "self-play-smoke-v1";
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

    CharacterState teamA;
    teamA.id = 100;
    teamA.definitionId = "Fighter";
    teamA.team = Team::TeamA;
    teamA.tileId = 1;
    teamA.health = 30;
    teamA.initiative = 20;
    setup.initialState.characters.push_back(teamA);

    CharacterState teamB = teamA;
    teamB.id = 200;
    teamB.team = Team::TeamB;
    teamB.tileId = 7;
    setup.initialState.characters.push_back(teamB);
    return setup;
}

const CharacterState* FindCharacter(const GameState& state, const EntityId id)
{
    const auto found = std::find_if(state.characters.begin(), state.characters.end(),
        [id](const CharacterState& character) { return character.id == id; });
    return found == state.characters.end() ? nullptr : &*found;
}

const TileState* FindTile(const GameState& state, const EntityId id)
{
    const auto found = std::find_if(state.tiles.begin(), state.tiles.end(),
        [id](const TileState& tile) { return tile.id == id; });
    return found == state.tiles.end() ? nullptr : &*found;
}

GameAction ChooseAction(const Game& game)
{
    const GameState& state = game.State();
    const EntityId actorId = state.activeCharacterId;
    const std::vector<GameAction> actions = game.LegalActions(actorId);

    const auto attack = std::find_if(actions.begin(), actions.end(), [](const GameAction& action)
    {
        return action.type == ActionType::BasicAttack;
    });
    if (attack != actions.end())
    {
        return *attack;
    }

    const CharacterState* actor = FindCharacter(state, actorId);
    const TileState* actorTile = actor ? FindTile(state, actor->tileId) : nullptr;
    GameAction bestMove;
    std::int32_t bestDistance = std::numeric_limits<std::int32_t>::max();
    for (const GameAction& action : actions)
    {
        if (action.type != ActionType::Move || !actor || !actorTile)
        {
            continue;
        }
        const TileState* destination = FindTile(state, action.targetTileId);
        if (!destination)
        {
            continue;
        }
        for (const CharacterState& enemy : state.characters)
        {
            if (!enemy.alive || enemy.team == actor->team)
            {
                continue;
            }
            const TileState* enemyTile = FindTile(state, enemy.tileId);
            if (!enemyTile)
            {
                continue;
            }
            const std::int32_t distance = HexDistance(destination->coord, enemyTile->coord);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestMove = action;
            }
        }
    }
    if (bestMove.actorId != InvalidEntityId)
    {
        return bestMove;
    }

    const auto endTurn = std::find_if(actions.begin(), actions.end(), [](const GameAction& action)
    {
        return action.type == ActionType::EndTurn;
    });
    return endTurn != actions.end() ? *endTurn : GameAction{};
}
}

int main()
{
    constexpr std::uint64_t matchCount = 500;
    for (std::uint64_t seed = 1; seed <= matchCount; ++seed)
    {
        Game game;
        if (!game.Reset(BuildMatch(seed)).success)
        {
            std::cerr << "Self-play reset failed for seed " << seed << '\n';
            return EXIT_FAILURE;
        }

        for (std::uint32_t decision = 0; decision < 1000 && !game.IsTerminal(); ++decision)
        {
            const GameAction action = ChooseAction(game);
            const ActionResult result = action.type == ActionType::EndTurn
                ? game.ApplyDecision(action)
                : game.ApplyAction(action);
            if (!result.success)
            {
                std::cerr << "Self-play action failed for seed " << seed
                          << ": " << result.errorCode << '\n';
                return EXIT_FAILURE;
            }
        }

        if (!game.IsTerminal() || game.State().winner == Team::Neutral)
        {
            std::cerr << "Self-play did not finish for seed " << seed << '\n';
            return EXIT_FAILURE;
        }
    }

    std::cout << "RoundCore completed " << matchCount << " headless matches\n";
    return EXIT_SUCCESS;
}
