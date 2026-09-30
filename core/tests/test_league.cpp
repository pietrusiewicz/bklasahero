// Testy rdzenia: terminarz kołowy, tabela, wynik.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include "bkh/league.h"
#include "bkh/random.h"

using namespace bkh;

namespace {

std::vector<Club> makeTenClubs(i32 tier = 0) {
    std::vector<Club> clubs;
    clubs.reserve(10);
    for (int i = 0; i < 10; ++i) {
        Club c{};
        c.id = i + 1;
        c.name = std::string("Club ") + std::to_string(i + 1);
        c.shortName = c.name;
        c.town = "Town";
        c.voivodeship = 0;
        c.lat = 52.0;
        c.lon = 21.0;
        c.population = 50000;
        c.placeOsmId = 100 + i;
        c.colors.primary = 0xFF000000u | static_cast<u32>(i * 0x100000);
        c.foundedYear = 1946 + i;
        c.strength = 40.0 + 2.0 * i;
        c.isPlayer = (i == 0);
        clubs.push_back(std::move(c));
    }
    (void)tier;
    return clubs;
}

TEST(LeagueTest, RoundRobinCoversAllPairs) {
    auto fixtures = League::buildRoundRobin(10);
    EXPECT_EQ(fixtures.size(), 45u);  // 10*9/2
    std::vector<int> appearances(10, 0);
    for (const auto& f : fixtures) {
        EXPECT_NE(f.home, f.away);
        ++appearances[f.home];
        ++appearances[f.away];
    }
    for (int i = 0; i < 10; ++i) EXPECT_EQ(appearances[i], 9);
}

TEST(LeagueTest, RoundRobinEachRoundFiveMatches) {
    auto fixtures = League::buildRoundRobin(10);
    std::vector<int> perRound(9, 0);
    for (const auto& f : fixtures) ++perRound[f.round];
    for (int r : perRound) EXPECT_EQ(r, 5);
}

TEST(LeagueTest, TableSortsByPoints) {
    League lg(makeTenClubs(), 0, 42, 0);
    Random rng(1);
    for (int r = 0; r < 9; ++r) {
        lg.simulateWholeRound(r, rng);
    }
    auto table = lg.table();
    ASSERT_EQ(table.size(), 10u);
    for (std::size_t i = 0; i + 1 < table.size(); ++i) {
        EXPECT_GE(table[i].points, table[i + 1].points);
    }
    EXPECT_TRUE(lg.isSeasonComplete());
}

TEST(LeagueTest, PromotionPlacesMatchRules) {
    League lg(makeTenClubs(), 4, 1, 0);  // III liga — promote=2
    auto promoted = lg.promotionPlaces();
    EXPECT_EQ(promoted.size(), 2u) << "before any results: best 2 by tie-breakers";
    Random rng(2);
    for (int r = 0; r < 9; ++r) lg.simulateWholeRound(r, rng);
    auto promoted2 = lg.promotionPlaces();
    EXPECT_EQ(promoted2.size(), 2u);
}

TEST(LeagueTest, PlayerFateAfterSeason) {
    League lg(makeTenClubs(), 0, 3, 0);
    Random rng(3);
    for (int r = 0; r < 9; ++r) lg.simulateWholeRound(r, rng);
    auto fate = lg.playerFate();
    EXPECT_NE(fate, League::PlayerFate::Unknown);
}

}  // namespace