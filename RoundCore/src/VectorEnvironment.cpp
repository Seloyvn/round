#include "round/core/VectorEnvironment.h"

#include <stdexcept>
#include <utility>

namespace roundcore
{
std::vector<ActionResult> VectorEnvironment::Reset(std::vector<MatchSetup> setups)
{
    games_.clear();
    games_.resize(setups.size());
    std::vector<ActionResult> results;
    results.reserve(setups.size());
    for (std::size_t index = 0; index < setups.size(); ++index)
    {
        results.push_back(games_[index].Reset(std::move(setups[index])));
    }
    return results;
}

VectorStepResult VectorEnvironment::Step(const std::vector<std::size_t>& legalActionIndices)
{
    if (legalActionIndices.size() != games_.size())
    {
        throw std::invalid_argument("VectorEnvironment requires exactly one action index per environment.");
    }

    VectorStepResult step;
    step.results.reserve(games_.size());
    step.observations.reserve(games_.size());
    for (std::size_t index = 0; index < games_.size(); ++index)
    {
        step.results.push_back(games_[index].ApplyDecisionIndex(legalActionIndices[index]));
        step.observations.push_back(games_[index].ObserveDecision());
    }
    return step;
}

std::vector<DecisionSnapshot> VectorEnvironment::Observe() const
{
    std::vector<DecisionSnapshot> observations;
    observations.reserve(games_.size());
    for (const Game& game : games_)
    {
        observations.push_back(game.ObserveDecision());
    }
    return observations;
}

std::vector<NumericObservation> VectorEnvironment::ObserveNumeric(const ObservationSchema& schema) const
{
    std::vector<NumericObservation> observations;
    observations.reserve(games_.size());
    for (const Game& game : games_)
    {
        observations.push_back(game.ObserveNumeric(schema));
    }
    return observations;
}

std::vector<NumericObservation> VectorEnvironment::ObserveTeamNumeric(
    const std::vector<Team>& perspectives,
    const ObservationSchema& schema) const
{
    if (perspectives.size() != games_.size())
    {
        throw std::invalid_argument("VectorEnvironment requires exactly one observation perspective per environment.");
    }
    std::vector<NumericObservation> observations;
    observations.reserve(games_.size());
    for (std::size_t index = 0; index < games_.size(); ++index)
    {
        observations.push_back(games_[index].ObserveTeamNumeric(perspectives[index], schema));
    }
    return observations;
}

std::size_t VectorEnvironment::Size() const noexcept
{
    return games_.size();
}

const Game& VectorEnvironment::At(const std::size_t environmentIndex) const
{
    return games_.at(environmentIndex);
}

Game& VectorEnvironment::At(const std::size_t environmentIndex)
{
    return games_.at(environmentIndex);
}
}
