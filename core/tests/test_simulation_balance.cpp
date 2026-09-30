// Testy balansu symulacji ligowej — regresja rozkładu wyników.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <cmath>

#include "bkh/league.h"
#include "bkh/random.h"
#include "bkh/shootout.h"

using namespace bkh;

namespace {

TEST(BalanceTest, EqualSeasonsHomeAwayNear50) {
    Random rng(123);
    int homeWins = 0;
    int awayWins = 0;
    constexpr int N = 1000;
    for (int i = 0; i < N; ++i) {
        ShootoutScore s = simulateShootout(50.0, 50.0, rng);
        if (s.home > s.away) ++homeWins;
        else if (s.away > s.home) ++awayWins;
    }
    // Przy idealnej symetrii odchylenie od 50% powinno być < 10 p.p.
    const f64 homeFrac = static_cast<f64>(homeWins) / N;
    EXPECT_NEAR(homeFrac, 0.5, 0.10);
    const f64 awayFrac = static_cast<f64>(awayWins) / N;
    EXPECT_NEAR(awayFrac, 0.5, 0.10);
}

TEST(BalanceTest, StrongerTeamWinsAboutTwoThirds) {
    Random rng(7);
    int wins = 0;
    constexpr int N = 1000;
    for (int i = 0; i < N; ++i) {
        ShootoutScore s = simulateShootout(80.0, 50.0, rng);
        if (s.home > s.away) ++wins;
    }
    const f64 frac = static_cast<f64>(wins) / N;
    EXPECT_GT(frac, 0.55);
    EXPECT_LT(frac, 0.80);
}

TEST(BalanceTest, LeagueSeasonProducesPromotionAndRelegation) {
    constexpr int NSeasons = 30;
    int promotions = 0;
    int relegations = 0;
    for (int s = 0; s < NSeasons; ++s) {
        std::vector<Club> clubs;
        clubs.reserve(10);
        for (int i = 0; i < 10; ++i) {
            Club c{};
            c.id = i + 1;
            c.name = "C" + std::to_string(i);
            c.shortName = c.name;
            c.town = "T";
            c.strength = 40.0 + 4.0 * i;
            c.isPlayer = (i == 0);
            clubs.push_back(std::move(c));
        }
        League lg(std::move(clubs), 2, s * 13ULL, 0);
        Random rng(s);
        for (int r = 0; r < 9; ++r) lg.simulateWholeRound(r, rng);
        if (lg.playerFate() == League::PlayerFate::Promoted ||
            lg.playerFate() == League::PlayerFate::Champion) ++promotions;
        if (lg.playerFate() == League::PlayerFate::Relegated) ++relegations;
    }
    // Oczekujemy choć kilku losowych awansów/spadków w 30 sezonach.
    EXPECT_GT(promotions + relegations, 1);
}

TEST(BalanceTest, AverageGoalsPerMatchReasonable) {
    Random rng(99);
    int totalGoals = 0;
    constexpr int N = 200;
    for (int i = 0; i < N; ++i) {
        LeagueMatchScore m = simulateLeagueMatch(50.0, 50.0, rng);
        totalGoals += m.homeGoals + m.awayGoals;
    }
    const f64 avg = static_cast<f64>(totalGoals) / N;
    EXPECT_GT(avg, 4.0);
    EXPECT_LT(avg, 9.0);
}

}  // namespace