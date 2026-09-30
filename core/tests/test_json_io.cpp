// Testy rdzenia: serializacja JSON.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include "bkh/career.h"
#include "bkh/json_io.h"
#include "bkh/place.h"

using namespace bkh;
using namespace bkh::json_io;

namespace {

TEST(JsonIoTest, RoundTripEmptyCareer) {
    CareerState s{};
    s.nickname = "Empty";
    s.seed = 0xDEADBEEFULL;
    s.homeOsmId = 42;
    s.homeCity = "TestCity";
    s.homeVoivodeship = 5;
    s.cosmetics = defaultCosmetics();
    const std::string json = saveToJson(s, 1000);
    auto r = loadFromJson(json);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r.value().nickname, "Empty");
    EXPECT_EQ(r.value().seed, 0xDEADBEEFULL);
    EXPECT_EQ(r.value().homeOsmId, 42);
    EXPECT_EQ(r.value().homeCity, "TestCity");
    EXPECT_EQ(r.value().homeVoivodeship, 5);
    EXPECT_EQ(r.value().cosmetics.size(), defaultCosmetics().size());
}

TEST(JsonIoTest, ParsePlayerShotInput) {
    auto r = parsePlayerShotInput(R"({"aimX":1.5,"aimY":1.2,"effort":0.7,"spin":0.3,"loft":-0.1})");
    ASSERT_TRUE(r.has_value());
    EXPECT_NEAR(r.value().aimM.x, 1.5, 1e-6);
    EXPECT_NEAR(r.value().spin, 0.3, 1e-6);
}

TEST(JsonIoTest, ParsePlayerDiveInput) {
    auto r = parsePlayerDiveInput(R"({"side":"Left","height":"High","commit":true})");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r.value().side, DiveSide::Left);
    EXPECT_EQ(r.value().height, DiveHeight::High);
}

TEST(JsonIoTest, ParseNewCareerParams) {
    auto r = parseNewCareerParams(R"({"nickname":"Ala","homeOsmId":42,"language":"pl"})");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r.value().nickname, "Ala");
    EXPECT_EQ(r.value().homeOsmId, 42);
    EXPECT_EQ(r.value().language, Lang::Pl);
}

TEST(JsonIoTest, MappingsAreBijective) {
    for (u8 i = 0; i < static_cast<u8>(AttributeKind::Count); ++i) {
        const AttributeKind k = static_cast<AttributeKind>(i);
        const auto back = attributeKindFromKey(attributeKey(k));
        ASSERT_TRUE(back.has_value());
        EXPECT_EQ(static_cast<int>(*back), static_cast<int>(k));
    }
}

}  // namespace