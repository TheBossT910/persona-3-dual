#include "BattleCalcsCoreTestExample.hpp"

#include <algorithm>

namespace
{
constexpr float kLevelMultipliers[24] = {0.5f,  0.51f, 0.53f, 0.59f, 0.66f, 0.75f, 0.84f, 0.91f,
                                         0.97f, 0.99f, 1.0f,  1.0f,  1.0f,  1.0f,  1.01f, 1.03f,
                                         1.09f, 1.16f, 1.25f, 1.34f, 1.41f, 1.47f, 1.49f, 1.5f};
}

float battle::calcs::levelDifferenceMultiplier(std::uint32_t attackerLevel, std::uint32_t defenderLevel)
{
    std::int32_t diff = static_cast<std::int32_t>(attackerLevel) - static_cast<std::int32_t>(defenderLevel);
    diff = std::clamp(diff, static_cast<std::int32_t>(-13), static_cast<std::int32_t>(10));

    return kLevelMultipliers[diff + 13];
}

float battle::calcs::affinityMultiplier(std::uint32_t affinity)
{
    switch (affinity)
    {
    case 1:
        return 1.25f;
    case 2:
        return 0.5f;
    case 3:
        return 0.0f;
    case 4:
        return -1.0f;
    case 5:
        return -2.0f;
    default:
        return 1.0f;
    }
}
