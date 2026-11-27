// Penalty shootout rules (IFAB Law 10, "Kicks from the penalty mark").
// This module has no SDL / OpenGL dependency and is unit tested headless.
//
// - Teams kick alternately (ABAB); the team listed first kicks first.
// - Each team takes five kicks unless the result is decided earlier: the
//   shootout ends as soon as one team has scored more than the other could
//   reach with all of its remaining kicks.
// - Still level after five kicks each: sudden death, one kick each, until one
//   team scores and the other does not in the same round.
// - Every eligible player kicks once before anyone kicks a second time.

#ifndef _HPP_FOOTBALL_ONTHEPITCH_PENALTYSHOOTOUT
#define _HPP_FOOTBALL_ONTHEPITCH_PENALTYSHOOTOUT

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

class PenaltyShootout {
public:
  static constexpr int kRegulationKicks = 5;

  explicit PenaltyShootout(int firstKickingTeam = 0) { Reset(firstKickingTeam); }

  void Reset(int firstKickingTeam) {
    first = (firstKickingTeam == 1) ? 1 : 0;
    goals[0] = goals[1] = 0;
    taken[0] = taken[1] = 0;
    history[0].clear();
    history[1].clear();
  }

  // Team (0 or 1) that takes the next kick, or -1 once the shootout is decided.
  int NextKickingTeam() const {
    if (IsDecided())
      return -1;
    return (taken[first] == taken[1 - first]) ? first : 1 - first;
  }

  // Record the outcome of the next kick. Ignored once the shootout is decided.
  void RecordKick(bool scored) {
    const int team = NextKickingTeam();
    if (team < 0)
      return;
    taken[team]++;
    if (scored)
      goals[team]++;
    history[team].push_back(scored);
  }

  bool IsDecided() const {
    const int remaining0 = std::max(0, kRegulationKicks - taken[0]);
    const int remaining1 = std::max(0, kRegulationKicks - taken[1]);
    if (taken[0] < kRegulationKicks || taken[1] < kRegulationKicks) {
      // Regulation rounds: decided once the trailing team cannot catch up.
      return goals[0] + remaining0 < goals[1] || goals[1] + remaining1 < goals[0];
    }
    // Sudden death: decided after a complete round with different scores.
    return taken[0] == taken[1] && goals[0] != goals[1];
  }

  // Winning team (0 or 1), or -1 while undecided.
  int Winner() const {
    if (!IsDecided())
      return -1;
    return goals[0] > goals[1] ? 0 : 1;
  }

  bool IsSuddenDeath() const {
    return taken[0] >= kRegulationKicks && taken[1] >= kRegulationKicks;
  }

  int Goals(int team) const { return (team == 0 || team == 1) ? goals[team] : 0; }
  int Taken(int team) const { return (team == 0 || team == 1) ? taken[team] : 0; }
  int FirstKickingTeam() const { return first; }
  const std::vector<bool>& History(int team) const { return history[team == 1 ? 1 : 0]; }

  // "3-2" style tally.
  std::string ScoreText() const {
    return std::to_string(goals[0]) + "-" + std::to_string(goals[1]);
  }

  // Kick markers for a scoreboard: "O" scored, "X" missed, "-" still to take
  // in the five regulation kicks. Sudden death kicks are appended.
  std::string MarkerText(int team) const {
    const std::vector<bool>& h = History(team);
    std::string text;
    const std::size_t shown = std::max<std::size_t>(kRegulationKicks, h.size());
    for (std::size_t i = 0; i < shown; i++) {
      if (!text.empty())
        text += ' ';
      text += (i < h.size()) ? (h[i] ? "O" : "X") : "-";
    }
    return text;
  }

private:
  int first = 0;
  int goals[2] = {0, 0};
  int taken[2] = {0, 0};
  std::vector<bool> history[2];
};

// A candidate kicker: an id (any stable handle) and how good a penalty taker
// they are (higher is better). The goalkeeper is flagged so they kick last.
template <typename Id>
struct ShootoutCandidate {
  Id id;
  float rating;
  bool goalkeeper;
};

// Order of kickers: best takers first, goalkeepers last; ties keep the input
// order so the result is deterministic. The order repeats once every
// candidate has kicked.
template <typename Id>
std::vector<Id> ShootoutKickerOrder(std::vector<ShootoutCandidate<Id>> candidates) {
  std::stable_sort(candidates.begin(), candidates.end(),
                   [](const ShootoutCandidate<Id>& a, const ShootoutCandidate<Id>& b) {
                     if (a.goalkeeper != b.goalkeeper)
                       return !a.goalkeeper;
                     return a.rating > b.rating;
                   });
  std::vector<Id> order;
  order.reserve(candidates.size());
  for (const auto& c : candidates)
    order.push_back(c.id);
  return order;
}

// Penalty-taking rating from player attributes (each 0..1): finishing and
// composure matter most, shot power a little.
inline float PenaltyTakerRating(float shot, float calmness, float shotPower) {
  return shot * 0.5f + calmness * 0.35f + shotPower * 0.15f;
}

// How a drawn match is settled. Quick Match exposes a "Knockout" option
// (config key match_knockout: 0 off, 1 penalties, 2 extra time + penalties).
// League and career fixtures ignore it: their rules come from the competition.
// The older hidden keys match_extra_time / match_penalties still apply.
struct KnockoutRules {
  bool extraTime = false;
  bool penalties = false;
};

inline KnockoutRules ResolveKnockoutRules(int quickMatchKnockout, bool competitiveFixture,
                                          bool extraTimeKey, bool penaltiesKey) {
  KnockoutRules rules;
  if (!competitiveFixture) {
    rules.penalties = quickMatchKnockout >= 1;
    rules.extraTime = quickMatchKnockout >= 2;
  }
  rules.extraTime = rules.extraTime || extraTimeKey;
  rules.penalties = rules.penalties || penaltiesKey;
  return rules;
}

#endif
