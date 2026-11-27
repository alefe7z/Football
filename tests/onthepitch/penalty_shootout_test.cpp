// Unit tests for the penalty shootout rules (onthepitch/penaltyshootout.hpp).

#include <gtest/gtest.h>

#include "onthepitch/penaltyshootout.hpp"

namespace {

void Kicks(PenaltyShootout& s, std::initializer_list<bool> outcomes) {
  for (bool scored : outcomes)
    s.RecordKick(scored);
}

}  // namespace

TEST(PenaltyShootout, TeamsAlternateStartingWithTheFirstKicker) {
  PenaltyShootout s(1);
  EXPECT_EQ(s.NextKickingTeam(), 1);
  s.RecordKick(true);
  EXPECT_EQ(s.NextKickingTeam(), 0);
  s.RecordKick(true);
  EXPECT_EQ(s.NextKickingTeam(), 1);
}

TEST(PenaltyShootout, AllFiveKicksEachWhenClose) {
  PenaltyShootout s(0);
  // 4-4 after four each, then home scores and away misses the last kick.
  Kicks(s, {true, true, true, true, true, true, true, true, true});
  EXPECT_FALSE(s.IsDecided());
  s.RecordKick(false);
  EXPECT_TRUE(s.IsDecided());
  EXPECT_EQ(s.Winner(), 0);
  EXPECT_EQ(s.ScoreText(), "5-4");
}

TEST(PenaltyShootout, EndsEarlyWhenTheTrailingTeamCannotCatchUp) {
  PenaltyShootout s(0);
  // Home 3/3, away 0/3: away can reach at most 2.
  Kicks(s, {true, false, true, false, true, false});
  EXPECT_TRUE(s.IsDecided());
  EXPECT_EQ(s.Winner(), 0);
  EXPECT_EQ(s.NextKickingTeam(), -1);
}

TEST(PenaltyShootout, EndsEarlyMidRoundForTheSecondKicker) {
  PenaltyShootout s(0);
  // Home misses 3 of 4, away scores 3 of 3: home can reach 2 at most.
  Kicks(s, {false, true, false, true, false, true, true});
  EXPECT_TRUE(s.IsDecided());
  EXPECT_EQ(s.Winner(), 1);
}

TEST(PenaltyShootout, SuddenDeathNeedsACompleteRound) {
  PenaltyShootout s(0);
  for (int i = 0; i < 10; i++)
    s.RecordKick(true);  // 5-5
  EXPECT_FALSE(s.IsDecided());
  EXPECT_TRUE(s.IsSuddenDeath());
  s.RecordKick(false);  // home misses: away still has to score
  EXPECT_FALSE(s.IsDecided());
  s.RecordKick(false);  // both missed: continue
  EXPECT_FALSE(s.IsDecided());
  s.RecordKick(true);
  s.RecordKick(false);
  EXPECT_TRUE(s.IsDecided());
  EXPECT_EQ(s.Winner(), 0);
  EXPECT_EQ(s.ScoreText(), "6-5");
}

TEST(PenaltyShootout, KicksAfterTheDecisionAreIgnored) {
  PenaltyShootout s(0);
  Kicks(s, {true, false, true, false, true, false});
  s.RecordKick(true);
  EXPECT_EQ(s.Taken(0), 3);
  EXPECT_EQ(s.Goals(0), 3);
}

TEST(PenaltyShootout, MarkersShowScoredMissedAndPending) {
  PenaltyShootout s(0);
  Kicks(s, {true, false, false});
  EXPECT_EQ(s.MarkerText(0), "O X - - -");
  EXPECT_EQ(s.MarkerText(1), "X - - - -");
}

TEST(PenaltyShootout, KickerOrderPutsBestTakersFirstAndKeeperLast) {
  std::vector<ShootoutCandidate<int>> candidates = {
      {1, 0.9f, true},  // keeper with a great rating still kicks last
      {2, 0.4f, false}, {3, 0.8f, false}, {4, 0.8f, false},  // tie with 3: input order kept
      {5, 0.6f, false},
  };
  const std::vector<int> order = ShootoutKickerOrder(candidates);
  EXPECT_EQ(order, (std::vector<int>{3, 4, 5, 2, 1}));
}

TEST(PenaltyShootout, TakerRatingWeighsFinishingAndComposure) {
  EXPECT_GT(PenaltyTakerRating(0.9f, 0.9f, 0.5f), PenaltyTakerRating(0.5f, 0.5f, 0.9f));
  EXPECT_FLOAT_EQ(PenaltyTakerRating(1.0f, 1.0f, 1.0f), 1.0f);
}

TEST(KnockoutRules, QuickMatchSettingSelectsPenaltiesAndExtraTime) {
  EXPECT_FALSE(ResolveKnockoutRules(0, false, false, false).penalties);
  const KnockoutRules pens = ResolveKnockoutRules(1, false, false, false);
  EXPECT_TRUE(pens.penalties);
  EXPECT_FALSE(pens.extraTime);
  const KnockoutRules both = ResolveKnockoutRules(2, false, false, false);
  EXPECT_TRUE(both.penalties);
  EXPECT_TRUE(both.extraTime);
}

TEST(KnockoutRules, LeagueAndCareerFixturesIgnoreTheQuickMatchSetting) {
  const KnockoutRules rules = ResolveKnockoutRules(2, true, false, false);
  EXPECT_FALSE(rules.penalties);
  EXPECT_FALSE(rules.extraTime);
}

TEST(KnockoutRules, LegacyConfigKeysStillApply) {
  EXPECT_TRUE(ResolveKnockoutRules(0, true, true, false).extraTime);
  EXPECT_TRUE(ResolveKnockoutRules(0, true, false, true).penalties);
}
