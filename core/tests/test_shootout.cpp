// Testy rdzenia: konkurs rzutów karnych i symulacja ligowa.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <unordered_set>

#include "bkh/random.h"
#include "bkh/shootout.h"

using namespace bkh;

namespace {

TEST(ShootoutTest, StrongerTeamWinsOften) {
    Random rng(1);
    int strongerWins = 0;
    constexpr int N = 100;
    for (int i = 0; i < N; ++i) {
        ShootoutScore s = simulateShootout(80.0, 40.0, rng);
        if (s.home > s.away) ++strongerWins;
    }
    EXPECT_GT(strongerWins, N * 60 / 100);
}

TEST(ShootoutTest, EqualTeamsBalanceAround50) {
    Random rng(2);
    int homeWins = 0;
    constexpr int N = 200;
    for (int i = 0; i < N; ++i) {
        ShootoutScore s = simulateShootout(50.0, 50.0, rng);
        if (s.home > s.away) ++homeWins;
        if (s.away > s.home) --homeWins;
    }
    EXPECT_LT(std::abs(homeWins), N / 5);
}

TEST(ShootoutTest, SimulateLeagueMatchProducesGoals) {
    Random rng(1);
    int totalGoals = 0;
    constexpr int N = 100;
    for (int i = 0; i < N; ++i) {
        LeagueMatchScore m = simulateLeagueMatch(50.0, 50.0, rng);
        totalGoals += m.homeGoals + m.awayGoals;
    }
    EXPECT_GT(totalGoals, N);  // co najmniej 1 gol/mecz statystycznie
}

TEST(ShootoutStateTest, EarlyWinnerTriggersFinish) {
    Shootout s(ShootoutRules{});
    auto homeGoal = [] {
        KickRecord kr{}; kr.side = Side::Home; kr.shot.outcome = ShotOutcome::Goal; return kr;
    };
    auto awayGoal = [] {
        KickRecord kr{}; kr.side = Side::Away; kr.shot.outcome = ShotOutcome::Goal; return kr;
    };
    auto awayMiss = [] {
        KickRecord kr{}; kr.side = Side::Away; kr.shot.outcome = ShotOutcome::Saved; return kr;
    };
    // H A H A H A H A → 4:4 po 8
    for (int i = 0; i < 4; ++i) {
        EXPECT_TRUE(s.recordKick(homeGoal()));
        EXPECT_TRUE(s.recordKick(awayGoal()));
    }
    EXPECT_FALSE(s.isFinished());
    // 5:4 — home trafia.
    EXPECT_TRUE(s.recordKick(homeGoal()));
    EXPECT_FALSE(s.isFinished());  // obie strony mają jeszcze szansę na remis
    // 5:5 po ostatnim awayGoal — remis → SD.
    EXPECT_TRUE(s.recordKick(awayGoal()));
    EXPECT_FALSE(s.isFinished());
    EXPECT_TRUE(s.inSuddenDeath());
    EXPECT_EQ(s.state().suddenDeathRounds, 1);
    // W nagłej śmierci: home strzela (5:6), away pudłuje (5:6) → home wygrywa.
    EXPECT_TRUE(s.recordKick(homeGoal()));
    EXPECT_TRUE(s.recordKick(awayMiss()));
    EXPECT_TRUE(s.isFinished());
    ASSERT_TRUE(s.state().winner.has_value());
    EXPECT_EQ(s.state().winner.value(), Side::Home);
}

TEST(ShootoutStateTest, SuddenDeathAfterTie) {
    Shootout s(ShootoutRules{});
    for (int i = 0; i < 10; ++i) {
        KickRecord kr{};
        kr.side = (i % 2 == 0) ? Side::Home : Side::Away;
        kr.shot.outcome = ShotOutcome::Goal;
        EXPECT_TRUE(s.recordKick(kr));
    }
    EXPECT_TRUE(s.inSuddenDeath());
    EXPECT_EQ(s.state().suddenDeathRounds, 1);
    EXPECT_FALSE(s.isFinished());

    KickRecord h{}; h.side = Side::Home; h.shot.outcome = ShotOutcome::Goal;
    KickRecord a{}; a.side = Side::Away; a.shot.outcome = ShotOutcome::Saved;
    EXPECT_TRUE(s.recordKick(h));
    EXPECT_FALSE(s.isFinished());
    EXPECT_TRUE(s.recordKick(a));
    EXPECT_TRUE(s.isFinished());
    EXPECT_EQ(s.state().winner.value(), Side::Home);
}

TEST(ShootoutStateTest, PressureRisesWhenMustScore) {
    Shootout s(ShootoutRules{});
    // Stan 0:0, pierwsze 5 obu stron: bramkarze wygrywają → 0:0 → SD
    for (int i = 0; i < 10; ++i) {
        KickRecord kr{};
        kr.side = (i % 2 == 0) ? Side::Home : Side::Away;
        kr.shot.outcome = ShotOutcome::Saved;
        EXPECT_TRUE(s.recordKick(kr));
    }
    EXPECT_TRUE(s.inSuddenDeath());
    EXPECT_GT(s.pressureFor(Side::Home), 0.5);
}

}  // namespace