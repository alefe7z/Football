// Unit tests for goalkeeper dive decisions (onthepitch/goalkeeperdive.hpp).

#include <gtest/gtest.h>

#include "onthepitch/goalkeeperdive.hpp"

using namespace GoalkeeperDive;

namespace {

struct P {
  float coords[3];
};

// A ball moving at constant velocity from (x0, y0, z0).
auto Straight(float x0, float y0, float z0, float vx, float vy, float vz) {
  return [=](int t_ms) {
    const float t = t_ms * 0.001f;
    return P{{x0 + vx * t, y0 + vy * t, z0 + vz * t}};
  };
}

}  // namespace

TEST(GoalkeeperDive, FindsWhereAPenaltyCrossesTheLine) {
  // From the spot (11 m out) towards the right corner at 22 m/s along x.
  const auto crossing =
      FindGoalLineCrossing(Straight(44.0f, 0.0f, 0.11f, 22.0f, 6.0f, 1.0f), 55.0f, 1, 3000);
  ASSERT_TRUE(crossing.found);
  EXPECT_NEAR(crossing.time_ms, 500, 10);
  EXPECT_NEAR(crossing.y, 3.0f, 0.1f);
  EXPECT_NEAR(crossing.z, 0.61f, 0.05f);
}

TEST(GoalkeeperDive, CrossingWorksForTheNegativeSideToo) {
  const auto crossing =
      FindGoalLineCrossing(Straight(-44.0f, 0.0f, 0.11f, -22.0f, -4.0f, 0.0f), -55.0f, -1, 3000);
  ASSERT_TRUE(crossing.found);
  EXPECT_NEAR(crossing.y, -2.0f, 0.1f);
}

TEST(GoalkeeperDive, NoCrossingForABallGoingAway) {
  EXPECT_FALSE(
      FindGoalLineCrossing(Straight(44.0f, 0.0f, 0.11f, -10.0f, 0.0f, 0.0f), 55.0f, 1, 3000).found);
}

TEST(GoalkeeperDive, ReadChanceRewardsGoodKeepersAndComposedTakers) {
  const float good = PenaltyReadChance(0.9f, 0.9f, 0.9f, 0.5f);
  const float poor = PenaltyReadChance(0.3f, 0.3f, 0.3f, 0.5f);
  EXPECT_GT(good, poor);
  EXPECT_LT(PenaltyReadChance(0.6f, 0.6f, 0.6f, 0.95f), PenaltyReadChance(0.6f, 0.6f, 0.6f, 0.2f));
  // Never a certainty either way.
  EXPECT_LE(PenaltyReadChance(1, 1, 1, 0), 0.65f);
  EXPECT_GE(PenaltyReadChance(0, 0, 0, 1), 0.2f);
}

TEST(GoalkeeperDive, ReadingTheShotDivesToTheBall) {
  const DivePlan plan = PlanPenaltyDive(2.8f, 0.4f, 0.5f, 0.1f, 0.9f, 0.5f);
  EXPECT_TRUE(plan.readShot);
  EXPECT_FLOAT_EQ(plan.y, 2.8f);
  EXPECT_FLOAT_EQ(plan.z, 0.4f);
}

TEST(GoalkeeperDive, AGuessAlwaysCommitsToASideOrStaysUp) {
  // Missed the read, guessed left.
  const DivePlan left = PlanPenaltyDive(2.8f, 0.4f, 0.5f, 0.9f, 0.3f, 0.5f);
  EXPECT_FALSE(left.readShot);
  EXPECT_LT(left.y, -1.5f);
  // Guessed right.
  EXPECT_GT(PlanPenaltyDive(2.8f, 0.4f, 0.5f, 0.9f, 0.8f, 0.5f).y, 1.5f);
  // Stayed in the middle.
  const DivePlan middle = PlanPenaltyDive(2.8f, 0.4f, 0.5f, 0.9f, 0.05f, 0.5f);
  EXPECT_TRUE(middle.stayedUp);
  EXPECT_FLOAT_EQ(middle.y, 0.0f);
}

TEST(GoalkeeperDive, TargetsStayInsideTheGoal) {
  const DivePlan plan = PlanPenaltyDive(5.0f, 4.0f, 0.5f, 0.0f, 0.0f, 0.0f);
  EXPECT_LE(plan.y, kPostY);
  EXPECT_LE(plan.z, kMaxDiveHeight);
  EXPECT_FLOAT_EQ(ClampToReach(3.4f, 0.0f), kMaxLateralReach);
  EXPECT_FLOAT_EQ(ClampToReach(-1.0f, 0.0f), -1.0f);
}

TEST(GoalkeeperDive, CommitsOnlyToWideShotsArrivingSoon) {
  EXPECT_TRUE(ShouldCommitDive(2.0f, 500.0f));
  EXPECT_FALSE(ShouldCommitDive(0.5f, 500.0f));   // close enough to step across
  EXPECT_FALSE(ShouldCommitDive(2.0f, 1500.0f));  // time to get there on foot
  EXPECT_FALSE(ShouldCommitDive(2.0f, 50.0f));    // too late to react
}
