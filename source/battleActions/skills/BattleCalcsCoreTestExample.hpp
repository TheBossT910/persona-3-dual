#pragma once

#include <cstdint>

namespace battle::calcs
{
float levelDifferenceMultiplier(std::uint32_t attackerLevel, std::uint32_t defenderLevel);
float affinityMultiplier(std::uint32_t affinity);
} // namespace battle::calcs
