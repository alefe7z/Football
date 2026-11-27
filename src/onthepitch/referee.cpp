#include "referee.hpp"

#include "../league/leaguecode.hpp"
#include "../main.hpp"
#include "../menu/career/career_database.hpp"
#include "AIsupport/AIfunctions.hpp"
#include "gameplaytuning.hpp"
#include "goalkeeperdive.hpp"
#include "managers/resourcemanagerpool.hpp"
#include "match.hpp"
#include "officials.hpp"
#include "player/playerofficial.hpp"
#include "scene/objectfactory.hpp"
#include "utils/localization.hpp"

Referee::Referee(Match* match) : match(match) {
  buffer.desiredSetPiece = e_SetPiece_KickOff;
  buffer.teamID = 0;
  buffer.stopTime = 0;
  buffer.prepareTime = 0;
  buffer.startTime = buffer.prepareTime + 2000;
  buffer.restartPos = Vector3(0, 0, 0);
  buffer.taker = nullptr;
  buffer.endPhase = true;
  buffer.active = true;

  foul.foulPlayer = nullptr;
  foul.foulVictim = nullptr;
  foul.foulType = 0;
  foul.advantage = false;
  foul.foulTime = 0;
  foul.hasBeenProcessed = true;

  afterSetPieceRelaxTime_ms = 0;

  // whistle

  boost::intrusive_ptr<Resource<SoundBuffer>> soundBufferRes =
      ResourceManagerPool::GetInstance()
          .GetManager<SoundBuffer>(e_ResourceType_SoundBuffer)
          ->Fetch("media/sounds/whistle2.wav", true, true);
  whistle[1] = boost::static_pointer_cast<Sound>(
      ObjectFactory::GetInstance().CreateObject("whistle1", e_ObjectType_Sound));
  GetScene3D()->CreateSystemObjects(whistle[1]);
  whistle[1]->SetSoundBuffer(soundBufferRes);
  // whistle[1]->SetGain(0.3 * GetConfiguration()->GetReal("audio_volume", 0.5));
  whistle[1]->SetLoop(false);
  GetScene3D()->AddObject(whistle[1]);

  soundBufferRes = ResourceManagerPool::GetInstance()
                       .GetManager<SoundBuffer>(e_ResourceType_SoundBuffer)
                       ->Fetch("media/sounds/whistle3.wav", true, true);
  whistle[3] = boost::static_pointer_cast<Sound>(
      ObjectFactory::GetInstance().CreateObject("whistle3", e_ObjectType_Sound));
  GetScene3D()->CreateSystemObjects(whistle[3]);
  whistle[3]->SetSoundBuffer(soundBufferRes);
  // whistle[3]->SetGain(0.3 * GetConfiguration()->GetReal("audio_volume", 0.5));
  whistle[3]->SetLoop(false);
  GetScene3D()->AddObject(whistle[3]);

  // for usage in destructor
  scene3D = GetScene3D();
}

Referee::~Referee() {
  if (Verbose())
    printf("exiting referee.. ");

  scene3D->DeleteObject(whistle[1]);
  scene3D->DeleteObject(whistle[3]);
  whistle[1].reset();
  whistle[3].reset();

  if (Verbose())
    printf("done\n");
}

void Referee::Process() {
  // printf("%i", match->GetMatchState());

  if (shootoutActive) {
    ProcessShootout();
    return;
  }

  if (match->IsInPlay() && !match->IsInSetPiece()) {
    Vector3 ballPos = match->GetBall()->Predict(0);

    // some phase is over :[

    // The period ends once its added time is played: with the ball near the
    // halfway line, or at most two minutes later wherever it is (waiting for
    // the ball alone could run a half 30 minutes over).
    const e_MatchPhase phase = match->GetMatchPhase();
    const bool timedPeriod = phase == e_MatchPhase_1stHalf || phase == e_MatchPhase_2ndHalf ||
                             phase == e_MatchPhase_1stExtraTime ||
                             phase == e_MatchPhase_2ndExtraTime;
    const unsigned long periodEnd_ms = match->GetPeriodEndTime_ms();
    const bool ballNearHalfway = ballPos.coords[0] < 10 && ballPos.coords[0] > -10;
    if (timedPeriod && match->GetAddedMinutes() >= 0 && match->GetMatchTime_ms() > periodEnd_ms &&
        (ballNearHalfway || match->GetMatchTime_ms() > periodEnd_ms + 120000)) {
      foul.advantage = false;
      if (!CheckFoul()) {
        match->StopPlay();
        Vector3 refPos(0, 0, 1.7f);
        if (match->GetOfficials() && match->GetOfficials()->GetReferee()) {
          refPos = match->GetOfficials()->GetReferee()->GetPosition() + Vector3(0, 0, 1.7f);
        }
        whistle[3]->SetPosition(refPos);
        whistle[3]->SetGain(0.3 * GetConfiguration()->GetReal("audio_volume", 0.5));
        whistle[3]->Poke(e_SystemType_Audio);

        buffer.desiredSetPiece = e_SetPiece_KickOff;
        buffer.stopTime = match->GetActualTime_ms();
        buffer.prepareTime = match->GetActualTime_ms() + 3000;
        buffer.startTime = buffer.prepareTime + 2000;
        buffer.restartPos = Vector3(0);
        buffer.active = true;
        buffer.endPhase = true;
        if (match->GetMatchPhase() == e_MatchPhase_1stHalf ||
            match->GetMatchPhase() == e_MatchPhase_1stExtraTime) {
          buffer.teamID = 1;
        } else {
          buffer.teamID = 0;
        }

        e_MatchPhase nextPhase;
        if (match->GetMatchPhase() == e_MatchPhase_1stHalf)
          nextPhase = e_MatchPhase_2ndHalf;
        if (match->GetMatchPhase() == e_MatchPhase_2ndHalf)
          nextPhase = e_MatchPhase_1stExtraTime;
        if (match->GetMatchPhase() == e_MatchPhase_1stExtraTime)
          nextPhase = e_MatchPhase_2ndExtraTime;
        if (match->GetMatchPhase() == e_MatchPhase_2ndExtraTime)
          nextPhase = e_MatchPhase_Penalties;
        match->SetMatchPhase(nextPhase);
      }
    }

    // goal kick / corner

    if (match->IsInPlay() && fabs(ballPos.coords[0]) > pitchHalfW + lineHalfW + 0.11) {
      foul.advantage = false;
      bool isFoul = false;
      if (!match->IsGoalScored())
        isFoul = CheckFoul();
      else {
        PreservePendingCaution();
        foul.foulType = 0;
        foul.hasBeenProcessed = true;
      }
      if (isFoul == false) {
        match->StopPlay();

        // corner, goal kick or kick off?
        signed int lastSide = -1;
        Team* lastTouchTeam = match->GetLastTouchTeam();
        if (lastTouchTeam == nullptr)
          lastTouchTeam = match->GetTeam(0);
        lastSide = lastTouchTeam->GetSide();

        if (match->IsGoalScored()) {
          buffer.desiredSetPiece = e_SetPiece_KickOff;
          buffer.stopTime = match->GetActualTime_ms();
          buffer.prepareTime = match->GetActualTime_ms() + 6000;
          buffer.startTime = buffer.prepareTime + 2000;
          buffer.restartPos = Vector3(0, 0, 0);
          buffer.teamID = abs(match->GetLastGoalTeamID() - 1);

        } else if ((ballPos.coords[0] > 0 && lastSide > 0) ||
                   (ballPos.coords[0] < 0 && lastSide < 0)) {
          buffer.desiredSetPiece = e_SetPiece_Corner;
          buffer.stopTime = match->GetActualTime_ms();
          buffer.prepareTime = match->GetActualTime_ms() + 2000;
          buffer.startTime = buffer.prepareTime + 2000;
          float y = ballPos.coords[1];
          if (y > 0)
            y = pitchHalfH;
          else
            y = -pitchHalfH;
          buffer.restartPos = Vector3(pitchHalfW * lastSide, y, 0);
          buffer.teamID = abs(lastTouchTeam->GetID() - 1);
          match->GetMatchData()->AddCorner(buffer.teamID);

        } else {
          buffer.desiredSetPiece = e_SetPiece_GoalKick;
          buffer.stopTime = match->GetActualTime_ms();
          buffer.prepareTime = match->GetActualTime_ms() + 2000;
          buffer.startTime = buffer.prepareTime + 2000;
          buffer.restartPos = Vector3(pitchHalfW * 0.92 * -lastSide, 0, 0);
          buffer.teamID = abs(lastTouchTeam->GetID() - 1);
        }

        buffer.active = true;
      }
    }

    // over sideline

    if (match->IsInPlay() && afterSetPieceRelaxTime_ms == 0) {
      if (fabs(ballPos.coords[1]) > pitchHalfH + lineHalfW + 0.11) {
        foul.advantage = false;
        if (!CheckFoul()) {
          match->StopPlay();
          Team* lastTouchTeam = match->GetLastTouchTeam();
          if (lastTouchTeam == nullptr)
            lastTouchTeam = match->GetTeam(0);
          buffer.teamID = abs(lastTouchTeam->GetID() - 1);
          buffer.desiredSetPiece = e_SetPiece_ThrowIn;
          buffer.stopTime = match->GetActualTime_ms();
          buffer.prepareTime = match->GetActualTime_ms() + 2000;
          buffer.startTime = buffer.prepareTime + 2000;
          buffer.restartPos.coords[0] =
              clamp(ballPos.coords[0], -pitchHalfW + 0.6f, pitchHalfW - 0.6f);
          if (ballPos.coords[1] > 0)
            buffer.restartPos.coords[1] = pitchHalfH;
          if (ballPos.coords[1] <= 0)
            buffer.restartPos.coords[1] = -pitchHalfH;
          buffer.restartPos.coords[2] = 0;
          buffer.active = true;
        }
      }
    }

    CheckFoul();

  } else {  // not in play, maybe something needs to happen?

    if (!match->IsInPlay() && !match->IsInSetPiece() && buffer.active == true) {
      if (buffer.stopTime + 300 == match->GetActualTime_ms() && buffer.endPhase == false &&
          buffer.desiredSetPiece != e_SetPiece_KickOff) {
        Vector3 refPos(0, 0, 1.7f);
        if (match->GetOfficials() && match->GetOfficials()->GetReferee()) {
          refPos = match->GetOfficials()->GetReferee()->GetPosition() + Vector3(0, 0, 1.7f);
        }
        whistle[1]->SetPosition(refPos);
        whistle[1]->SetGain(0.3 * GetConfiguration()->GetReal("audio_volume", 0.5));
        whistle[1]->Poke(e_SystemType_Audio);
      }

      if (buffer.prepareTime == match->GetActualTime_ms()) {
        if (buffer.endPhase == true) {
          if (match->GetMatchPhase() == e_MatchPhase_PreMatch) {
            match->SetMatchPhase(e_MatchPhase_1stHalf);
          } else {
            // game over conditions
            // Knockout ties: extra time and the shootout are opt-in;
            // league, career and friendly fixtures end level after 90'.
            // menu_smoke_force_shootout goes to a shootout whatever the
            // score, so automated runs can cover it.
            const bool forceShootout =
                GetConfiguration()->GetBool("menu_smoke_force_shootout", false);
            const KnockoutRules rules = ResolveKnockoutRules(
                GetConfiguration()->GetInt("match_knockout", 0),
                LeagueHasPendingFixture() || CareerDatabase::GetInstance().HasPendingFixture(),
                GetConfiguration()->GetBool("match_extra_time", false),
                GetConfiguration()->GetBool("match_penalties", false));
            const bool tied = match->GetScore(0) == match->GetScore(1) || forceShootout;
            const bool penalties = rules.penalties || forceShootout;
            if (match->GetMatchPhase() == e_MatchPhase_1stExtraTime) {
              const bool extraTime = rules.extraTime && !forceShootout;
              if (!tied || (!extraTime && !penalties)) {
                match->GameOver();
                return;
              }
              if (!extraTime) {
                StartShootout();
                return;
              }
            }
            if (match->GetMatchPhase() == e_MatchPhase_Penalties) {
              if (tied && penalties) {
                StartShootout();
                return;
              }
              match->GameOver();
              return;
            }
            match->sig_OnMatchPhaseChange(match);
          }
          buffer.endPhase = false;
        }

        PrepareSetPiece(buffer.desiredSetPiece);
      }

      if (buffer.startTime == match->GetActualTime_ms()) {
        // blow whistle and wait for set piece taker to touch the ball
        Vector3 refPos(0, 0, 1.7f);
        if (match->GetOfficials() && match->GetOfficials()->GetReferee()) {
          refPos = match->GetOfficials()->GetReferee()->GetPosition() + Vector3(0, 0, 1.7f);
        }
        whistle[1]->SetPosition(refPos);
        whistle[1]->SetGain(0.3 * GetConfiguration()->GetReal("audio_volume", 0.5));
        whistle[1]->Poke(e_SystemType_Audio);
        match->StartPlay();
        match->StartSetPiece();
      }
    }
  }

  IssuePendingCautions();

  if (match->IsInSetPiece()) {
    AnticipatePenalty();
    // check if set piece has been taken
    if (buffer.taker->TouchAnim() && !buffer.taker->TouchPending()) {
      buffer.active = false;
      if (buffer.desiredSetPiece == e_SetPiece_Penalty) {
        if (!penaltyDivePlanned)
          PlanPenaltyKeeperDive(buffer.teamID, 0);
        else
          RefinePenaltyKeeperDive(buffer.teamID);
      }
      match->StopSetPiece();
      match->GetTeam(0)->GetController()->PrepareSetPiece(e_SetPiece_None);
      match->GetTeam(1)->GetController()->PrepareSetPiece(e_SetPiece_None);
      afterSetPieceRelaxTime_ms = 400;
      foul.foulPlayer = 0;
      foul.foulType = 0;

      if (match->GetMatchPhase() == e_MatchPhase_PreMatch) {
        match->SetMatchPhase(e_MatchPhase_1stHalf);
      }
    }
  }

  if (afterSetPieceRelaxTime_ms > 0)
    afterSetPieceRelaxTime_ms -= 10;
}

void Referee::PrepareSetPiece(e_SetPiece setPiece) {
  // position players for set piece situation

  offsidePlayers.clear();
  penaltyDivePlanned = false;
  match->ResetSituation(buffer.restartPos);

  match->GetTeam(0)->GetController()->PrepareSetPiece(setPiece, buffer.teamID);
  match->GetTeam(1)->GetController()->PrepareSetPiece(setPiece, buffer.teamID);

  buffer.taker = match->GetTeam(buffer.teamID)->GetController()->GetPieceTaker();
}

void Referee::AlterSetPiecePrepareTime(unsigned long newTime_ms) {
  if (buffer.active) {
    buffer.prepareTime = newTime_ms;
    buffer.startTime = buffer.prepareTime + 2000;
  }
}

void Referee::BallTouched() {
  // check for offside player receiving the ball

  int lastTouchTeamID = match->GetLastTouchTeamID();
  if (lastTouchTeamID == -1)
    return;  // shouldn't happen really ;)
  if (match->IsInPlay() && !match->IsInSetPiece() && buffer.active == false &&
      match->GetTeam(abs(lastTouchTeamID - 1))->GetActivePlayerCount() >
          1) {  // disable if only 1 player: that's debug mode with only keeper
    std::map<Player*, Vector3>::iterator playerIter = offsidePlayers.begin();
    while (playerIter != offsidePlayers.end()) {
      if (match->GetTeam(lastTouchTeamID)->GetLastTouchPlayer() == playerIter->first) {
        foul.advantage = false;
        if (!CheckFoul()) {
          // uooooga uooooga offside!
          match->StopPlay();
          buffer.desiredSetPiece = e_SetPiece_FreeKick;
          buffer.stopTime = match->GetActualTime_ms();
          buffer.prepareTime = match->GetActualTime_ms() + 2000;
          buffer.startTime = buffer.prepareTime + 2000;
          buffer.restartPos = playerIter->second;
          buffer.teamID = abs(lastTouchTeamID - 1);
          buffer.active = true;
          match->GetMatchData()->AddOffside(lastTouchTeamID);
          match->SpamMessage(Localization::GetInstance().Translate("ingame_offside") + "!");
          break;
        } else
          break;
      }
      playerIter++;
    }
  }

  offsidePlayers.clear();

  if (match->IsInPlay() &&
      (buffer.active == false || !IsOffsideExemptRestart(buffer.desiredSetPiece))) {
    // check for offside players at moment of touch
    float offside = AI_GetOffsideLine(match, match->GetMentalImage(0), abs(lastTouchTeamID - 1));
    std::vector<Player*> players;
    Team* team = match->GetTeam(lastTouchTeamID);
    match->GetTeam(lastTouchTeamID)->GetActivePlayers(players);
    for (unsigned int i = 0; i < players.size(); i++) {
      if (players.at(i) != team->GetLastTouchPlayer()) {
        if (players.at(i)->GetPosition().coords[0] * team->GetSide() <
            offside * team->GetSide() - 0.20 /*relax*/) {
          offsidePlayers.insert(
              std::pair<Player*, Vector3>(players.at(i), players.at(i)->GetPosition()));
        }
      }
    }
  }
}

void Referee::TripNotice(Player* tripee, Player* tripper, int tackleType) {
  if (buffer.active)
    return;

  if (tackleType == 2) {  // standing tackle
    if (tripee->GetTeam()->GetFadingTeamPossessionAmount() > 1.1 &&
        (tripper->GetCurrentFunctionType() == e_FunctionType_Interfere ||
         tripper->GetCurrentFunctionType() == e_FunctionType_Sliding) &&
        (tripee->GetPosition() - match->GetBall()->Predict(0).Get2D()).GetLength() < 2.0 &&
        tripper->GetTeam()->GetID() != tripee->GetTeam()->GetID()) {
      // uooooga uooooga foul!
      PreservePendingCaution();
      foul.foulType = 1;
      foul.advantage = true;
      foul.foulPlayer = tripper;
      foul.foulVictim = tripee;
      foul.foulTime = match->GetActualTime_ms();
      foul.foulPosition = tripee->GetPosition();
      foul.hasBeenProcessed = false;
      match->GetMatchData()->AddFoul(tripper->GetTeamID());
      if (!IsReleaseVersion())
        match->SpamMessage(Localization::GetInstance().Translate("ingame_advantage"), 2000);
    }

  } else if (tackleType == 3 &&
             (tripper != foul.foulPlayer || foul.foulType == 0)) {  // sliding tackle

    if (match->GetActualTime_ms() - tripper->GetLastTouchTime_ms() > 600 &&
        tripper->GetCurrentFunctionType() == e_FunctionType_Sliding &&
        tripper->GetTeam()->GetID() != tripee->GetTeam()->GetID() &&
        (match->GetBall()->Predict(0) - tripee->GetPosition()).GetLength() < 8.0) {
      float severity = 1.0;
      if (tripper->TouchAnim()) {
        severity = std::pow(clamp(fabs(tripper->GetTouchFrame() - tripper->GetCurrentFrame()) /
                                      tripper->GetTouchFrame(),
                                  0.0, 1.0),
                            0.7) *
                   0.5;
        severity +=
            NormalizedClamp((match->GetBall()->Predict(0) - tripper->GetTouchPos()).GetLength(),
                            0.0, 2.0) *
            0.5;
      }
      // from behind?
      severity += (tripee->GetPosition() - tripper->GetPosition())
                          .GetNormalized(0)
                          .GetDotProduct(tripee->GetDirectionVec()) *
                      0.5 +
                  0.5;

      // PES 5/6: raise threshold slightly so marginal sliding tackles don't
      // always result in a foul — physical play is part of the game.
      if (severity > 1.05) {
        // uooooga uooooga foul!
        // printf("sliding! %lu ms ago\n", match->GetActualTime_ms() -
        // tripper->GetLastTouchTime_ms());
        PreservePendingCaution();
        foul.foulType = 1;
        foul.advantage = true;
        foul.foulPlayer = tripper;
        foul.foulVictim = tripee;
        foul.foulTime = match->GetActualTime_ms();
        foul.foulPosition = tripee->GetPosition();
        foul.hasBeenProcessed = false;
        match->GetMatchData()->AddFoul(tripper->GetTeamID());
        if (severity > 1.4) {
          foul.foulType = 2;
          // Avoid letting a player awaiting a second caution keep participating.
          if (tripper->GetCards() + pendingCautions.CountFor(tripper) >= 1)
            foul.advantage = false;
        }
        if (severity > 2.0) {
          foul.foulType = 3;
          foul.advantage = false;
        } else if (foul.advantage) {
          if (!IsReleaseVersion())
            match->SpamMessage(Localization::GetInstance().Translate("ingame_advantage"), 3000);
        }
      }
    }
  }
}

bool Referee::CheckFoul() {
  bool penalty = false;
  if (foul.foulType != 0 && foul.foulVictim && foul.foulVictim->GetTeam()) {
    if (fabs(foul.foulPosition.coords[1]) < 20.15 - lineHalfW &&
        foul.foulPosition.coords[0] * -foul.foulVictim->GetTeam()->GetSide() >
            pitchHalfW - 16.5 + lineHalfW)
      penalty = true;
  }

  if (foul.advantage) {
    if (penalty) {
      foul.advantage = false;
    } else {
      const bool possessionLost = foul.foulVictim && foul.foulVictim->GetTeam() &&
                                  foul.foulVictim->GetTeam()->GetFadingTeamPossessionAmount() < 1.0;
      const auto decision = GameplayTuning::EvaluateAdvantage(match->GetActualTime_ms(),
                                                              foul.foulTime, possessionLost);
      if (decision == GameplayTuning::AdvantageDecision::PlayedOut) {
        PreservePendingCaution();
        foul.foulPlayer = nullptr;
        foul.foulVictim = nullptr;
        foul.foulType = 0;
        foul.advantage = false;
        foul.hasBeenProcessed = true;
      } else if (decision == GameplayTuning::AdvantageDecision::RecallFoul) {
        foul.advantage = false;
      }
    }
  }

  if (foul.foulType != 0 && foul.advantage == false && !foul.hasBeenProcessed) {
    match->StopPlay();
    if (!penalty) {
      buffer.desiredSetPiece = e_SetPiece_FreeKick;
      buffer.stopTime = match->GetActualTime_ms();
      buffer.prepareTime = match->GetActualTime_ms() + 2000;
      if (foul.foulType >= 2)
        buffer.prepareTime += 10000;
      buffer.startTime = buffer.prepareTime + 2000;
      buffer.restartPos = foul.foulPosition;
    } else {
      buffer.desiredSetPiece = e_SetPiece_Penalty;
      buffer.stopTime = match->GetActualTime_ms();
      buffer.prepareTime = match->GetActualTime_ms() + 2000;
      if (foul.foulType >= 2)
        buffer.prepareTime += 10000;
      buffer.startTime = buffer.prepareTime + 2000;
      buffer.restartPos =
          Vector3((pitchHalfW - 11.0) * foul.foulPlayer->GetTeam()->GetSide(), 0, 0);
    }
    buffer.teamID = foul.foulVictim->GetTeam()->GetID();
    buffer.active = true;
    std::string spamMessage = Localization::GetInstance().Translate("ingame_foul") + "!";
    if (foul.foulType == 2) {
      spamMessage.append(" (" + Localization::GetInstance().Translate("ingame_yellow_card") + ")");
      match->GetMatchData()->AddYellowCard(foul.foulPlayer->GetTeamID());
      foul.foulPlayer->GiveYellowCard(match->GetActualTime_ms() +
                                      6000);  // need to find out proper moment
    }
    if (foul.foulType == 3) {
      spamMessage.append(" (" + Localization::GetInstance().Translate("ingame_red_card") + ")");
      match->GetMatchData()->AddRedCard(foul.foulPlayer->GetTeamID());
      foul.foulPlayer->GiveRedCard(match->GetActualTime_ms() +
                                   6000);  // need to find out proper moment
    }
    match->SpamMessage(spamMessage);

    foul.hasBeenProcessed = true;

    return true;
  }

  return false;
}

void Referee::PreservePendingCaution() {
  if (foul.foulType == 2 && !foul.hasBeenProcessed) {
    pendingCautions.Add(foul.foulPlayer);
    foul.hasBeenProcessed = true;
  }
}

void Referee::IssuePendingCautions() {
  // Resolve before the next restart, including goals and period breaks.
  // Keep the restart and current foul intact: the caution is a separate sanction.
  const auto cautions = pendingCautions.TakeAtStoppage(match->IsInPlay());
  for (Player* offender : cautions) {
    match->GetMatchData()->AddYellowCard(offender->GetTeamID());
    offender->GiveYellowCard(match->GetActualTime_ms());
    match->SpamMessage(Localization::GetInstance().Translate("ingame_yellow_card") + "!");
  }
}

// ---------------------------------------------------------------------------
// Penalty shootout

void Referee::BlowWhistle(int index) {
  if (!whistle[index])
    return;
  Vector3 refPos(0, 0, 1.7f);
  if (match->GetOfficials() && match->GetOfficials()->GetReferee()) {
    refPos = match->GetOfficials()->GetReferee()->GetPosition() + Vector3(0, 0, 1.7f);
  }
  whistle[index]->SetPosition(refPos);
  whistle[index]->SetGain(0.3 * GetConfiguration()->GetReal("audio_volume", 0.5));
  whistle[index]->Poke(e_SystemType_Audio);
}

void Referee::StartShootout() {
  shootoutActive = true;
  shootoutKickLive = false;
  shootoutFinished = false;
  match->SetMatchPhase(e_MatchPhase_Penalties);

  // Coin toss: the winner kicks first.
  const int firstTeam = static_cast<int>((match->GetActualTime_ms() / 10) % 2);
  shootout.Reset(firstTeam);

  // Kicker order per team: the best penalty takers first, the keeper last.
  for (int t = 0; t < 2; t++) {
    std::vector<Player*> players;
    match->GetTeam(t)->GetActivePlayers(players);
    std::vector<ShootoutCandidate<Player*>> candidates;
    for (Player* player : players) {
      candidates.push_back(
          {player,
           PenaltyTakerRating(player->GetStat("technical_shot"), player->GetStat("mental_calmness"),
                              player->GetStat("physical_shotpower")),
           player->GetFormationEntry().role == e_PlayerRole_GK});
    }
    shootoutOrder[t] = ShootoutKickerOrder(candidates);
    shootoutNextKicker[t] = 0;
  }

  match->GetMatchData()->SetShootoutResult(0, 0);
  buffer.active = false;
  buffer.endPhase = false;
  match->StopPlay();
  match->SpamMessage(Localization::GetInstance().Translate("ingame_penalty_shootout"), 3000);
  shootoutNextEvent_ms = match->GetActualTime_ms() + 3000;
  printf("[shootout] started, %s kick first\n",
         match->GetTeam(firstTeam)->GetTeamData()->GetName().c_str());
}

void Referee::ScheduleShootoutKick() {
  const int team = shootout.NextKickingTeam();
  if (team < 0)
    return;
  const int defending = 1 - team;

  // The next eligible kicker in this team's order (sent-off players skipped).
  Player* kicker = nullptr;
  std::vector<Player*>& order = shootoutOrder[team];
  for (std::size_t tries = 0; tries < order.size() && !kicker; tries++) {
    Player* candidate = order[shootoutNextKicker[team] % order.size()];
    shootoutNextKicker[team]++;
    if (candidate && candidate->IsActive())
      kicker = candidate;
  }
  match->GetTeam(team)->GetController()->SetForcedPieceTaker(kicker);

  shootoutKickTeam = team;
  buffer.desiredSetPiece = e_SetPiece_Penalty;
  buffer.teamID = team;
  buffer.stopTime = match->GetActualTime_ms();
  buffer.prepareTime = match->GetActualTime_ms() + 1000;
  buffer.startTime = buffer.prepareTime + 2500;
  buffer.restartPos = Vector3((pitchHalfW - 11.0) * match->GetTeam(defending)->GetSide(), 0, 0);
  buffer.endPhase = false;
  buffer.active = true;
}

bool Referee::ShootoutReboundTouched() const {
  // Only the kicker's strike and the defending goalkeeper's touches count; a
  // touch by anyone else after the kick ends the attempt.
  Player* keeper = match->GetTeam(1 - shootoutKickTeam)->GetGoalie();
  for (int t = 0; t < 2; t++) {
    std::vector<Player*> players;
    match->GetTeam(t)->GetActivePlayers(players);
    for (Player* player : players) {
      if (player == buffer.taker || player == keeper)
        continue;
      if (player->GetLastTouchTime_ms() > shootoutKickTime_ms)
        return true;
    }
  }
  return false;
}

void Referee::ShootoutBallInGoal(int scoringTeamID) {
  if (!shootoutActive || !shootoutKickLive)
    return;
  ResolveShootoutKick(scoringTeamID == shootoutKickTeam && !ShootoutReboundTouched());
}

void Referee::ResolveShootoutKick(bool scored) {
  shootoutKickLive = false;
  shootout.RecordKick(scored);
  match->StopPlay();
  match->GetMatchData()->SetShootoutResult(shootout.Goals(0), shootout.Goals(1));

  Localization& loc = Localization::GetInstance();
  const std::string tally = match->GetTeam(0)->GetTeamData()->GetName() + " " +
                            shootout.ScoreText() + " " +
                            match->GetTeam(1)->GetTeamData()->GetName();
  const std::string kickerName =
      buffer.taker ? buffer.taker->GetPlayerData()->GetLastName() + " - " : std::string();
  // Saved when the goalkeeper touched it after the strike.
  Player* keeper = match->GetTeam(1 - shootoutKickTeam)->GetGoalie();
  const bool saved = !scored && keeper && keeper->GetLastTouchTime_ms() >= shootoutKickTime_ms;
  if (saved)
    match->GetMatchData()->AddPenaltySave(1 - shootoutKickTeam);
  printf("[shootout] %s%s, %s\n", kickerName.c_str(),
         scored ? "scored" : (saved ? "saved" : "missed"), shootout.ScoreText().c_str());

  if (shootout.IsDecided()) {
    const int winner = shootout.Winner();
    match->SpamMessage(
        TRF("ingame_shootout_winner",
            {match->GetTeam(winner)->GetTeamData()->GetName(), shootout.ScoreText()}),
        5000);
    printf("[shootout] decided: %s win %s on penalties\n",
           match->GetTeam(winner)->GetTeamData()->GetName().c_str(), shootout.ScoreText().c_str());
    BlowWhistle(3);
    shootoutFinished = true;
    shootoutNextEvent_ms = match->GetActualTime_ms() + 5000;
  } else {
    const std::string outcome =
        scored ? loc.Translate("ingame_goal") + "!"
               : loc.Translate(saved ? "ingame_penalty_saved" : "ingame_penalty_missed");
    match->SpamMessage(kickerName + outcome + "  (" + tally + ")", 2500);
    shootoutNextEvent_ms = match->GetActualTime_ms() + 2500;
  }
}

void Referee::ProcessShootout() {
  const unsigned long now = match->GetActualTime_ms();

  if (shootoutFinished) {
    if (now >= shootoutNextEvent_ms) {
      shootoutFinished = false;
      match->GameOver();
    }
    return;
  }

  // Waiting between kicks.
  if (!buffer.active && !shootoutKickLive) {
    if (now >= shootoutNextEvent_ms)
      ScheduleShootoutKick();
    return;
  }

  // Setting up and taking the kick.
  if (buffer.active) {
    if (!match->IsInPlay() && !match->IsInSetPiece()) {
      if (now == buffer.prepareTime)
        PrepareSetPiece(e_SetPiece_Penalty);
      if (now == buffer.startTime) {
        BlowWhistle(1);
        match->StartPlay();
        match->StartSetPiece();
      }
    }
    if (match->IsInSetPiece())
      AnticipatePenalty();
    if (match->IsInSetPiece() && buffer.taker && buffer.taker->TouchAnim() &&
        !buffer.taker->TouchPending()) {
      buffer.active = false;
      if (!penaltyDivePlanned)
        PlanPenaltyKeeperDive(buffer.teamID, 0);
      else
        RefinePenaltyKeeperDive(buffer.teamID);
      match->StopSetPiece();
      match->GetTeam(0)->GetController()->PrepareSetPiece(e_SetPiece_None);
      match->GetTeam(1)->GetController()->PrepareSetPiece(e_SetPiece_None);
      shootoutKickLive = true;
      shootoutKickTime_ms = now;
    }
    return;
  }

  // Ball struck: a miss once it is saved and held, cleared, rebounds to
  // another player, leaves the pitch away from the goal mouth, or after a
  // few seconds without crossing the line.
  const Vector3 ballPos = match->GetBall()->Predict(0);
  const bool inGoalMouth = fabs(ballPos.coords[1]) < 3.7f && ballPos.coords[2] < 2.5f;
  const bool outOfPlay =
      (fabs(ballPos.coords[0]) > pitchHalfW + lineHalfW + 0.11 && !inGoalMouth) ||
      fabs(ballPos.coords[1]) > pitchHalfH + lineHalfW + 0.11;
  const bool keeperHolds =
      match->GetBallRetainer() != nullptr && match->GetBallRetainer() != buffer.taker;
  if (outOfPlay || keeperHolds || ShootoutReboundTouched() || now > shootoutKickTime_ms + 3500)
    ResolveShootoutKick(false);
}

// ---------------------------------------------------------------------------
// Penalty: the goalkeeper commits to a side as the ball is struck

void Referee::AnticipatePenalty() {
  if (buffer.desiredSetPiece != e_SetPiece_Penalty || penaltyDivePlanned || !buffer.taker)
    return;
  if (!buffer.taker->TouchAnim() || !buffer.taker->TouchPending())
    return;
  // Keepers move as the taker plants his foot, a moment before contact.
  const int framesToStrike = buffer.taker->GetTouchFrame() - buffer.taker->GetCurrentFrame();
  if (framesToStrike <= 15)
    PlanPenaltyKeeperDive(buffer.teamID, framesToStrike * 10);
}

void Referee::PlanPenaltyKeeperDive(int kickingTeamID, int strikeIn_ms) {
  penaltyDivePlanned = true;
  Team* defending = match->GetTeam(1 - kickingTeamID);
  Player* keeper = defending->GetGoalie();
  if (!keeper)
    return;

  const int side = defending->GetSide();
  const float lineX = pitchHalfW * side;
  const Vector3 ballPos = match->GetBall()->Predict(0);
  float shotY = 0.0f;
  float shotZ = 0.5f;
  int arrive_ms = strikeIn_ms + 450;
  if (strikeIn_ms > 0) {
    // Not struck yet: read where the taker means to put it.
    const Vector3 aim =
        buffer.taker->GetCurrentAnim()->originatingCommand.touchInfo.desiredDirection;
    const float distance = fabs(lineX - ballPos.coords[0]);
    if (fabs(aim.coords[0]) > 0.05f) {
      const float along = distance / fabs(aim.coords[0]);
      shotY = ballPos.coords[1] + aim.coords[1] * along;
      // The run-up shows the side, not the height: expect a typical
      // penalty height unless the aim says otherwise.
      shotZ = clamp(0.85f + aim.coords[2] * along, 0.15f, 2.3f);
    }
    arrive_ms = strikeIn_ms + static_cast<int>(distance / 33.0f * 1000.0f);
  } else {
    const auto crossing = GoalkeeperDive::FindGoalLineCrossing(
        [this](int t_ms) { return match->GetBall()->Predict(t_ms); }, lineX, side, 2000);
    if (crossing.found) {
      shotY = crossing.y;
      shotZ = crossing.z;
      arrive_ms = crossing.time_ms;
    }
  }

  const float takerComposure = buffer.taker ? buffer.taker->GetStat("mental_calmness") : 0.5f;
  const float readChance = GoalkeeperDive::PenaltyReadChance(
      keeper->GetStat("physical_reaction"), keeper->GetStat("physical_agility"),
      keeper->GetStat("mental_defensivepositioning"), takerComposure);
  const GoalkeeperDive::DivePlan plan = GoalkeeperDive::PlanPenaltyDive(
      shotY, shotZ, readChance, random(0, 1), random(0, 1), random(0, 1));

  const Vector3 keeperPos = keeper->GetPosition();
  penaltyDiveRead = plan.readShot;
  penaltyDiveTarget = Vector3(keeperPos.coords[0],
                              GoalkeeperDive::ClampToReach(plan.y, keeperPos.coords[1]), plan.z);
  const Vector3& target = penaltyDiveTarget;
  const unsigned long now = match->GetActualTime_ms();
  keeper->SetDivePlan(ballPos, target, now + arrive_ms, now + arrive_ms + 700);
  if (GetConfiguration()->GetBool("debug", false))
    printf("[keeper] %s %s (aim y %.1f z %.1f, dive y %.1f z %.1f, %d ms to the line)\n",
           keeper->GetPlayerData()->GetLastName().c_str(),
           plan.readShot ? "read the taker" : (plan.stayedUp ? "stayed up" : "guessed"), shotY,
           shotZ, target.coords[1], target.coords[2], arrive_ms);
}

// The ball has been struck. A keeper still waiting to go sees when it was hit,
// and one who read the taker now sees exactly where it is going.
void Referee::RefinePenaltyKeeperDive(int kickingTeamID) {
  Team* defending = match->GetTeam(1 - kickingTeamID);
  Player* keeper = defending->GetGoalie();
  if (!keeper || !keeper->HasDivePlan())
    return;  // already diving
  const int side = defending->GetSide();
  const auto crossing = GoalkeeperDive::FindGoalLineCrossing(
      [this](int t_ms) { return match->GetBall()->Predict(t_ms); }, pitchHalfW * side, side, 2000);
  if (!crossing.found)
    return;
  Vector3 target = penaltyDiveTarget;
  if (penaltyDiveRead) {
    target.coords[1] = GoalkeeperDive::ClampToReach(crossing.y, keeper->GetPosition().coords[1]);
    target.coords[2] =
        clamp(crossing.z, GoalkeeperDive::kMinDiveHeight, GoalkeeperDive::kMaxDiveHeight);
  }
  const unsigned long now = match->GetActualTime_ms();
  keeper->SetDivePlan(match->GetBall()->Predict(0), target, now + crossing.time_ms,
                      now + crossing.time_ms + 700);
}
