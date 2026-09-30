// Testy rdzenia: katalog miejscowości.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

#include "bkh/place.h"

using namespace bkh;

namespace {

const char* kTinyCsv =
    "osm_id,name,place,population,lat,lon,voivodeship\n"
    "1,Warszawa,city,1800000,52.2297,21.0122,mazowieckie\n"
    "2,Lodz,city,750000,51.7592,19.1460,lodzkie\n"
    "3,Poznan,city,550000,52.4082,16.9335,wielkopolskie\n"
    "4,Krakow,city,800000,50.0647,19.9450,malopolskie\n"
    "5,Mala Wies,village,500,52.0000,21.0000,mazowieckie\n";

TEST(PlaceTest, ParsesCsv) {
    PlaceParseReport rpt;
    PlaceCatalog cat = PlaceCatalog::parseCsv(kTinyCsv, rpt);
    EXPECT_EQ(cat.places().size(), 5u);
    EXPECT_EQ(rpt.rowsAccepted, 5);
    EXPECT_EQ(rpt.rowsRejected, 0);
}

TEST(PlaceTest, FindByOsmId) {
    PlaceParseReport rpt;
    PlaceCatalog cat = PlaceCatalog::parseCsv(kTinyCsv, rpt);
    const Place* w = cat.findByOsmId(1);
    ASSERT_NE(w, nullptr);
    EXPECT_EQ(w->name, "Warszawa");
}

TEST(PlaceTest, SearchFindsByPrefix) {
    PlaceParseReport rpt;
    PlaceCatalog cat = PlaceCatalog::parseCsv(kTinyCsv, rpt);
    auto res = cat.search("lod", 10);
    ASSERT_GE(res.size(), 1u);
    EXPECT_EQ(res[0]->name, "Lodz");
}

TEST(PlaceTest, BestMatchPicksHighestPopulation) {
    PlaceParseReport rpt;
    PlaceCatalog cat = PlaceCatalog::parseCsv(kTinyCsv, rpt);
    const Place* p = cat.bestMatch("kra");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->name, "Krakow");
}

TEST(PlaceTest, DistanceIsRoughlySymmetric) {
    f64 d1 = PlaceCatalog::distanceKm(52.0, 21.0, 52.5, 21.5);
    f64 d2 = PlaceCatalog::distanceKm(52.5, 21.5, 52.0, 21.0);
    EXPECT_NEAR(d1, d2, 1e-6);
    EXPECT_GT(d1, 50.0);
    EXPECT_LT(d1, 100.0);
}

TEST(PlaceTest, LargestSortsByPopulation) {
    PlaceParseReport rpt;
    PlaceCatalog cat = PlaceCatalog::parseCsv(kTinyCsv, rpt);
    auto largest = cat.largest(2);
    ASSERT_EQ(largest.size(), 2u);
    EXPECT_EQ(largest[0]->name, "Warszawa");
}

TEST(PlaceTest, ChecksumIsStable) {
    PlaceParseReport rpt;
    PlaceCatalog cat1 = PlaceCatalog::parseCsv(kTinyCsv, rpt);
    PlaceCatalog cat2 = PlaceCatalog::parseCsv(kTinyCsv, rpt);
    EXPECT_EQ(cat1.checksum(), cat2.checksum());
}

}  // namespace