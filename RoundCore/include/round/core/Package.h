#pragma once

#include "round/core/Game.h"

#include <cstdint>
#include <vector>

namespace roundcore
{
constexpr std::uint32_t MatchPackageVersion = 18;

ROUNDCORE_API std::vector<std::uint8_t> SerializeMatchSetup(const MatchSetup& setup);
ROUNDCORE_API MatchSetup DeserializeMatchSetup(const std::vector<std::uint8_t>& bytes);
}
