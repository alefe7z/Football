// Goalkeeper dive decisions: where a keeper commits to diving on a penalty,
// and when he throws himself at a shot he can no longer reach by stepping
// across. No SDL / OpenGL dependency; unit tested headless.
//
// The animation system only starts a save animation that can touch the
// predicted ball, so a keeper used to stand still for anything out of reach
// (every well-struck penalty). A dive plan gives the keeper a committed
// target: the save animation is chosen against that target, while the touch
// itself is still checked against the real ball. Guess the wrong way and the
// keeper dives the wrong way; guess right and the ball can be saved.

#ifndef _HPP_FOOTBALL_ONTHEPITCH_GOALKEEPERDIVE
#define _HPP_FOOTBALL_ONTHEPITCH_GOALKEEPERDIVE

#include <algorithm>
#include <cmath>

namespace GoalkeeperDive {

// Furthest point a keeper can reach from where he stands, in metres: the
// side-dive animations touch the ball about 1.9 m to the side, and the
// animation system can stretch that by a few tenths.
inline constexpr float kMaxLateralReach = 2.3f;
inline constexpr float kMinDiveHeight = 0.15f;
inline constexpr float kMaxDiveHeight = 2.3f;
inline constexpr float kPostY = 3.4f;  // inside the post (goal half-width 3.66)

// Where a shot crosses the goal line, relative to the centre of the goal.
struct GoalLineCrossing {
  bool found = false;
  float y = 0.0f;
  float z = 0.0f;
  int time_ms = 0;  // from now
};

// Walk a ball prediction (predict(time_ms) returns something with coords[3])
// until it crosses the goal line at x = goalLineX, approached from the pitch.
// side is +1 when the goal line is at positive x, -1 otherwise.
template <typename Predict>
GoalLineCrossing FindGoalLineCrossing(Predict predict, float goalLineX, int side, int maxTime_ms,
                                      int step_ms = 10) {
  GoalLineCrossing result;
  auto previous = predict(0);
  for (int t = step_ms; t <= maxTime_ms; t += step_ms) {
    auto current = predict(t);
    const float before = previous.coords[0] * side;
    const float after = current.coords[0] * side;
    const float line = goalLineX * side;
    if (before < line && after >= line) {
      const float travel = after - before;
      const float bias = travel > 1e-5f ? (line - before) / travel : 1.0f;
      result.found = true;
      result.y = previous.coords[1] + (current.coords[1] - previous.coords[1]) * bias;
      result.z = previous.coords[2] + (current.coords[2] - previous.coords[2]) * bias;
      result.time_ms = t - step_ms + static_cast<int>(step_ms * bias);
      return result;
    }
    previous = current;
  }
  return result;
}

// Chance (0..1) that a keeper reads which way a penalty goes. Real keepers go
// the right way roughly half the time; quick, well-positioned keepers more
// often, and a composed taker hides it better.
inline float PenaltyReadChance(float reaction, float agility, float positioning,
                               float takerComposure) {
  const float keeper = reaction * 0.4f + agility * 0.25f + positioning * 0.35f;
  const float chance = 0.32f + keeper * 0.33f - takerComposure * 0.12f;
  return std::clamp(chance, 0.2f, 0.65f);
}

struct DivePlan {
  float y = 0.0f;  // target relative to the centre of the goal
  float z = 0.5f;
  bool readShot = false;  // went for the ball's real line
  bool stayedUp = false;  // stayed in the middle
};

// The keeper's penalty dive. rolls are uniform random numbers in [0, 1).
// - Read the shot (roll1 < readChance): dive to where the ball is going.
// - Otherwise guess: one time in eight stay in the middle, else pick a side
//   at random (which can still be the right side, just not the right spot).
inline DivePlan PlanPenaltyDive(float shotY, float shotZ, float readChance, float roll1,
                                float roll2, float roll3) {
  DivePlan plan;
  if (roll1 < readChance) {
    plan.readShot = true;
    plan.y = shotY;
    plan.z = shotZ;
  } else if (roll2 < 0.125f) {
    plan.stayedUp = true;
    plan.y = 0.0f;
    plan.z = 1.1f;
  } else {
    const float side = roll2 < 0.5625f ? -1.0f : 1.0f;
    plan.y = side * (1.4f + roll3 * 0.9f);
    plan.z = 0.3f + roll3 * 1.3f;
  }
  plan.y = std::clamp(plan.y, -kPostY, kPostY);
  plan.z = std::clamp(plan.z, kMinDiveHeight, kMaxDiveHeight);
  return plan;
}

// Keep a dive target within what the keeper standing at keeperY can reach.
inline float ClampToReach(float targetY, float keeperY) {
  return std::clamp(targetY, keeperY - kMaxLateralReach, keeperY + kMaxLateralReach);
}

// Open play: commit to a full-stretch dive when a shot on target is too wide
// to reach by moving across, and arrives soon enough that there is no time to.
inline bool ShouldCommitDive(float lateralDistance, float timeToLine_ms) {
  return std::fabs(lateralDistance) > 1.3f && timeToLine_ms > 120.0f && timeToLine_ms < 900.0f;
}

}  // namespace GoalkeeperDive

#endif
