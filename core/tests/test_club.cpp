// Testy rdzenia: generowanie klubów.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <set>
#include <string>

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

// --- Lokalność doboru rywali ----------------------------------------------
//
// Nazwy klubów mają być skorelowane z mapą: B klasa to liga OKOLICY miasta
// gracza, więc rywale muszą pochodzić z pobliskich miejscowości, a nie
// z największych miast w kraju.

namespace {

// Dom gracza + dziewięciu sąsiadów (4–27 km) + wielkie miasta w innych
// województwach jako „trucizna": gdyby generator sięgał po cały kraj,
// w lidze pojawiłby się Kraków zamiast Lipowca.
const char* kLocalityCsv =
    "osm_id,name,place,population,lat,lon,voivodeship\n"
    "100,Domowo,town,5000,52.0000,20.0000,mazowieckie\n"
    "101,Bliskowice,village,400,52.0400,20.0000,mazowieckie\n"
    "102,Sredniowo,village,350,52.0000,20.1000,mazowieckie\n"
    "103,Dalekus,village,300,52.0000,20.2000,mazowieckie\n"
    "104,Sosnowo,village,280,51.9000,20.0000,mazowieckie\n"
    "105,Wierzbno,village,260,52.1000,20.1000,mazowieckie\n"
    "106,Grabina,village,240,51.9500,20.1500,mazowieckie\n"
    "107,Ostrowiec,village,220,52.1500,19.9000,mazowieckie\n"
    "108,Kruszyna,village,200,51.8500,20.1000,mazowieckie\n"
    "109,Lipowiec,village,180,52.2000,20.2000,mazowieckie\n"
    "900,Krakow,city,766000,50.0614,19.9366,malopolskie\n"
    "901,Wroclaw,city,640000,51.1079,17.0385,dolnoslaskie\n"
    "902,Poznan,city,540000,52.4064,16.9252,wielkopolskie\n";

PlaceCatalog catalogFrom(const char* csv) {
    PlaceParseReport rpt;
    return PlaceCatalog::parseCsv(csv, rpt);
}

}  // namespace

TEST(ClubTest, RegionalLeagueComesFromNearbyPlaces) {
    PlaceCatalog cat = catalogFrom(kLocalityCsv);
    ClubGenerator gen(cat);
    const Place& home = *cat.findByOsmId(100);
    Random rng(2024);
    std::size_t playerIdx = 999;
    const auto clubs = gen.makeLeague(home, 0, 10, rng, playerIdx);

    ASSERT_EQ(clubs.size(), 10u);
    EXPECT_EQ(clubs[0].town, "Domowo");
    EXPECT_TRUE(clubs[0].isPlayer);

    const f64 cap = Pyramid::tier(0).radiusKm * 2.5;  // czapka promienia
    for (std::size_t i = 1; i < clubs.size(); ++i) {
        const Club& c = clubs[i];
        EXPECT_EQ(c.voivodeship, home.voivodeship) << c.name;
        const f64 distance = PlaceCatalog::distanceKm(home.lat, home.lon, c.lat, c.lon);
        EXPECT_LE(distance, cap) << c.name << " z " << c.town << " (" << distance << " km)";
    }
}

TEST(ClubTest, SparseRegionStaysInVoivodeshipInsteadOfWholeCountry) {
    // Tylko trzech sąsiadów w promieniu, reszta województwa daleko (200+ km),
    // a obok — wielkie miasta z innych województw.
    const char* csv =
        "osm_id,name,place,population,lat,lon,voivodeship\n"
        "100,Domowo,town,5000,52.0000,20.0000,mazowieckie\n"
        "101,Bliskowice,village,400,52.0400,20.0000,mazowieckie\n"
        "102,Sredniowo,village,350,52.0000,20.1000,mazowieckie\n"
        "103,Dalekus,village,300,52.0000,20.2000,mazowieckie\n"
        "201,Ostroleka,town,52000,53.0860,21.5760,mazowieckie\n"
        "202,Plock,town,118000,52.5468,19.7069,mazowieckie\n"
        "203,Siedlce,town,76000,52.1677,22.2902,mazowieckie\n"
        "204,Radom,city,210000,51.4027,21.1471,mazowieckie\n"
        "205,Ostrow Mazowiecka,town,22000,52.8020,21.8950,mazowieckie\n"
        "206,Ciechanow,town,44000,52.8815,20.6100,mazowieckie\n"
        "207,Mlawa,town,30000,53.1120,20.3840,mazowieckie\n"
        "208,Pultusk,town,19000,52.7030,21.0830,mazowieckie\n"
        "900,Krakow,city,766000,50.0614,19.9366,malopolskie\n"
        "901,Wroclaw,city,640000,51.1079,17.0385,dolnoslaskie\n"
        "902,Poznan,city,540000,52.4064,16.9252,wielkopolskie\n";
    PlaceCatalog cat = catalogFrom(csv);
    ClubGenerator gen(cat);
    const Place& home = *cat.findByOsmId(100);

    Random rng(7);
    const auto places = gen.selectRegionalPlaces(home, 0, 9, rng);
    ASSERT_EQ(places.size(), 9u);
    for (const Place* p : places) {
        // Nawet gdy w promieniu brakuje miejscowości, zostajemy w województwie —
        // nigdy nie sięgamy po Kraków czy Wrocław.
        EXPECT_EQ(p->voivodeship, home.voivodeship) << p->name;
    }
}

TEST(ClubTest, LeagueHasNoDuplicateTowns) {
    // Dwie różne miejscowości o tej samej nazwie (typowe „Zalesie") nie mogą
    // dać dwóch klubów o identycznej nazwie w jednej lidze.
    const char* csv =
        "osm_id,name,place,population,lat,lon,voivodeship\n"
        "100,Domowo,town,5000,52.0000,20.0000,mazowieckie\n"
        "200,Zalesie,village,300,52.0400,20.0000,mazowieckie\n"
        "201,Zalesie,village,280,52.0500,20.0500,mazowieckie\n"
        "202,Karniewo,village,400,52.0600,20.1000,mazowieckie\n"
        "203,Szczuki,village,350,52.0700,20.1500,mazowieckie\n"
        "204,Krasne,village,320,52.0800,20.2000,mazowieckie\n"
        "205,Dobrzankowo,village,300,52.0900,20.2500,mazowieckie\n"
        "206,Leszno,village,290,52.1000,20.3000,mazowieckie\n"
        "207,Golymin,village,270,52.1100,20.3500,mazowieckie\n"
        "208,Licowo,village,260,52.1200,20.4000,mazowieckie\n";
    PlaceCatalog cat = catalogFrom(csv);
    ClubGenerator gen(cat);
    const Place& home = *cat.findByOsmId(100);
    Random rng(11);
    std::size_t playerIdx = 999;
    const auto clubs = gen.makeLeague(home, 0, 10, rng, playerIdx);

    std::set<std::string> towns;
    for (const Club& c : clubs) {
        EXPECT_TRUE(towns.insert(c.town).second) << "powtórzona miejscowość: " << c.town;
    }
}

}  // namespace