// Testy rdzenia: generowanie klubów.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include "bkh/club.h"
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

TEST(ClubTest, AdjectivalFormTransforms) {
    EXPECT_FALSE(ClubGenerator::adjectivalForm("Warszawa").empty());
    EXPECT_FALSE(ClubGenerator::adjectivalForm("Plock").empty());
}

TEST(ClubTest, MakeLeagueProducesTenClubs) {
    PlaceCatalog cat = makeCatalog();
    ClubGenerator gen(cat);
    Place home = *cat.findByOsmId(1);
    Random rng(42);
    std::size_t playerIdx = 999;
    auto clubs = gen.makeLeague(home, 0, 10, rng, playerIdx);
    EXPECT_EQ(clubs.size(), 10u);
    EXPECT_EQ(playerIdx, 0u);
    EXPECT_TRUE(clubs[0].isPlayer);
    EXPECT_EQ(clubs[0].placeOsmId, 1);
}

TEST(ClubTest, ClubNamesAreNonEmpty) {
    PlaceCatalog cat = makeCatalog();
    ClubGenerator gen(cat);
    Place home = *cat.findByOsmId(1);
    Random rng(7);
    std::size_t playerIdx = 999;
    auto clubs = gen.makeLeague(home, 0, 10, rng, playerIdx);
    for (const auto& c : clubs) {
        EXPECT_FALSE(c.name.empty());
        EXPECT_FALSE(c.shortName.empty());
        EXPECT_GT(c.strength, 0.0);
        EXPECT_LE(c.strength, 100.0);
    }
}

TEST(ClubTest, PyramidNamesStable) {
    EXPECT_STREQ(Pyramid::tierName(0, Lang::Pl).data(), "B klasa");
    EXPECT_STREQ(Pyramid::tierName(7, Lang::En).data(), "Ekstraklasa");
    EXPECT_STREQ(Pyramid::tierShortName(2, Lang::Pl).data(), "Okręgówka");
}

}  // namespace