#include "battleActions/skills/BattleCalcsCoreTestExample.hpp"

#include <gtest/gtest.h>

TEST(BattleCalcsCore, LevelDifferenceUsesCenterValueAtParity)
{
    EXPECT_FLOAT_EQ(battle::calcs::levelDifferenceMultiplier(40, 40), 1.0f);
}

TEST(BattleCalcsCore, LevelDifferenceClampsLowAndHighBounds)
{
    EXPECT_FLOAT_EQ(battle::calcs::levelDifferenceMultiplier(1, 99), 0.5f);
    EXPECT_FLOAT_EQ(battle::calcs::levelDifferenceMultiplier(99, 1), 1.5f);
}

TEST(BattleCalcsCore, LevelDifferenceUsesKnownTablePoint)
{
    EXPECT_FLOAT_EQ(battle::calcs::levelDifferenceMultiplier(20, 25), 0.97f);
}

TEST(BattleCalcsCore, AffinityMultiplierMatchesBattleAffinityMapping)
{
    EXPECT_FLOAT_EQ(battle::calcs::affinityMultiplier(0), 1.0f);
    EXPECT_FLOAT_EQ(battle::calcs::affinityMultiplier(1), 1.25f);
    EXPECT_FLOAT_EQ(battle::calcs::affinityMultiplier(2), 0.5f);
    EXPECT_FLOAT_EQ(battle::calcs::affinityMultiplier(3), 0.0f);
    EXPECT_FLOAT_EQ(battle::calcs::affinityMultiplier(4), -1.0f);
    EXPECT_FLOAT_EQ(battle::calcs::affinityMultiplier(5), -2.0f);
}

TEST(BattleCalcsCore, AffinityMultiplierDefaultsToNeutralForUnknownValues)
{
    EXPECT_FLOAT_EQ(battle::calcs::affinityMultiplier(999), 1.0f);
}
