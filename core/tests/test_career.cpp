// Testy rdzenia: kariera gracza.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include "bkh/career.h"
#include "bkh/place.h"

using namespace bkh;

namespace {

const char* kSampleCsv =
    "osm_id,name,place,population,lat,lon,voivodeship\n"
    "1,Warszawa,city,1800000,52.2297,21.0122,mazowieckie\n"
    "2,Lodz,city,750000,51.7592,19.1460,lodzkie\n"
    "3,Radom,city,220000,51.4027,21.1471,mazowieckie\n"
    "4,Plock,city,120000,52.5468,19.7069,mazowieckie\n"
    "5,Siedlce,city,76000,52.1677,22.2902,mazowieckie\n"
    "6,Radomsko,city,48000,51.0667,19.4500,lodzkie\n"
    "7,Kutno,city,45000,52.2333,19.3667,lodzkie\n"
    "8,Lowicz,city,30000,52.1056,19.9467,lodzkie\n"
    "9,Rawa Mazowiecka,city,18000,51.7667,20.2500,lodzkie\n"
    "10,Tomaszow Mazowiecki,city,65000,51.5333,20.0000,lodzkie\n";

PlaceCatalog makeCatalog() {
    PlaceParseReport rpt;
    return PlaceCatalog::parseCsv(kSampleCsv, rpt);
}

TEST(CareerTest, StartNewProducesValidState) {
    PlaceCatalog cat = makeCatalog();
    NewCareerParams p{};
    p.nickname = "Test";
    p.homeOsmId = 1;
    p.language = Lang::Pl;
    auto res = Career::startNew(p, cat);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value().state().tierIndex, 0);
    EXPECT_EQ(res.value().state().homeOsmId, 1);
    EXPECT_EQ(res.value().state().seasonNumber, 1);
}

TEST(CareerTest, BeginMatchProducesSetup) {
    PlaceCatalog cat = makeCatalog();
    NewCareerParams p{};
    p.nickname = "Test";
    p.homeOsmId = 1;
    p.language = Lang::Pl;
    auto cr = Career::startNew(p, cat);
    ASSERT_TRUE(cr.has_value());
    Random rng(cr.value().state().seed);
    auto setup = cr.value().beginMatch(rng);
    ASSERT_TRUE(setup.has_value());
    EXPECT_EQ(setup.value().opponent.id != 0, true);
    EXPECT_EQ(cr.value().state().phase, CareerPhase::MatchInProgress);
}

TEST(CareerTest, MatchSetupCarriesTownsAndDistance) {
    // Zapowiedź meczu pokazuje, skąd jedzie rywal, więc setup musi nieść
    // miejscowości obu klubów i policzoną odległość.
    PlaceCatalog cat = makeCatalog();
    NewCareerParams p{};
    p.nickname = "Test";
    p.homeOsmId = 1;  // Warszawa
    p.language = Lang::Pl;
    auto cr = Career::startNew(p, cat);
    ASSERT_TRUE(cr.has_value());
    Random rng(cr.value().state().seed);
    auto setup = cr.value().beginMatch(rng);
    ASSERT_TRUE(setup.has_value());

    const MatchSetup& s = setup.value();
    EXPECT_EQ(s.playerClub.town, "Warszawa");
    EXPECT_FALSE(s.opponent.town.empty());
    EXPECT_NE(s.opponent.town, s.playerClub.town);

    const f64 expected = PlaceCatalog::distanceKm(
        s.playerClub.lat, s.playerClub.lon, s.opponent.lat, s.opponent.lon);
    EXPECT_NEAR(s.distanceKm, expected, 1e-6);
    EXPECT_GT(s.distanceKm, 0.0);

    // Mapka zapowiedzi: cała liga z pozycjami, dokładnie jeden klub gracza.
    EXPECT_EQ(s.leagueClubs.size(), 10u);
    std::size_t players = 0;
    for (const Club& c : s.leagueClubs) {
        EXPECT_NE(c.town.empty(), true);
        EXPECT_NE(c.lat, 0.0);
        EXPECT_NE(c.lon, 0.0);
        if (c.isPlayer) ++players;
    }
    EXPECT_EQ(players, 1u);
}

TEST(CareerTest, AwardXpGrantsLevelsAndPoints) {
    PlaceCatalog cat = makeCatalog();
    NewCareerParams p{};
    p.nickname = "Test";
    p.homeOsmId = 1;
    auto cr = Career::startNew(p, cat);
    auto& c = cr.value();
    const int before = c.state().progression.level;
    const int pointsBefore = c.state().progression.skillPoints;
    c.awardXp(c.state().rules.xpForLevel(1) + 50);
    EXPECT_GE(c.state().progression.level, before + 1);
    EXPECT_GT(c.state().progression.skillPoints, pointsBefore);
}

TEST(CareerTest, UpgradeRequiresSkillPoints) {
    PlaceCatalog cat = makeCatalog();
    NewCareerParams p{};
    p.nickname = "Test";
    p.homeOsmId = 1;
    auto cr = Career::startNew(p, cat);
    auto& c = cr.value();
    EXPECT_FALSE(c.upgradeAttribute(AttributeKind::ShotPower).has_value());
    c.awardXp(c.state().rules.xpForLevel(1) + 100);
    EXPECT_TRUE(c.upgradeAttribute(AttributeKind::ShotPower).has_value());
}

TEST(CareerTest, EquipRequiresUnlocked) {
    PlaceCatalog cat = makeCatalog();
    NewCareerParams p{};
    p.nickname = "Test";
    p.homeOsmId = 1;
    auto cr = Career::startNew(p, cat);
    auto& c = cr.value();
    EXPECT_TRUE(c.unlockCosmetic("kit.bklasa.green").has_value());
    EXPECT_TRUE(c.equipCosmetic(CosmeticKind::Kit, "kit.bklasa.green").has_value());
    EXPECT_FALSE(c.equipCosmetic(CosmeticKind::Kit, "kit.zimowy").has_value());
}

}  // namespace