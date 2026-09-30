// Implementacja serializacji JSON (nagłówki) i parsowania wejścia gracza.
// jedyne miejsce (oprócz facade.cpp), które włącza nlohmann/json.hpp.
// Zob. core/include/bkh/json_io.h dla kontraktu i docs/PROTOCOL.md dla kształtu.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "bkh/json_io.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <sstream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nlohmann/json.hpp>
#pragma GCC diagnostic pop

namespace bkh::json_io {

namespace {

using json = nlohmann::ordered_json;

std::string jsonToString(const json& j) { return j.dump(); }

// ----------------- drobne konwersje pomocnicze ----------------------------

std::optional<i64> asI64(const json& j) {
    if (j.is_number_integer()) return j.get<i64>();
    if (j.is_number_float()) return static_cast<i64>(j.get<f64>());
    if (j.is_string()) {
        const auto s = j.get<std::string>();
        char* end = nullptr;
        const long long v = std::strtoll(s.c_str(), &end, 10);
        if (end == s.c_str() + s.size()) return static_cast<i64>(v);
    }
    return std::nullopt;
}

std::optional<i32> asI32(const json& j) {
    const auto v = asI64(j);
    if (!v.has_value()) return std::nullopt;
    if (*v < static_cast<i64>(INT32_MIN) || *v > static_cast<i64>(INT32_MAX)) return std::nullopt;
    return static_cast<i32>(*v);
}

std::optional<f64> asF64(const json& j) {
    if (j.is_number()) return j.get<f64>();
    if (j.is_string()) {
        const auto s = j.get<std::string>();
        char* end = nullptr;
        const f64 v = std::strtod(s.c_str(), &end);
        if (end == s.c_str() + s.size()) return v;
    }
    return std::nullopt;
}

std::optional<bool> asBool(const json& j) {
    if (j.is_boolean()) return j.get<bool>();
    if (j.is_number_integer()) return j.get<i64>() != 0;
    return std::nullopt;
}

std::optional<u64> asU64(const json& j) {
    const auto v = asI64(j);
    if (!v.has_value() || *v < 0) return std::nullopt;
    return static_cast<u64>(*v);
}

std::string asString(const json& j) {
    if (j.is_string()) return j.get<std::string>();
    return j.dump();
}

// ----------------- enums i messageKey -------------------------------------

std::optional<AttributeKind> attrFromKey(const std::string& k) {
    if (k == "ShotPower") return AttributeKind::ShotPower;
    if (k == "ShotAccuracy") return AttributeKind::ShotAccuracy;
    if (k == "Composure") return AttributeKind::Composure;
    if (k == "Curve") return AttributeKind::Curve;
    if (k == "Reflexes") return AttributeKind::Reflexes;
    if (k == "Reach") return AttributeKind::Reach;
    if (k == "Reading") return AttributeKind::Reading;
    if (k == "Handling") return AttributeKind::Handling;
    return std::nullopt;
}

const char* attrKey(AttributeKind kind) {
    switch (kind) {
        case AttributeKind::ShotPower: return "ShotPower";
        case AttributeKind::ShotAccuracy: return "ShotAccuracy";
        case AttributeKind::Composure: return "Composure";
        case AttributeKind::Curve: return "Curve";
        case AttributeKind::Reflexes: return "Reflexes";
        case AttributeKind::Reach: return "Reach";
        case AttributeKind::Reading: return "Reading";
        case AttributeKind::Handling: return "Handling";
        case AttributeKind::Count: return "Count";
    }
    return "Unknown";
}

std::optional<CosmeticKind> cosmFromKey(const std::string& k) {
    if (k == "Kit") return CosmeticKind::Kit;
    if (k == "Ball") return CosmeticKind::Ball;
    if (k == "Gloves") return CosmeticKind::Gloves;
    if (k == "Net") return CosmeticKind::Net;
    return std::nullopt;
}

const char* cosmKey(CosmeticKind kind) {
    switch (kind) {
        case CosmeticKind::Kit: return "Kit";
        case CosmeticKind::Ball: return "Ball";
        case CosmeticKind::Gloves: return "Gloves";
        case CosmeticKind::Net: return "Net";
        case CosmeticKind::Count: return "Count";
    }
    return "Unknown";
}

std::optional<DiveSide> sideFromKey(const std::string& k) {
    if (k == "Left") return DiveSide::Left;
    if (k == "Center") return DiveSide::Center;
    if (k == "Right") return DiveSide::Right;
    return std::nullopt;
}

std::optional<DiveHeight> heightFromKey(const std::string& k) {
    if (k == "Low") return DiveHeight::Low;
    if (k == "Mid") return DiveHeight::Mid;
    if (k == "High") return DiveHeight::High;
    return std::nullopt;
}

}  // namespace

// ----------------- Enkodery ------------------------------------------------

std::string toJson(const PlayerAttributes& a) {
    json j = json::object();
    json obj = json::object();
    obj["Composure"] = a.get(AttributeKind::Composure);
    obj["Curve"] = a.get(AttributeKind::Curve);
    obj["Handling"] = a.get(AttributeKind::Handling);
    obj["Reach"] = a.get(AttributeKind::Reach);
    obj["Reading"] = a.get(AttributeKind::Reading);
    obj["Reflexes"] = a.get(AttributeKind::Reflexes);
    obj["ShotAccuracy"] = a.get(AttributeKind::ShotAccuracy);
    obj["ShotPower"] = a.get(AttributeKind::ShotPower);
    j["attributes"] = obj;
    return jsonToString(j["attributes"]);
}

std::string toJson(const Progression& p) {
    json j;
    j["level"] = p.level;
    j["skillPoints"] = p.skillPoints;
    j["totalXp"] = p.totalXp;
    j["xp"] = p.xp;
    return jsonToString(j);
}

std::string toJson(const CareerStats& s) {
    json j;
    j["bestWinStreak"] = s.bestWinStreak;
    j["championships"] = s.championships;
    j["cleanShootouts"] = s.cleanShootouts;
    j["currentWinStreak"] = s.currentWinStreak;
    j["goalsConceded"] = s.goalsConceded;
    j["goalsScored"] = s.goalsScored;
    j["losses"] = s.losses;
    j["matches"] = s.matches;
    j["perfectShootouts"] = s.perfectShootouts;
    j["promotions"] = s.promotions;
    j["relegations"] = s.relegations;
    j["saves"] = s.saves;
    j["savesAttempted"] = s.savesAttempted;
    j["seasonsPlayed"] = s.seasonsPlayed;
    j["shotsOnTarget"] = s.shotsOnTarget;
    j["shotsTaken"] = s.shotsTaken;
    j["suddenDeathMatches"] = s.suddenDeathMatches;
    j["wins"] = s.wins;
    j["woodworkHits"] = s.woodworkHits;
    return jsonToString(j);
}

std::string toJson(const CosmeticItem& c) {
    json j;
    j["colorPrimary"] = c.colorPrimary;
    j["colorSecondary"] = c.colorSecondary;
    j["defaultItem"] = c.defaultItem;
    j["id"] = c.id;
    j["kind"] = cosmKey(c.kind);
    j["nameKey"] = c.nameKey;
    j["unlockRequirementKey"] = c.unlockRequirementKey;
    j["unlocked"] = c.unlocked;
    return jsonToString(j);
}

std::vector<std::string> toJsonList(const std::vector<CosmeticItem>& items) {
    std::vector<std::string> out;
    out.reserve(items.size());
    for (const auto& c : items) out.push_back(toJson(c));
    return out;
}

std::string toJson(const Club& c) {
    json j;
    j["colors"] = {{"accent", c.colors.accent}, {"primary", c.colors.primary}, {"secondary", c.colors.secondary}};
    j["foundedYear"] = c.foundedYear;
    j["id"] = c.id;
    j["isPlayer"] = c.isPlayer;
    j["lat"] = c.lat;
    j["lon"] = c.lon;
    j["name"] = c.name;
    j["placeOsmId"] = c.placeOsmId;
    j["population"] = c.population;
    j["shortName"] = c.shortName;
    j["strength"] = c.strength;
    j["town"] = c.town;
    j["voivodeship"] = c.voivodeship;
    return jsonToString(j);
}

std::vector<std::string> toJsonList(const std::vector<Club>& clubs) {
    std::vector<std::string> out;
    out.reserve(clubs.size());
    for (const auto& c : clubs) out.push_back(toJson(c));
    return out;
}

std::string toJson(const Place& p) {
    json j;
    j["lat"] = p.lat;
    j["lon"] = p.lon;
    j["name"] = p.name;
    j["osmId"] = p.osmId;
    j["place"] = placeTypeName(p.type);
    j["population"] = p.population;
    j["voivodeship"] = p.voivodeship;
    return jsonToString(j);
}

std::vector<std::string> toJsonList(const std::vector<const Place*>& places) {
    std::vector<std::string> out;
    out.reserve(places.size());
    for (const Place* p : places) out.push_back(toJson(*p));
    return out;
}

std::string toJson(const LeagueRow& r) {
    json j;
    j["clubIndex"] = r.clubIndex;
    j["goalDifference"] = r.goalDifference();
    j["goalsAgainst"] = r.goalsAgainst;
    j["goalsFor"] = r.goalsFor;
    j["losses"] = r.losses;
    j["played"] = r.played;
    j["points"] = r.points;
    j["position"] = r.position;
    j["wins"] = r.wins;
    return jsonToString(j);
}

std::vector<std::string> toJsonList(const std::vector<LeagueRow>& rows) {
    std::vector<std::string> out;
    out.reserve(rows.size());
    for (const auto& r : rows) out.push_back(toJson(r));
    return out;
}

std::string toJson(const Fixture& f) {
    json j;
    j["away"] = f.away;
    j["home"] = f.home;
    j["matchIndex"] = f.matchIndex;
    j["round"] = f.round;
    return jsonToString(j);
}

std::string toJson(const MatchResultRecord& r) {
    json j;
    j["awayGoals"] = r.awayGoals;
    j["awayWins"] = r.awayWins;
    j["homeGoals"] = r.homeGoals;
    j["homeWins"] = r.homeWins;
    j["played"] = r.played;
    j["playerPlayed"] = r.playerPlayed;
    j["playerWon"] = r.playerWon;
    j["shootoutRounds"] = r.shootoutRounds;
    return jsonToString(j);
}

std::string toJson(const ShotResolution& s) {
    json j;
    j["actualAimX"] = s.actualAimM.x;
    j["actualAimY"] = s.actualAimM.y;
    j["caught"] = s.caught;
    j["contactTimeS"] = s.contactTimeS;
    j["crossedGoalLine"] = s.crossedGoalLine;
    j["crossingSpeedMs"] = s.crossingSpeedMs;
    j["crossingTimeS"] = s.crossingTimeS;
    j["flightTimeS"] = s.flightTimeS;
    j["fumbled"] = s.fumbled;
    j["intendedAimX"] = s.intendedAimM.x;
    j["intendedAimY"] = s.intendedAimM.y;
    j["keeperHeight"] = messageKey(s.keeperHeight);
    j["keeperSide"] = messageKey(s.keeperSide);
    j["keeperTimingErrorS"] = s.keeperTimingErrorS;
    j["keeperTouched"] = s.keeperTouched;
    j["marginXm"] = s.marginXm;
    j["marginYm"] = s.marginYm;
    j["maxSpeedMs"] = s.maxSpeedMs;
    j["outcome"] = messageKey(s.outcome);
    j["saveDistanceM"] = s.saveDistanceM;
    j["sideSpinRps"] = s.sideSpinRps;
    j["speedMs"] = s.speedMs;
    j["spinRps"] = s.topSpinRps;
    j["woodwork"] = messageKey(s.woodwork);
    j["zone"] = messageKey(s.zone);
    return jsonToString(j);
}

std::string toJson(const KickRecord& k) {
    json j;
    j["keeperClubId"] = k.keeperClubId;
    j["playerRole"] = messageKey(k.playerRole);
    j["resolution"] = toJson(k.shot);
    j["round"] = k.round;
    j["scored"] = k.scored();
    j["sequence"] = k.sequence;
    j["shooterClubId"] = k.shooterClubId;
    j["side"] = messageKey(k.side);
    j["suddenDeath"] = k.suddenDeath;
    return jsonToString(j);
}

std::vector<std::string> toJsonList(const std::vector<KickRecord>& kicks) {
    std::vector<std::string> out;
    out.reserve(kicks.size());
    for (const auto& k : kicks) out.push_back(toJson(k));
    return out;
}

std::string toJson(const MatchSetup& m) {
    json j;
    j["fixture"] = {{"away", m.fixture.away}, {"home", m.fixture.home},
                    {"matchIndex", m.fixture.matchIndex}, {"round", m.fixture.round}};
    j["isDecisive"] = m.isDecisive;
    j["leagueLabel"] = m.leagueLabel;
    j["opponent"] = m.opponent.name;
    j["opponentShort"] = m.opponent.shortName;
    j["playerClub"] = m.playerClub.name;
    j["playerShootsFirst"] = messageKey(m.playerShootsFirst);
    j["round"] = m.round;
    j["rules"] = {{"firstKicker", messageKey(m.rules.firstKicker)},
                  {"kicksPerSide", m.rules.kicksPerSide},
                  {"maxSuddenDeathRounds", m.rules.maxSuddenDeathRounds},
                  {"suddenDeath", m.rules.suddenDeath}};
    j["seasonNumber"] = m.seasonNumber;
    return jsonToString(j);
}

std::string toJson(const SeasonSummary& s) {
    json j;
    json arr = json::array();
    for (const auto& r : s.finalTable) arr.push_back(json::parse(toJson(r)));
    j["finalTable"] = arr;
    json promoted = json::array();
    for (auto v : s.promotedClubs) promoted.push_back(v);
    j["promotedClubs"] = promoted;
    json relegated = json::array();
    for (auto v : s.relegatedClubs) relegated.push_back(v);
    j["relegatedClubs"] = relegated;
    j["newAchievements"] = s.newAchievements;
    j["nextTierIndex"] = s.nextTierIndex;
    j["playerFate"] = League::messageKey(s.playerFate);
    j["record"] = json::parse(toJson(s.record));
    j["seasonAdvanced"] = s.seasonAdvanced;
    j["xpGained"] = s.xpGained;
    return jsonToString(j);
}

std::string toJson(const SeasonRecord& r) {
    json j;
    j["champion"] = r.champion;
    j["goalsAgainst"] = r.goalsAgainst;
    j["goalsFor"] = r.goalsFor;
    j["leagueLabel"] = r.leagueLabel;
    j["losses"] = r.losses;
    j["position"] = r.position;
    j["promoted"] = r.promoted;
    j["relegated"] = r.relegated;
    j["seasonNumber"] = r.seasonNumber;
    j["tierIndex"] = r.tierIndex;
    j["wins"] = r.wins;
    return jsonToString(j);
}

std::string toJson(const ShootoutState& s) {
    json j;
    j["homeScore"] = s.homeScore;
    j["homeTaken"] = s.homeTaken;
    json kicks = json::array();
    for (const auto& k : s.kicks) kicks.push_back(json::parse(toJson(k)));
    j["kicks"] = kicks;
    j["suddenDeathRounds"] = s.suddenDeathRounds;
    j["winner"] = s.winner.has_value() ? std::string(messageKey(s.winner.value())) : "";
    return jsonToString(j);
}

// ----------------- Zapis pełnego stanu kariery ----------------------------

std::string toJson(const CareerState& s) {
    json j;
    j["attributes"] = json::parse(toJson(s.attributes));
    j["cosmetics"] = json::array();
    for (const auto& c : s.cosmetics) j["cosmetics"].push_back(json::parse(toJson(c)));
    j["equippedBall"] = s.equippedBall;
    j["equippedGloves"] = s.equippedGloves;
    j["equippedKit"] = s.equippedKit;
    j["equippedNet"] = s.equippedNet;
    j["history"] = json::array();
    for (const auto& h : s.history) j["history"].push_back(json::parse(toJson(h)));
    j["homeCity"] = s.homeCity;
    j["homeOsmId"] = s.homeOsmId;
    j["homeVoivodeship"] = s.homeVoivodeship;
    j["language"] = s.language == Lang::Pl ? "pl" : "en";
    j["league"] = json::object();
    j["league"]["clubs"] = json::array();
    for (const auto& c : s.league.clubs) j["league"]["clubs"].push_back(json::parse(toJson(c)));
    j["league"]["fixtures"] = json::array();
    for (const auto& f : s.league.fixtures) j["league"]["fixtures"].push_back(json::parse(toJson(f)));
    j["league"]["playerClubIndex"] = s.league.playerClubIndex;
    j["league"]["results"] = json::array();
    for (const auto& r : s.league.results) j["league"]["results"].push_back(json::parse(toJson(r)));
    j["league"]["seasonSeed"] = static_cast<i64>(s.league.seasonSeed);
    j["league"]["tierIndex"] = s.league.tierIndex;
    j["nickname"] = s.nickname;
    j["phase"] = messageKey(s.phase);
    j["progression"] = json::parse(toJson(s.progression));
    j["rules"] = {{"levelXpBase", s.rules.levelXpBase}, {"levelXpGrowth", s.rules.levelXpGrowth},
                  {"maxLevel", s.rules.maxLevel}, {"skillPointsPerLevel", s.rules.skillPointsPerLevel},
                  {"xpBasePerMatch", s.rules.xpBasePerMatch}, {"xpBonusWin", s.rules.xpBonusWin},
                  {"xpChampionship", s.rules.xpChampionship}, {"xpForLossConsolation", s.rules.xpForLossConsolation},
                  {"xpPerCleanShootout", s.rules.xpPerCleanShootout}, {"xpPerGoal", s.rules.xpPerGoal},
                  {"xpPerSave", s.rules.xpPerSave}, {"xpPromotion", s.rules.xpPromotion}};
    j["schemaVersion"] = s.schemaVersion;
    j["seed"] = static_cast<i64>(s.seed);
    j["stats"] = json::parse(toJson(s.stats));
    j["tierIndex"] = s.tierIndex;
    j["unlockedAchievements"] = s.unlockedAchievements;
    j["seasonNumber"] = s.seasonNumber;
    return jsonToString(j);
}

std::string saveToJson(const CareerState& s, i64 savedAtEpochMs) {
    json j = json::parse(toJson(s));
    j["savedAtEpochMs"] = savedAtEpochMs;
    return jsonToString(j);
}

// ----------------- Dekodery -----------------------------------------------

Result<PlayerAttributes> parseAttributes(const json& j) {
    PlayerAttributes a;
    if (!j.is_object()) return unexpected(makeError(errc::kBadJson, "attributes: not object"));
    for (auto& [k, v] : j.items()) {
        const auto opt = attrFromKey(k);
        if (!opt.has_value()) return unexpected(makeError(errc::kBadJson, "unknown attribute key: " + k));
        const auto i = asI32(v);
        if (!i.has_value()) return unexpected(makeError(errc::kBadJson, "attribute value: " + k));
        a.set(opt.value(), *i);
    }
    return a;
}

Result<Progression> parseProgression(const json& j) {
    Progression p;
    auto get = [&](const char* k, i32& dst) -> Result<void> {
        if (!j.contains(k)) return unexpected(makeError(errc::kBadJson, std::string("missing ") + k));
        const auto v = asI32(j[k]);
        if (!v.has_value()) return unexpected(makeError(errc::kBadJson, std::string("bad ") + k));
        dst = *v;
        return {};
    };
    auto err = get("level", p.level);
    if (!err) return unexpected(makeError(err.error().code, err.error().message));
    err = get("xp", p.xp);
    if (!err) return unexpected(makeError(err.error().code, err.error().message));
    err = get("skillPoints", p.skillPoints);
    if (!err) return unexpected(makeError(err.error().code, err.error().message));
    err = get("totalXp", p.totalXp);
    if (!err) return unexpected(makeError(err.error().code, err.error().message));
    return p;
}

Result<CareerStats> parseStats(const json& j) {
    CareerStats s;
    for (auto& [k, v] : j.items()) {
        const auto i = asI32(v);
        if (!i.has_value()) continue;
        if (k == "matches") s.matches = *i;
        else if (k == "wins") s.wins = *i;
        else if (k == "losses") s.losses = *i;
        else if (k == "shotsTaken") s.shotsTaken = *i;
        else if (k == "goalsScored") s.goalsScored = *i;
        else if (k == "shotsOnTarget") s.shotsOnTarget = *i;
        else if (k == "woodworkHits") s.woodworkHits = *i;
        else if (k == "savesAttempted") s.savesAttempted = *i;
        else if (k == "saves") s.saves = *i;
        else if (k == "goalsConceded") s.goalsConceded = *i;
        else if (k == "cleanShootouts") s.cleanShootouts = *i;
        else if (k == "suddenDeathMatches") s.suddenDeathMatches = *i;
        else if (k == "perfectShootouts") s.perfectShootouts = *i;
        else if (k == "bestWinStreak") s.bestWinStreak = *i;
        else if (k == "currentWinStreak") s.currentWinStreak = *i;
        else if (k == "promotions") s.promotions = *i;
        else if (k == "relegations") s.relegations = *i;
        else if (k == "championships") s.championships = *i;
        else if (k == "seasonsPlayed") s.seasonsPlayed = *i;
    }
    return s;
}

Result<CosmeticItem> parseCosmetic(const json& j) {
    CosmeticItem c;
    if (!j.is_object()) return unexpected(makeError(errc::kBadJson, "cosmetic: not object"));
    if (j.contains("id")) c.id = j["id"].get<std::string>();
    if (j.contains("nameKey")) c.nameKey = j["nameKey"].get<std::string>();
    if (j.contains("kind")) {
        const auto k = cosmFromKey(j["kind"].get<std::string>());
        if (!k.has_value()) return unexpected(makeError(errc::kBadJson, "unknown cosmetic kind"));
        c.kind = *k;
    }
    if (j.contains("defaultItem")) {
        const auto bo = asBool(j["defaultItem"]);
        if (bo.has_value()) c.defaultItem = *bo;
    }
    if (j.contains("unlocked")) {
        const auto bo = asBool(j["unlocked"]);
        if (bo.has_value()) c.unlocked = *bo;
    }
    if (j.contains("colorPrimary")) {
        const auto u = asU64(j["colorPrimary"]);
        if (u.has_value()) c.colorPrimary = static_cast<u32>(*u);
    }
    if (j.contains("colorSecondary")) {
        const auto u = asU64(j["colorSecondary"]);
        if (u.has_value()) c.colorSecondary = static_cast<u32>(*u);
    }
    if (j.contains("unlockRequirementKey")) {
        c.unlockRequirementKey = j["unlockRequirementKey"].get<std::string>();
    }
    return c;
}

Result<Club> parseClub(const json& j) {
    Club c;
    if (!j.is_object()) return unexpected(makeError(errc::kBadJson, "club: not object"));
    if (j.contains("id")) {
        const auto v = asI32(j["id"]);
        if (v.has_value()) c.id = *v;
    }
    if (j.contains("name")) c.name = j["name"].get<std::string>();
    if (j.contains("shortName")) c.shortName = j["shortName"].get<std::string>();
    if (j.contains("town")) c.town = j["town"].get<std::string>();
    if (j.contains("voivodeship")) {
        const auto v = asI32(j["voivodeship"]);
        if (v.has_value() && *v >= 0 && *v <= 255) c.voivodeship = static_cast<u8>(*v);
    }
    if (j.contains("lat")) {
        const auto v = asF64(j["lat"]);
        if (v.has_value()) c.lat = *v;
    }
    if (j.contains("lon")) {
        const auto v = asF64(j["lon"]);
        if (v.has_value()) c.lon = *v;
    }
    if (j.contains("population")) {
        const auto v = asI32(j["population"]);
        if (v.has_value()) c.population = *v;
    }
    if (j.contains("placeOsmId")) {
        const auto v = asI64(j["placeOsmId"]);
        if (v.has_value()) c.placeOsmId = *v;
    }
    if (j.contains("strength")) {
        const auto v = asF64(j["strength"]);
        if (v.has_value()) c.strength = *v;
    }
    if (j.contains("foundedYear")) {
        const auto v = asI32(j["foundedYear"]);
        if (v.has_value()) c.foundedYear = *v;
    }
    if (j.contains("isPlayer")) {
        const auto v = asBool(j["isPlayer"]);
        if (v.has_value()) c.isPlayer = *v;
    }
    if (j.contains("colors") && j["colors"].is_object()) {
        const auto& cols = j["colors"];
        if (cols.contains("primary")) {
            const auto v = asU64(cols["primary"]);
            if (v.has_value()) c.colors.primary = static_cast<u32>(*v);
        }
        if (cols.contains("secondary")) {
            const auto v = asU64(cols["secondary"]);
            if (v.has_value()) c.colors.secondary = static_cast<u32>(*v);
        }
        if (cols.contains("accent")) {
            const auto v = asU64(cols["accent"]);
            if (v.has_value()) c.colors.accent = static_cast<u32>(*v);
        }
    }
    return c;
}

Result<Fixture> parseFixture(const json& j) {
    Fixture f;
    if (!j.is_object()) return unexpected(makeError(errc::kBadJson, "fixture: not object"));
    auto v32 = [&](const char* k, i32& dst) -> Result<void> {
        if (!j.contains(k)) return unexpected(makeError(errc::kBadJson, std::string("missing ") + k));
        const auto v = asI32(j[k]);
        if (!v.has_value()) return unexpected(makeError(errc::kBadJson, std::string("bad ") + k));
        dst = *v;
        return {};
    };
    auto err = v32("matchIndex", f.matchIndex);
    if (!err) return unexpected(makeError(err.error().code, err.error().message));
    err = v32("round", f.round);
    if (!err) return unexpected(makeError(err.error().code, err.error().message));
    err = v32("home", f.home);
    if (!err) return unexpected(makeError(err.error().code, err.error().message));
    err = v32("away", f.away);
    if (!err) return unexpected(makeError(err.error().code, err.error().message));
    return f;
}

Result<MatchResultRecord> parseResult(const json& j) {
    MatchResultRecord r;
    if (!j.is_object()) return unexpected(makeError(errc::kBadJson, "result: not object"));
    if (j.contains("played")) {
        const auto v = asBool(j["played"]);
        if (v.has_value()) r.played = *v;
    }
    if (j.contains("homeGoals")) {
        const auto v = asI32(j["homeGoals"]);
        if (v.has_value()) r.homeGoals = *v;
    }
    if (j.contains("awayGoals")) {
        const auto v = asI32(j["awayGoals"]);
        if (v.has_value()) r.awayGoals = *v;
    }
    if (j.contains("homeWins")) {
        const auto v = asI32(j["homeWins"]);
        if (v.has_value()) r.homeWins = *v;
    }
    if (j.contains("awayWins")) {
        const auto v = asI32(j["awayWins"]);
        if (v.has_value()) r.awayWins = *v;
    }
    if (j.contains("shootoutRounds")) {
        const auto v = asI32(j["shootoutRounds"]);
        if (v.has_value()) r.shootoutRounds = *v;
    }
    if (j.contains("playerPlayed")) {
        const auto v = asBool(j["playerPlayed"]);
        if (v.has_value()) r.playerPlayed = *v;
    }
    if (j.contains("playerWon")) {
        const auto v = asBool(j["playerWon"]);
        if (v.has_value()) r.playerWon = *v;
    }
    return r;
}

Result<SeasonRecord> parseSeasonRecord(const json& j) {
    SeasonRecord r;
    if (!j.is_object()) return unexpected(makeError(errc::kBadJson, "season record: not object"));
    if (j.contains("seasonNumber")) { const auto v = asI32(j["seasonNumber"]); if (v) r.seasonNumber = *v; }
    if (j.contains("tierIndex")) { const auto v = asI32(j["tierIndex"]); if (v) r.tierIndex = *v; }
    if (j.contains("leagueLabel")) r.leagueLabel = j["leagueLabel"].get<std::string>();
    if (j.contains("position")) { const auto v = asI32(j["position"]); if (v) r.position = *v; }
    if (j.contains("wins")) { const auto v = asI32(j["wins"]); if (v) r.wins = *v; }
    if (j.contains("losses")) { const auto v = asI32(j["losses"]); if (v) r.losses = *v; }
    if (j.contains("goalsFor")) { const auto v = asI32(j["goalsFor"]); if (v) r.goalsFor = *v; }
    if (j.contains("goalsAgainst")) { const auto v = asI32(j["goalsAgainst"]); if (v) r.goalsAgainst = *v; }
    if (j.contains("champion")) { const auto v = asBool(j["champion"]); if (v) r.champion = *v; }
    if (j.contains("promoted")) { const auto v = asBool(j["promoted"]); if (v) r.promoted = *v; }
    if (j.contains("relegated")) { const auto v = asBool(j["relegated"]); if (v) r.relegated = *v; }
    return r;
}

Result<CareerState> careerFromJson(std::string_view sv) {
    json j;
    try { j = json::parse(sv); }
    catch (const std::exception& e) { return unexpected(makeError(errc::kBadJson, std::string("parse: ") + e.what())); }
    if (!j.is_object()) return unexpected(makeError(errc::kBadJson, "root not object"));
    CareerState s;
    if (j.contains("schemaVersion")) {
        const auto v = asI32(j["schemaVersion"]);
        if (!v.has_value() || *v > CareerState::kSchemaVersion) {
            return unexpected(makeError(errc::kSchemaVersion, "save schema too new"));
        }
        s.schemaVersion = *v;
    }
    if (j.contains("seed")) {
        const auto v = asU64(j["seed"]);
        if (v.has_value()) s.seed = *v;
    }
    if (j.contains("language")) {
        const auto code = j["language"].get<std::string>();
        if (code == "pl") s.language = Lang::Pl;
        else if (code == "en") s.language = Lang::En;
    }
    if (j.contains("nickname")) s.nickname = j["nickname"].get<std::string>();
    if (j.contains("homeOsmId")) {
        const auto v = asI64(j["homeOsmId"]);
        if (v.has_value()) s.homeOsmId = *v;
    }
    if (j.contains("homeCity")) s.homeCity = j["homeCity"].get<std::string>();
    if (j.contains("homeVoivodeship")) {
        const auto v = asI32(j["homeVoivodeship"]);
        if (v.has_value() && *v >= 0 && *v <= 255) s.homeVoivodeship = static_cast<u8>(*v);
    }
    if (j.contains("tierIndex")) {
        const auto v = asI32(j["tierIndex"]);
        if (v.has_value()) s.tierIndex = *v;
    }
    if (j.contains("seasonNumber")) {
        const auto v = asI32(j["seasonNumber"]);
        if (v.has_value()) s.seasonNumber = *v;
    }
    if (j.contains("phase")) {
        const std::string k = j["phase"].get<std::string>();
        if      (k == "career.no_career") s.phase = CareerPhase::NoCareer;
        else if (k == "career.before_match") s.phase = CareerPhase::BeforeMatch;
        else if (k == "career.match_in_progress") s.phase = CareerPhase::MatchInProgress;
        else if (k == "career.after_match") s.phase = CareerPhase::AfterMatch;
        else if (k == "career.season_summary") s.phase = CareerPhase::SeasonSummary;
    }
    if (j.contains("savedAtEpochMs")) {
        const auto v = asI64(j["savedAtEpochMs"]);
        if (v.has_value()) s.savedAtEpochMs = *v;
    }
    if (j.contains("equippedKit")) s.equippedKit = j["equippedKit"].get<std::string>();
    if (j.contains("equippedBall")) s.equippedBall = j["equippedBall"].get<std::string>();
    if (j.contains("equippedGloves")) s.equippedGloves = j["equippedGloves"].get<std::string>();
    if (j.contains("equippedNet")) s.equippedNet = j["equippedNet"].get<std::string>();
    if (j.contains("attributes")) {
        auto r = parseAttributes(j["attributes"]);
        if (!r) return unexpected(makeError(r.error().code, r.error().message));
        s.attributes = r.value();
    }
    if (j.contains("progression")) {
        auto r = parseProgression(j["progression"]);
        if (!r) return unexpected(makeError(r.error().code, r.error().message));
        s.progression = r.value();
    }
    if (j.contains("stats")) {
        auto r = parseStats(j["stats"]);
        if (!r) return unexpected(makeError(r.error().code, r.error().message));
        s.stats = r.value();
    }
    if (j.contains("rules") && j["rules"].is_object()) {
        auto& R = j["rules"];
        auto set32 = [&](const char* k, i32& dst) {
            if (!R.contains(k)) return;
            const auto v = asI32(R[k]);
            if (v.has_value()) dst = *v;
        };
        auto setF = [&](const char* k, f64& dst) {
            if (!R.contains(k)) return;
            const auto v = asF64(R[k]);
            if (v.has_value()) dst = *v;
        };
        set32("levelXpBase", s.rules.levelXpBase);
        setF ("levelXpGrowth", s.rules.levelXpGrowth);
        set32("maxLevel", s.rules.maxLevel);
        set32("skillPointsPerLevel", s.rules.skillPointsPerLevel);
        set32("xpBasePerMatch", s.rules.xpBasePerMatch);
        set32("xpBonusWin", s.rules.xpBonusWin);
        set32("xpChampionship", s.rules.xpChampionship);
        set32("xpForLossConsolation", s.rules.xpForLossConsolation);
        set32("xpPerCleanShootout", s.rules.xpPerCleanShootout);
        set32("xpPerGoal", s.rules.xpPerGoal);
        set32("xpPerSave", s.rules.xpPerSave);
        set32("xpPromotion", s.rules.xpPromotion);
    }
    if (j.contains("cosmetics") && j["cosmetics"].is_array()) {
        s.cosmetics.clear();
        for (const auto& ci : j["cosmetics"]) {
            auto r = parseCosmetic(ci);
            if (!r) return unexpected(makeError(r.error().code, r.error().message));
            s.cosmetics.push_back(r.value());
        }
    }
    if (j.contains("history") && j["history"].is_array()) {
        s.history.clear();
        for (const auto& h : j["history"]) {
            auto r = parseSeasonRecord(h);
            if (!r) return unexpected(makeError(r.error().code, r.error().message));
            s.history.push_back(r.value());
        }
    }
    if (j.contains("unlockedAchievements") && j["unlockedAchievements"].is_array()) {
        s.unlockedAchievements.clear();
        for (const auto& a : j["unlockedAchievements"]) {
            s.unlockedAchievements.push_back(a.get<std::string>());
        }
    }
    if (j.contains("league") && j["league"].is_object()) {
        auto& L = j["league"];
        if (L.contains("tierIndex")) {
            const auto v = asI32(L["tierIndex"]);
            if (v.has_value()) s.league.tierIndex = *v;
        }
        if (L.contains("playerClubIndex")) {
            const auto v = asI32(L["playerClubIndex"]);
            if (v.has_value()) s.league.playerClubIndex = *v;
        }
        if (L.contains("seasonSeed")) {
            const auto v = asU64(L["seasonSeed"]);
            if (v.has_value()) s.league.seasonSeed = *v;
        }
        if (L.contains("clubs") && L["clubs"].is_array()) {
            s.league.clubs.clear();
            for (const auto& cj : L["clubs"]) {
                auto r = parseClub(cj);
                if (!r) return unexpected(makeError(r.error().code, r.error().message));
                s.league.clubs.push_back(r.value());
            }
        }
        if (L.contains("fixtures") && L["fixtures"].is_array()) {
            s.league.fixtures.clear();
            for (const auto& fj : L["fixtures"]) {
                auto r = parseFixture(fj);
                if (!r) return unexpected(makeError(r.error().code, r.error().message));
                s.league.fixtures.push_back(r.value());
            }
        }
        if (L.contains("results") && L["results"].is_array()) {
            s.league.results.clear();
            for (const auto& rj : L["results"]) {
                auto r = parseResult(rj);
                if (!r) return unexpected(makeError(r.error().code, r.error().message));
                s.league.results.push_back(r.value());
            }
        }
    }
    return migrate(s);
}

Result<CareerState> loadFromJson(std::string_view sv) { return careerFromJson(sv); }

Result<CareerState> migrate(CareerState state) {
    // Aktualnie brak migracji — `schemaVersion == 1`. Zostawiamy na przyszłość.
    state.schemaVersion = CareerState::kSchemaVersion;
    return state;
}

// ----------------- Parsowanie wejścia gracza ------------------------------

Result<PlayerShotInput> parsePlayerShotInput(std::string_view sv) {
    json j;
    try { j = json::parse(sv); }
    catch (const std::exception& e) { return unexpected(makeError(errc::kBadJson, std::string("parse: ") + e.what())); }
    if (!j.is_object()) return unexpected(makeError(errc::kBadJson, "shot: not object"));
    PlayerShotInput p{};
    if (j.contains("aimX")) { const auto v = asF64(j["aimX"]); if (v) p.aimM.x = *v; }
    if (j.contains("aimY")) { const auto v = asF64(j["aimY"]); if (v) p.aimM.y = *v; }
    if (j.contains("effort")) { const auto v = asF64(j["effort"]); if (v) p.effort = *v; }
    if (j.contains("spin")) { const auto v = asF64(j["spin"]); if (v) p.spin = *v; }
    if (j.contains("loft")) { const auto v = asF64(j["loft"]); if (v) p.loft = *v; }
    return p;
}

Result<PlayerDiveInput> parsePlayerDiveInput(std::string_view sv) {
    json j;
    try { j = json::parse(sv); }
    catch (const std::exception& e) { return unexpected(makeError(errc::kBadJson, std::string("parse: ") + e.what())); }
    if (!j.is_object()) return unexpected(makeError(errc::kBadJson, "dive: not object"));
    PlayerDiveInput p{};
    if (j.contains("side")) {
        const auto s = sideFromKey(j["side"].get<std::string>());
        if (!s.has_value()) return unexpected(makeError(errc::kBadJson, "unknown dive side"));
        p.side = *s;
    }
    if (j.contains("height")) {
        const auto h = heightFromKey(j["height"].get<std::string>());
        if (!h.has_value()) return unexpected(makeError(errc::kBadJson, "unknown dive height"));
        p.height = *h;
    }
    if (j.contains("timingErrorS")) {
        const auto v = asF64(j["timingErrorS"]);
        if (v.has_value()) p.timingErrorS = *v;
    }
    if (j.contains("commit")) {
        const auto b = asBool(j["commit"]);
        if (b.has_value()) p.commit = *b;
    }
    return p;
}

Result<NewCareerParams> parseNewCareerParams(std::string_view sv) {
    json j;
    try { j = json::parse(sv); }
    catch (const std::exception& e) { return unexpected(makeError(errc::kBadJson, std::string("parse: ") + e.what())); }
    if (!j.is_object()) return unexpected(makeError(errc::kBadJson, "newCareer: not object"));
    NewCareerParams p{};
    if (j.contains("nickname")) p.nickname = j["nickname"].get<std::string>();
    if (j.contains("homeOsmId")) {
        const auto v = asI64(j["homeOsmId"]);
        if (v.has_value()) p.homeOsmId = *v;
    }
    if (j.contains("seed")) {
        const auto v = asU64(j["seed"]);
        if (v.has_value()) p.seed = *v;
    }
    if (j.contains("language")) {
        const auto code = j["language"].get<std::string>();
        if (code == "pl") p.language = Lang::Pl;
        else if (code == "en") p.language = Lang::En;
    }
    if (j.contains("startingTier")) {
        const auto v = asI32(j["startingTier"]);
        if (v.has_value()) p.startingTier = *v;
    }
    if (j.contains("preferredClubName")) {
        p.preferredClubName = j["preferredClubName"].get<std::string>();
    }
    return p;
}

std::optional<AttributeKind> attributeKindFromKey(std::string_view k) {
    return attrFromKey(std::string(k));
}
const char* attributeKey(AttributeKind kind) { return attrKey(kind); }
std::optional<CosmeticKind> cosmeticKindFromKey(std::string_view k) {
    return cosmFromKey(std::string(k));
}
const char* cosmeticKey(CosmeticKind kind) { return cosmKey(kind); }
std::optional<DiveSide> diveSideFromKey(std::string_view k) {
    return sideFromKey(std::string(k));
}
std::optional<DiveHeight> diveHeightFromKey(std::string_view k) {
    return heightFromKey(std::string(k));
}

std::optional<Lang> langFromCode(std::string_view code) {
    if (code == "pl") return Lang::Pl;
    if (code == "en") return Lang::En;
    return std::nullopt;
}

}  // namespace bkh::json_io