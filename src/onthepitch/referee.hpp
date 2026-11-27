#ifndef _HPP_REFEREE
#define _HPP_REFEREE

#include "../gamedefines.hpp"
#include "defines.hpp"
#include "penaltyshootout.hpp"
#include "pendingcautions.hpp"
#include "scene/objects/sound.hpp"
#include "scene/scene3d/scene3d.hpp"

using namespace blunted;

class Match;

struct RefereeBuffer {
  bool active;
  e_SetPiece desiredSetPiece;
  signed int teamID;
  unsigned long stopTime;
  unsigned long prepareTime;
  unsigned long startTime;
  Vector3 restartPos;
  Player* taker;
  bool endPhase;
};

struct Foul {
  Player* foulPlayer;
  Player* foulVictim;
  int foulType;  // 0: nothing, 1: foul, 2: yellow, 3: red
  bool advantage;
  unsigned long foulTime;
  Vector3 foulPosition;
  bool hasBeenProcessed;
};

class Referee {
public:
  Referee(Match* match);
  virtual ~Referee();

  void Process();

  void PrepareSetPiece(e_SetPiece setPiece);

  const RefereeBuffer& GetBuffer() { return buffer; };

  void AlterSetPiecePrepareTime(unsigned long newTime_ms);

  void BallTouched();
  void TripNotice(Player* tripee, Player* tripper,
                  int tackleType);  // 1 == standing tackle resulting in little trip, 2 == standing
                                    // tackle resulting in fall, 3 == sliding tackle
  bool CheckFoul();

  // Penalty shootout (kicks from the penalty mark) after a drawn knockout tie.
  bool IsShootoutActive() const { return shootoutActive; }
  const PenaltyShootout& GetShootout() const { return shootout; }
  // Called by the match when the ball crosses a goal line during the shootout.
  void ShootoutBallInGoal(int scoringTeamID);

  Player* GetCurrentFoulPlayer() { return foul.foulPlayer; }
  int GetCurrentFoulType() { return foul.foulType; }

protected:
  Match* match;

  std::shared_ptr<Scene3D> scene3D;

  RefereeBuffer buffer;

  int afterSetPieceRelaxTime_ms;  // throw-ins cause immediate new throw-ins, because ball is still
                                  // outside the lines at the moment of throwing ;)

  std::map<Player*, Vector3> offsidePlayers;  // player, position at time of touch

  Foul foul;
  PendingCautions<Player> pendingCautions;

  void PreservePendingCaution();
  void IssuePendingCautions();

  // The goalkeeper commits to a dive. With strikeIn_ms > 0 the ball has not
  // been struck yet and he reads the taker's intended direction.
  void PlanPenaltyKeeperDive(int kickingTeamID, int strikeIn_ms);
  void AnticipatePenalty();
  void RefinePenaltyKeeperDive(int kickingTeamID);
  bool penaltyDivePlanned = false;
  bool penaltyDiveRead = false;
  Vector3 penaltyDiveTarget;
  void StartShootout();
  void ProcessShootout();
  void ScheduleShootoutKick();
  void ResolveShootoutKick(bool scored);
  bool ShootoutReboundTouched() const;
  void BlowWhistle(int index);

  PenaltyShootout shootout;
  bool shootoutActive = false;
  bool shootoutKickLive = false;  // ball struck, waiting for the outcome
  bool shootoutFinished = false;
  int shootoutKickTeam = -1;
  unsigned long shootoutKickTime_ms = 0;
  unsigned long shootoutNextEvent_ms = 0;
  std::vector<Player*> shootoutOrder[2];
  std::size_t shootoutNextKicker[2] = {0, 0};

  boost::intrusive_ptr<Sound> whistle[4];  // 0: short, 1: long, 2: half time, 3: full time
};

#endif
