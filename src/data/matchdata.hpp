#ifndef _HPP_MATCHDATA
#define _HPP_MATCHDATA

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "../gamedefines.hpp"
#include "defines.hpp"
#include "teamdata.hpp"

class MatchData {
public:
  MatchData(int team1DatabaseID, int team2DatabaseID);
  virtual ~MatchData() = default;

  TeamData* GetTeamData(int id) { return teamData[id].get(); }
  int GetGoalCount(int id) { return goalCount[id]; }
  void SetGoalCount(int id, int amount) { goalCount[id] = amount; }
  void AddPossessionTime_10ms(int teamID);
  unsigned long GetPossessionTime_ms(int teamID) { return possessionTime_ms[teamID]; }
  float GetPossessionFactor_60seconds() {
    return possession60seconds / 60.0f * 0.5f + 0.5f;
  }  // REMEMBER THESE ARE IRL INGAME SECONDS (because, I guess the tactics should be based on irl
     // possession time instead of gametime? not sure yet, think about this)
  void AddShot(int teamID) { shots[teamID] += 1; }
  int GetShots(int teamID) { return shots[teamID]; }
  void AddShotOnTarget(int teamID) { shotsOnTarget[teamID] += 1; }
  int GetShotsOnTarget(int teamID) { return shotsOnTarget[teamID]; }

  // pass tracking
  void AddPassAttempt(int teamID) {
    passAttempts[teamID]++;
    pendingPassTeamID = teamID;
  }
  void RecordBallTouch(int receivingTeamID) {
    if (pendingPassTeamID == receivingTeamID) {
      passesCompleted[receivingTeamID]++;
    }
    pendingPassTeamID = -1;
  }
  int GetPassAttempts(int teamID) const { return passAttempts[teamID]; }
  int GetPassesCompleted(int teamID) const { return passesCompleted[teamID]; }

  // foul tracking
  void AddFoul(int teamID) { foulsCommitted[teamID]++; }
  int GetFouls(int teamID) const { return foulsCommitted[teamID]; }

  // set-piece and discipline tracking
  void AddCorner(int teamID) { corners[teamID]++; }
  int GetCorners(int teamID) const { return corners[teamID]; }
  void AddOffside(int teamID) { offsides[teamID]++; }
  int GetOffsides(int teamID) const { return offsides[teamID]; }
  void AddYellowCard(int teamID) { yellowCards[teamID]++; }
  int GetYellowCards(int teamID) const { return yellowCards[teamID]; }
  void AddRedCard(int teamID) { redCards[teamID]++; }
  int GetRedCards(int teamID) const { return redCards[teamID]; }

  // Penalty shootout result (kicks from the penalty mark). The match score
  // stays a draw; the shootout only decides who goes through.
  void SetShootoutResult(int goals0, int goals1) {
    shootoutPlayed = true;
    shootoutGoals[0] = goals0;
    shootoutGoals[1] = goals1;
  }
  bool HasShootout() const { return shootoutPlayed; }
  int GetShootoutGoals(int teamID) const { return shootoutGoals[teamID]; }
  // Winner of the match including the shootout: 0, 1, or -1 for a draw.
  int GetWinnerTeamID() const {
    if (goalCount[0] != goalCount[1])
      return goalCount[0] > goalCount[1] ? 0 : 1;
    if (shootoutPlayed && shootoutGoals[0] != shootoutGoals[1])
      return shootoutGoals[0] > shootoutGoals[1] ? 0 : 1;
    return -1;
  }

  // Database IDs of the players who scored for a team, one entry per goal
  // (own goals are not credited to anyone).
  void AddGoalScorer(int teamID, int playerDatabaseID, const std::string& minute = "") {
    goalScorers[teamID].push_back(playerDatabaseID);
    goalMinutes[teamID].push_back(minute);
  }
  const std::vector<int>& GetGoalScorers(int teamID) const { return goalScorers[teamID]; }
  // Parallel to GetGoalScorers: when each goal was scored ("23'", "45+1'").
  const std::vector<std::string>& GetGoalMinutes(int teamID) const { return goalMinutes[teamID]; }

  // whether this match's result has already been written to match history
  bool IsHistorySaved() const { return historySaved; }
  void SetHistorySaved(bool value) { historySaved = value; }

protected:
  std::array<std::unique_ptr<TeamData>, 2> teamData;

  int goalCount[2];

  unsigned long possessionTime_ms[2];
  float possession60seconds;  // -600 to 600 for possession of team 1 / 2 respectively
  int shots[2];
  int shotsOnTarget[2];

  int passAttempts[2];
  int passesCompleted[2];
  int pendingPassTeamID;

  int foulsCommitted[2];
  int corners[2] = {0, 0};
  int offsides[2] = {0, 0};
  int yellowCards[2] = {0, 0};
  int redCards[2] = {0, 0};

  bool shootoutPlayed = false;
  int shootoutGoals[2] = {0, 0};

  std::array<std::vector<int>, 2> goalScorers;
  std::array<std::vector<std::string>, 2> goalMinutes;

  bool historySaved;
};

#endif
