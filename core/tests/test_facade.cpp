// Testy integracyjne fasady: pełny przepływ poleceń JSON.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <cstring>
#include <fstream>
#include <sstream>

#include "bkh/facade.h"

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
    "10,Tomaszow Mazowiecki,city,65000,51.5333,20.0000,lodzkie\n"
    "11,Kielce,city,200000,50.8667,20.6167,swietokrzyskie\n"
    "12,Lublin,city,350000,51.2500,22.5667,lubelskie\n"
    "13,Bialystok,city,300000,53.1167,23.1667,podlaskie\n"
    "14,Olsztyn,city,170000,53.7833,20.5000,warminsko-mazurskie\n";

TEST(FacadeTest, VersionReturnsProtocolInfo) {
    Facade f;
    const std::string resp = f.command(R"({"cmd":"version"})");
    EXPECT_NE(resp.find("protocolVersion"), std::string::npos);
    EXPECT_NE(resp.find("saveSchemaVersion"), std::string::npos);
}

TEST(FacadeTest, LoadPlacesAndQuery) {
    Facade f;
    std::string err;
    EXPECT_TRUE(f.loadPlaces(kSampleCsv, std::strlen(kSampleCsv), err));
    EXPECT_TRUE(f.placesLoaded());
    EXPECT_EQ(f.placesCount(), 14u);
}

TEST(FacadeTest, SearchCityReturnsMatches) {
    Facade f;
    std::string err;
    ASSERT_TRUE(f.loadPlaces(kSampleCsv, std::strlen(kSampleCsv), err));
    const std::string resp = f.command(R"({"cmd":"searchCity","query":"war","limit":5})");
    EXPECT_NE(resp.find("Warszawa"), std::string::npos);
}

TEST(FacadeTest, UnknownCommandReturnsErrorEnvelope) {
    Facade f;
    std::string err;
    ASSERT_TRUE(f.loadPlaces(kSampleCsv, std::strlen(kSampleCsv), err));
    const std::string resp = f.command(R"({"cmd":"definitelyNotACommand"})");
    EXPECT_NE(resp.find("\"ok\":false"), std::string::npos);
}

TEST(FacadeTest, NoCareerBeforeStart) {
    Facade f;
    std::string err;
    ASSERT_TRUE(f.loadPlaces(kSampleCsv, std::strlen(kSampleCsv), err));
    const std::string resp = f.command(R"({"cmd":"career"})");
    EXPECT_NE(resp.find("\"ok\":false"), std::string::npos);
    EXPECT_NE(resp.find("error.no_career"), std::string::npos);
}

TEST(FacadeTest, NewCareerStartsAtTier0) {
    Facade f;
    std::string err;
    ASSERT_TRUE(f.loadPlaces(kSampleCsv, std::strlen(kSampleCsv), err));
    const std::string resp = f.command(R"({"cmd":"newCareer","nickname":"Ala","homeOsmId":1,"seed":1234})");
    EXPECT_NE(resp.find("\"ok\":true"), std::string::npos);
    EXPECT_NE(resp.find("B klasa"), std::string::npos);
    EXPECT_TRUE(f.hasCareer());
}

TEST(FacadeTest, SaveLoadRoundtrip) {
    Facade f;
    std::string err;
    ASSERT_TRUE(f.loadPlaces(kSampleCsv, std::strlen(kSampleCsv), err));
    f.command(R"({"cmd":"newCareer","nickname":"Ala","homeOsmId":1,"seed":1234})");
    const std::string saved = f.saveCareerToJson(1000);
    EXPECT_FALSE(saved.empty());
    Facade g;
    std::string err2;
    EXPECT_TRUE(g.loadPlaces(kSampleCsv, std::strlen(kSampleCsv), err2));
    EXPECT_TRUE(g.loadCareerFromJson(saved, err2));
    EXPECT_TRUE(g.hasCareer());
}

}  // namespace