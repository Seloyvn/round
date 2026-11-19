#pragma once

#include "round/core/Game.h"

#include <cstddef>
#include <vector>

namespace roundcore
{
struct VectorStepResult
{
    std::vector<ActionResult> results;
    std::vector<DecisionSnapshot> observations;
};

class ROUNDCORE_API VectorEnvironment
{
public:
    std::vector<ActionResult> Reset(std::vector<MatchSetup> setups);
    VectorStepResult Step(const std::vector<std::size_t>& legalActionIndices);
    std::vector<DecisionSnapshot> Observe() const;
    std::vector<NumericObservation> ObserveNumeric(const ObservationSchema& schema) const;
    std::vector<NumericObservation> ObserveTeamNumeric(
        const std::vector<Team>& perspectives,
        const ObservationSchema& schema) const;

    std::size_t Size() const noexcept;
    const Game& At(std::size_t environmentIndex) const;
    Game& At(std::size_t environmentIndex);

private:
    std::vector<Game> games_;
};
}
