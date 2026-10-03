// Implementacja fasady rdzenia: jedyne wejście dla mostka JNI (Android) i dla
// testów integracyjnych (host). Zob. core/include/bkh/facade.h oraz
// docs/PROTOCOL.md dla kształtu protokołu.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "bkh/facade.h"

#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <sstream>
#include <unordered_map>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nlohmann/json.hpp>
#pragma GCC diagnostic pop

#include "bkh/json_io.h"

namespace bkh {

namespace {

using njson = nlohmann::ordered_json;

std::string dump(const njson& j) { return j.dump(); }

std::optional<i64> asI64(const njson& j) {
    if (j.is_number_integer()) return j.get<i64>();
    if (j.is_number_float())   return static_cast<i64>(j.get<f64>());
    if (j.is_string()) {
        const auto s = j.get<std::string>();
        char* end = nullptr;
        const long long v = std::strtoll(s.c_str(), &end, 10);
        if (end == s.c_str() + s.size()) return static_cast<i64>(v);
    }
    return std::nullopt;
}
std::optional<i32> asI32(const njson& j) {
    const auto v = asI64(j);
    if (!v.has_value()) return std::nullopt;
    if (*v < INT32_MIN || *v > INT32_MAX) return std::nullopt;
    return static_cast<i32>(*v);
}
std::optional<u64> asU64(const njson& j) {
    const auto v = asI64(j);
    if (!v.has_value() || *v < 0) return std::nullopt;
    return static_cast<u64>(*v);
}
std::optional<f64> asF64(const njson& j) {
    if (j.is_number()) return j.get<f64>();
    return std::nullopt;
}
std::optional<bool> asBool(const njson& j) {
    if (j.is_boolean()) return j.get<bool>();
    if (j.is_number_integer()) return j.get<i64>() != 0;
    return std::nullopt;
}

njson okPayload(njson data) {
    return njson{{"ok", true}, {"data", std::move(data)}};
}
njson errPayload(std::string_view code, std::string message) {
    return njson{{"ok", false}, {"error", {{"code", std::string(code)}, {"message", std::move(message)}}}};
}
njson errPayload(const Error& e) {
    return njson{{"ok", false}, {"error", {{"code", e.code}, {"message", e.message}}}};
}

DiveSide sideFromKey(const std::string& k) {
    if (k == "Left")   return DiveSide::Left;
    if (k == "Right")  return DiveSide::Right;
    return DiveSide::Center;
}
DiveHeight heightFromKey(const std::string& k) {
    if (k == "Low")  return DiveHeight::Low;
    if (k == "High") return DiveHeight::High;
    return DiveHeight::Mid;
}
Side sideFromKeyS(const std::string& k) {
    if (k == "side.home") return Side::Home;
    if (k == "side.away") return Side::Away;
    return Side::Home;
}

}  // namespace

struct Facade::Impl {
    std::unique_ptr<PlaceCatalog> catalog;
    std::unique_ptr<Career> career;
    std::unique_ptr<Shootout> activeShootout;
    std::unique_ptr<MatchSetup> pendingSetup;
    bool placesLoaded = false;
    u64 frameEpoch = 0;
    Lang lang = Lang::Pl;
    // Ostatnia animacja (ostatni strzał z bieżącego meczu gracza).
    ShotPlayback lastPlayback;
    bool hasPlayback = false;
    std::atomic<u64> monotonicId{0};

    Result<njson> dispatch(std::string_view cmd, const njson& args);
};

namespace {

// --------- budowanie JSON odpowiedzi z typów rdzenia ----------------------

njson shootoutStateJson(const ShootoutState& s) {
    njson arr = njson::array();
    for (const auto& k : s.kicks) {
        njson kick;
        kick["sequence"] = k.sequence;
        kick["outcome"]   = messageKey(k.shot.outcome);
        kick["playerRole"]= messageKey(k.playerRole);
        kick["round"]     = k.round;
        kick["scored"]    = k.scored();
        kick["side"]      = messageKey(k.side);
        kick["suddenDeath"]= k.suddenDeath;
        kick["keeperSide"]= messageKey(k.shot.keeperSide);
        kick["keeperHeight"]= messageKey(k.shot.keeperHeight);
        arr.push_back(std::move(kick));
    }
    njson out;
    out["awayScore"] = s.awayScore;
    out["awayTaken"] = s.awayTaken;
    out["decidedByTieBreaker"] = s.decidedByTieBreaker;
    out["decidedEarly"] = s.decidedEarly;
    out["homeScore"] = s.homeScore;
    out["homeTaken"] = s.homeTaken;
    out["kicks"] = arr;
    out["suddenDeathRounds"] = s.suddenDeathRounds;
    if (s.winner.has_value()) out["winner"] = messageKey(*s.winner);
    else                       out["winner"] = nullptr;
    return out;
}

njson leagueTableJson(const std::vector<LeagueRow>& rows) {
    njson arr = njson::array();
    for (const auto& r : rows) {
        njson j;
        j["clubIndex"] = r.clubIndex;
        j["goalDifference"] = r.goalDifference();
        j["goalsAgainst"] = r.goalsAgainst;
        j["goalsFor"] = r.goalsFor;
        j["losses"] = r.losses;
        j["played"] = r.played;
        j["points"] = r.points;
        j["position"] = r.position;
        j["wins"] = r.wins;
        arr.push_back(std::move(j));
    }
    return arr;
}

}  // namespace

// =========================================================================
// Facade — implementacja
// =========================================================================

Facade::Facade() : impl_(std::make_unique<Impl>()) {}
Facade::~Facade() = default;
Facade::Facade(Facade&&) noexcept = default;
Facade& Facade::operator=(Facade&&) noexcept = default;

const char* Facade::coreVersion() {
    return "bkh-core/" BKH_VERSION_STRING;
}
i32 Facade::saveSchemaVersion() { return CareerState::kSchemaVersion; }
bool Facade::placesLoaded() const { return impl_->placesLoaded; }
std::size_t Facade::placesCount() const { return impl_->catalog ? impl_->catalog->places().size() : 0; }
bool Facade::hasCareer() const { return static_cast<bool>(impl_->career); }
const PlaceCatalog* Facade::catalog() const { return impl_->catalog.get(); }
const Career* Facade::career() const { return impl_->career.get(); }
const Shootout* Facade::activeShootout() const { return impl_->activeShootout.get(); }
Lang Facade::language() const { return impl_->lang; }
void Facade::setLanguage(Lang lang) { impl_->lang = lang; }

bool Facade::loadPlaces(const char* data, std::size_t size, std::string& errorOut) {
    std::string_view sv(data, size);
    PlaceParseReport rpt;
    auto cat = std::make_unique<PlaceCatalog>(PlaceCatalog::parseCsv(sv, rpt));
    if (rpt.rowsAccepted == 0 && rpt.rowsSeen > 0) {
        std::ostringstream os;
        os << "nie udało się zaimportować żadnej miejscowości (odrzuconych: " << rpt.rowsRejected << ")";
        errorOut = os.str();
        return false;
    }
    if (rpt.duplicateIds > 0) {
        // Tylko ostrzeżenie — katalog i tak zostaje użyty.
    }
    impl_->catalog = std::move(cat);
    impl_->placesLoaded = true;
    errorOut.clear();
    return true;
}

std::string Facade::saveCareerToJson(i64 savedAtEpochMs) const {
    if (!impl_->career) return {};
    return json_io::saveToJson(impl_->career->state(), savedAtEpochMs);
}

bool Facade::loadCareerFromJson(std::string_view json, std::string& errorOut) {
    auto r = json_io::loadFromJson(json);
    if (!r) {
        errorOut = r.error().message;
        return false;
    }
    impl_->career = std::make_unique<Career>();
    impl_->career->setState(r.value());
    return true;
}

bool Facade::hasPlayback() const { return impl_->hasPlayback; }
int Facade::frameFloatCount() const {
    if (!impl_->hasPlayback) return 0;
    const auto& pb = impl_->lastPlayback;
    return static_cast<int>(frame::requiredFloats(pb.ball.size(), pb.keeper.size()));
}

void Facade::clearPlayback() {
    impl_->hasPlayback = false;
    impl_->lastPlayback = ShotPlayback{};
}

int Facade::writeFrame(float* dest, int capacity) const {
    if (!impl_->hasPlayback || !dest) return 0;
    const auto& pb = impl_->lastPlayback;
    const std::size_t need = frame::requiredFloats(pb.ball.size(), pb.keeper.size());
    if (static_cast<std::size_t>(capacity) < need) {
        // Sygnalizujemy potrzebę: Kotlin powinien powiększyć bufor.
        return -static_cast<int>(need);
    }
    std::size_t i = 0;
    auto write = [&](f64 v) {
        dest[i++] = static_cast<float>(v);
    };
    // [0] wersja układu, [1] ballSamples, [2] keeperSamples, [3] durationS,
    // [4] contactTimeS, [5] crossingTimeS, [6] impactX, [7] impactY.
    write(static_cast<f64>(frame::kLayoutVersion));
    write(static_cast<f64>(pb.ball.size()));
    write(static_cast<f64>(pb.keeper.size()));
    write(pb.durationS);
    write(pb.contactTimeS);
    write(pb.crossingTimeS);
    write(pb.impactPointM.x);
    write(pb.impactPointM.y);
    for (const auto& s : pb.ball) {
        write(s.timeS); write(s.positionM.x); write(s.positionM.y); write(s.positionM.z);
        write(s.velocityMs.x); write(s.velocityMs.y); write(s.velocityMs.z);
    }
    for (const auto& s : pb.keeper) {
        write(s.timeS);
        write(s.bodyM.x); write(s.bodyM.y); write(s.bodyM.z);
        write(s.diveProgress);
        const f64 sideF = static_cast<f64>(static_cast<int>(s.side));
        write(sideF);
    }
    return static_cast<int>(i);
}

// -------------------------------------------------------------------------
// Dispatcher
// -------------------------------------------------------------------------

Result<njson> Facade::Impl::dispatch(std::string_view cmd, const njson& args) {
    // 1. metainformacje
    if (cmd == "version") {
        njson data;
        data["coreVersion"] = BKH_VERSION_STRING;
        data["protocolVersion"] = kProtocolVersion;
        data["saveSchemaVersion"] = CareerState::kSchemaVersion;
        data["frameLayoutVersion"] = frame::kLayoutVersion;
        return okPayload(std::move(data));
    }
    if (cmd == "catalog") {
        njson items = njson::array();
        if (!catalog) return unexpected(makeError(errc::kNotFound, "catalog not loaded"));
        const i32 lim = args.value("limit", 5000);
        const auto list = catalog->largest(lim < 0 ? 0u : static_cast<std::size_t>(lim), 0);
        for (const Place* p : list) {
            njson j;
            j["name"] = p->name;
            j["osmId"] = p->osmId;
            j["place"] = placeTypeName(p->type);
            j["population"] = p->population;
            j["lat"] = p->lat;
            j["lon"] = p->lon;
            j["voivodeship"] = p->voivodeship;
            items.push_back(std::move(j));
        }
        njson out;
        out["count"] = items.size();
        out["items"] = items;
        return okPayload(std::move(out));
    }
    if (cmd == "searchCity") {
        if (!catalog) return unexpected(makeError(errc::kNotFound, "catalog not loaded"));
        const std::string q = args.value("query", "");
        const i32 lim = args.value("limit", 25);
        if (q.empty()) return unexpected(makeError(errc::kBadParam, "empty query"));
        const auto list = catalog->search(q, lim < 0 ? 0u : static_cast<std::size_t>(lim));
        njson items = njson::array();
        for (const Place* p : list) {
            njson j;
            j["name"] = p->name;
            j["osmId"] = p->osmId;
            j["place"] = placeTypeName(p->type);
            j["population"] = p->population;
            j["lat"] = p->lat;
            j["lon"] = p->lon;
            j["voivodeship"] = p->voivodeship;
            items.push_back(std::move(j));
        }
        njson out;
        out["items"] = items;
        return okPayload(std::move(out));
    }
    if (cmd == "resolveCity") {
        if (!catalog) return unexpected(makeError(errc::kNotFound, "catalog not loaded"));
        const std::string name = args.value("name", "");
        if (name.empty()) return unexpected(makeError(errc::kBadParam, "empty name"));
        const Place* exact = catalog->findByName(name);
        const Place* best = exact ? exact : catalog->bestMatch(name);
        if (!best) return unexpected(makeError(errc::kNotFound, "city not found"));
        njson j;
        j["name"] = best->name;
        j["osmId"] = best->osmId;
        j["place"] = placeTypeName(best->type);
        j["population"] = best->population;
        j["lat"] = best->lat;
        j["lon"] = best->lon;
        j["voivodeship"] = best->voivodeship;
        j["exact"] = exact != nullptr;
        return okPayload(std::move(j));
    }
    if (cmd == "setLanguage") {
        const std::string code = args.value("code", "pl");
        if (code == "pl") lang = Lang::Pl;
        else if (code == "en") lang = Lang::En;
        else return unexpected(makeError(errc::kBadParam, "unknown language: " + code));
        return okPayload(njson{{"language", code}});
    }

    // 2. Kariera — wyszukiwanie / tworzenie
    if (cmd == "newCareer") {
        NewCareerParams p{};
        p.nickname = args.value("nickname", std::string{});
        p.preferredClubName = args.value("preferredClubName", std::string{});
        p.language = lang;
        const auto idV = asI64(args.value("homeOsmId", njson(0)));
        if (!idV.has_value() || *idV <= 0) return unexpected(makeError(errc::kBadParam, "homeOsmId"));
        p.homeOsmId = *idV;
        const auto seedV = asU64(args.value("seed", njson(0)));
        if (seedV.has_value()) p.seed = *seedV;
        const auto tierV = asI32(args.value("startingTier", njson(0)));
        if (tierV.has_value()) p.startingTier = Pyramid::clampTier(*tierV);
        if (!catalog) return unexpected(makeError(errc::kNotFound, "catalog not loaded"));
        const auto result = Career::startNew(p, *catalog);
        if (!result) return unexpected(makeError(result.error().code, result.error().message));
        career = std::make_unique<Career>(std::move(result.value()));
        njson out;
        out["schemaVersion"] = CareerState::kSchemaVersion;
        out["homeOsmId"] = p.homeOsmId;
        out["homeCity"] = career->state().homeCity;
        out["homeVoivodeship"] = career->state().homeVoivodeship;
        out["tierIndex"] = career->state().tierIndex;
        out["leagueLabel"] = Pyramid::leagueLabel(career->state().tierIndex, career->state().homeVoivodeship, lang);
        out["seed"] = static_cast<i64>(career->state().seed);
        out["seasonNumber"] = career->state().seasonNumber;
        out["phase"] = messageKey(career->state().phase);
        return okPayload(std::move(out));
    }

    if (!career) return unexpected(makeError(errc::kNoCareer, "career not started"));

    if (cmd == "career") {
        const auto& s = career->state();
        njson out;
        out["schemaVersion"] = s.schemaVersion;
        out["homeOsmId"] = s.homeOsmId;
        out["homeCity"] = s.homeCity;
        out["homeVoivodeship"] = s.homeVoivodeship;
        out["language"] = lang == Lang::Pl ? "pl" : "en";
        out["nickname"] = s.nickname;
        out["tierIndex"] = s.tierIndex;
        out["seasonNumber"] = s.seasonNumber;
        out["phase"] = messageKey(s.phase);
        out["seed"] = static_cast<i64>(s.seed);
        out["level"] = s.progression.level;
        out["xp"] = s.progression.xp;
        out["skillPoints"] = s.progression.skillPoints;
        out["xpToNext"] = career->xpToNextLevel();
        out["matches"] = s.stats.matches;
        out["wins"] = s.stats.wins;
        out["losses"] = s.stats.losses;
        out["goalsScored"] = s.stats.goalsScored;
        out["saves"] = s.stats.saves;
        out["cleanShootouts"] = s.stats.cleanShootouts;
        out["promotions"] = s.stats.promotions;
        out["relegations"] = s.stats.relegations;
        out["championships"] = s.stats.championships;
        out["leagueLabel"] = Pyramid::leagueLabel(s.tierIndex, s.homeVoivodeship, lang);
        return okPayload(std::move(out));
    }
    if (cmd == "table") {
        League lg = career->makeLeague();
        const auto& s = career->state();
        njson out;
        out["tierIndex"] = s.tierIndex;
        out["leagueLabel"] = Pyramid::leagueLabel(s.tierIndex, s.homeVoivodeship, lang);
        out["playerClubIndex"] = lg.playerClubIndex();
        out["seasonNumber"] = s.seasonNumber;
        out["round"] = lg.nextIncompleteRound();
        out["matchesPlayed"] = lg.playedMatches();
        out["totalRounds"] = lg.roundCount();
        njson arr = njson::array();
        for (const auto& row : lg.table()) {
            njson r;
            const Club& c = lg.clubs()[row.clubIndex];
            r["position"] = row.position;
            r["clubIndex"] = row.clubIndex;
            r["name"] = c.name;
            r["shortName"] = c.shortName;
            // Miejscowość, z której pochodzi klub — UI pokazuje ją obok skrótu,
            // żeby było widać, że liga składa się z okolicznych miejscowości.
            r["town"] = c.town;
            r["isPlayer"] = c.isPlayer;
            r["played"] = row.played;
            r["wins"] = row.wins;
            r["losses"] = row.losses;
            r["goalsFor"] = row.goalsFor;
            r["goalsAgainst"] = row.goalsAgainst;
            r["goalDifference"] = row.goalDifference();
            r["points"] = row.points;
            arr.push_back(std::move(r));
        }
        out["table"] = arr;
        return okPayload(std::move(out));
    }
    if (cmd == "fixtures") {
        League lg = career->makeLeague();
        const auto& fixtures = lg.fixtures();
        njson arr = njson::array();
        for (const auto& f : fixtures) {
            njson j;
            j["matchIndex"] = f.matchIndex;
            j["round"] = f.round;
            j["home"] = lg.clubs()[f.home].name;
            j["away"] = lg.clubs()[f.away].name;
            j["homeShort"] = lg.clubs()[f.home].shortName;
            j["awayShort"] = lg.clubs()[f.away].shortName;
            j["homeTown"] = lg.clubs()[f.home].town;
            j["awayTown"] = lg.clubs()[f.away].town;
            j["played"] = lg.result(f.matchIndex).played;
            j["homeGoals"] = lg.result(f.matchIndex).homeGoals;
            j["awayGoals"] = lg.result(f.matchIndex).awayGoals;
            j["playerPlayed"] = lg.result(f.matchIndex).playerPlayed;
            arr.push_back(std::move(j));
        }
        njson out;
        out["fixtures"] = arr;
        return okPayload(std::move(out));
    }
    if (cmd == "attributes") {
        const auto& a = career->state().attributes;
        njson attrs = njson::object();
        attrs[json_io::attributeKey(AttributeKind::ShotPower)]    = a.get(AttributeKind::ShotPower);
        attrs[json_io::attributeKey(AttributeKind::ShotAccuracy)] = a.get(AttributeKind::ShotAccuracy);
        attrs[json_io::attributeKey(AttributeKind::Composure)]    = a.get(AttributeKind::Composure);
        attrs[json_io::attributeKey(AttributeKind::Curve)]        = a.get(AttributeKind::Curve);
        attrs[json_io::attributeKey(AttributeKind::Reflexes)]     = a.get(AttributeKind::Reflexes);
        attrs[json_io::attributeKey(AttributeKind::Reach)]        = a.get(AttributeKind::Reach);
        attrs[json_io::attributeKey(AttributeKind::Reading)]     = a.get(AttributeKind::Reading);
        attrs[json_io::attributeKey(AttributeKind::Handling)]    = a.get(AttributeKind::Handling);
        njson out;
        out["attributes"] = attrs;
        out["skillPoints"] = career->state().progression.skillPoints;
        out["min"] = PlayerAttributes::kMin;
        out["max"] = PlayerAttributes::kMax;
        return okPayload(std::move(out));
    }
    if (cmd == "upgrade") {
        const std::string k = args.value("attribute", std::string{});
        const auto opt = json_io::attributeKindFromKey(k);
        if (!opt.has_value()) return unexpected(makeError(errc::kBadParam, "unknown attribute: " + k));
        const auto r = career->upgradeAttribute(opt.value());
        if (!r) return unexpected(makeError(r.error().code, r.error().message));
        return okPayload(njson{{"attribute", k}});
    }
    if (cmd == "cosmetics") {
        const auto& list = career->state().cosmetics;
        njson arr = njson::array();
        for (const auto& ci : list) {
            njson j;
            j["id"] = ci.id;
            j["nameKey"] = ci.nameKey;
            j["kind"] = json_io::cosmeticKey(ci.kind);
            j["unlocked"] = ci.unlocked;
            j["defaultItem"] = ci.defaultItem;
            j["colorPrimary"] = ci.colorPrimary;
            j["colorSecondary"] = ci.colorSecondary;
            j["unlockRequirementKey"] = ci.unlockRequirementKey;
            arr.push_back(std::move(j));
        }
        njson equipped = njson::object();
        equipped["Kit"]    = career->state().equippedKit;
        equipped["Ball"]   = career->state().equippedBall;
        equipped["Gloves"] = career->state().equippedGloves;
        equipped["Net"]    = career->state().equippedNet;
        njson out;
        out["cosmetics"] = arr;
        out["equipped"] = equipped;
        return okPayload(std::move(out));
    }
    if (cmd == "equip") {
        const std::string k = args.value("kind", std::string{});
        const std::string id = args.value("id", std::string{});
        const auto kopt = json_io::cosmeticKindFromKey(k);
        if (!kopt.has_value()) return unexpected(makeError(errc::kBadParam, "unknown kind: " + k));
        const auto r = career->equipCosmetic(kopt.value(), id);
        if (!r) return unexpected(makeError(r.error().code, r.error().message));
        return okPayload(njson{{"kind", k}, {"id", id}});
    }
    if (cmd == "achievements") {
        const auto& rules = achievementRules();
        const auto& unlocked = career->state().unlockedAchievements;
        std::unordered_map<std::string, bool> u;
        for (const auto& k : unlocked) u[k] = true;
        njson arr = njson::array();
        for (const auto& r : rules) {
            njson j;
            j["key"] = r.key;
            j["nameKey"] = r.nameKey;
            j["unlocked"] = u.count(r.key) > 0;
            arr.push_back(std::move(j));
        }
        return okPayload(njson{{"achievements", arr}});
    }
    if (cmd == "stats") {
        const auto& s = career->state().stats;
        njson out;
        out["matches"] = s.matches;
        out["wins"] = s.wins;
        out["losses"] = s.losses;
        out["shotsTaken"] = s.shotsTaken;
        out["goalsScored"] = s.goalsScored;
        out["shotsOnTarget"] = s.shotsOnTarget;
        out["woodworkHits"] = s.woodworkHits;
        out["savesAttempted"] = s.savesAttempted;
        out["saves"] = s.saves;
        out["goalsConceded"] = s.goalsConceded;
        out["cleanShootouts"] = s.cleanShootouts;
        out["suddenDeathMatches"] = s.suddenDeathMatches;
        out["perfectShootouts"] = s.perfectShootouts;
        out["bestWinStreak"] = s.bestWinStreak;
        out["currentWinStreak"] = s.currentWinStreak;
        out["promotions"] = s.promotions;
        out["relegations"] = s.relegations;
        out["championships"] = s.championships;
        out["shotConversion"] = s.shotConversion();
        out["saveRate"] = s.saveRate();
        return okPayload(std::move(out));
    }

    // 3. Mecz
    if (cmd == "beginMatch") {
        const u64 s = career->state().seed ^ (0xA5A5A5A5ULL + career->state().seasonNumber * 0x9E3779B97F4A7C15ULL);
        Random rng(s);
        const auto r = career->beginMatch(rng);
        if (!r) return unexpected(makeError(r.error().code, r.error().message));
        pendingSetup = std::make_unique<MatchSetup>(r.value());
        // Inicjalizacja Shootout.
        ShootoutRules rules = r.value().rules;
        rules.firstKicker = r.value().playerShootsFirst;
        Shootout s0(rules);
        activeShootout = std::make_unique<Shootout>(std::move(s0));
        njson out;
        out["setup"] = json_io::toJson(r.value());
        out["shooter"] = {
            {"power", r.value().playerShooter.power},
            {"accuracy", r.value().playerShooter.accuracy},
            {"composure", r.value().playerShooter.composure},
            {"curve", r.value().playerShooter.curve}
        };
        out["keeper"] = {
            {"reactionTimeS", r.value().playerKeeper.reactionTimeS},
            {"readingSkill", r.value().playerKeeper.readingSkill},
            {"composure", r.value().playerKeeper.composure},
            {"handlingSkill", r.value().playerKeeper.handlingSkill}
        };
        return okPayload(std::move(out));
    }
    if (cmd == "matchState") {
        if (!activeShootout) return unexpected(makeError(errc::kNoMatch, "no active match"));
        njson out;
        out["shootout"] = shootoutStateJson(activeShootout->state());
        out["nextKicker"] = messageKey(activeShootout->nextKicker());
        out["nextRound"] = activeShootout->nextRound();
        out["suddenDeath"] = activeShootout->inSuddenDeath();
        out["finished"] = activeShootout->isFinished();
        return okPayload(std::move(out));
    }
    if (cmd == "shoot") {
        if (!activeShootout || !pendingSetup) {
            return unexpected(makeError(errc::kNoMatch, "no active match"));
        }
        if (activeShootout->isFinished()) {
            return unexpected(makeError(errc::kBadParam, "shootout already finished"));
        }
        // Pamiętaj stan przed strzałem (do cofania w UI).
        auto prev = activeShootout->state();

        // Parsuj ShotInput.
        const auto psi = json_io::parsePlayerShotInput(args.dump());
        if (!psi) return unexpected(makeError(psi.error().code, psi.error().message));
        const ShooterProfile& profile = pendingSetup->playerShooter;
        const ShotInput input = sanitize(psi.value()).toShotInput(profile);

        // Buduj kontekst.
        ShotContext ctx{};
        ctx.shooter = profile;
        ctx.keeper = pendingSetup->cpuKeeper;
        ctx.cpuShoots = false;
        ctx.cpuKeeps = true;
        ctx.keeperHabitBias = 0.0;
        ctx.pressure = activeShootout->pressureFor(Side::Home);  // gracz = Home
        ctx.physics = PhysicsParams::competitive();
        ctx.playerDive = {};  // nieużywane przy strzelaniu
        // Wykonaj.
        Random rng(career->state().seed ^ (0xB16B00B5ULL + static_cast<u64>(activeShootout->state().kicks.size()) * 0xC2B2AE3D27D4EB4FULL));
        const ShotExecution exec = executeShot(input, ctx, rng);

        // Zarejestruj rzut.
        KickRecord kr{};
        kr.side = Side::Home;
        kr.playerRole = PlayerRole::Shooter;
        kr.round = activeShootout->state().homeTaken;
        kr.sequence = static_cast<i32>(activeShootout->state().kicks.size());
        /* kr.shotInput — implicit in kr.shot */
        kr.shot = exec.resolution;
        
        
        kr.suddenDeath = activeShootout->inSuddenDeath();
        activeShootout->recordKick(kr);
        lastPlayback = exec.playback;
        hasPlayback = true;
        (void)prev;

        // Sprawdź koniec.
        njson out;
        out["kick"] = json_io::toJson(kr);
        out["resolution"] = json_io::toJson(exec.resolution);
        out["shootout"] = shootoutStateJson(activeShootout->state());
        out["finished"] = activeShootout->isFinished();
        if (activeShootout->isFinished()) {
            if (activeShootout->state().winner.has_value()) {
                out["winner"] = messageKey(activeShootout->state().winner.value());
            }
        }
        return okPayload(std::move(out));
    }
    if (cmd == "dive") {
        if (!activeShootout || !pendingSetup) {
            return unexpected(makeError(errc::kNoMatch, "no active match"));
        }
        if (activeShootout->isFinished()) {
            return unexpected(makeError(errc::kBadParam, "shootout already finished"));
        }
        const auto pdi = json_io::parsePlayerDiveInput(args.dump());
        if (!pdi) return unexpected(makeError(pdi.error().code, pdi.error().message));
        const DiveCommand cmd2 = toDiveCommand(pdi.value());
        KeeperProfile profile = pendingSetup->playerKeeper;

        // Wykonaj jeden strzał CPU (symulacja) i interakcję z graczem.
        ShotContext ctx{};
        ctx.shooter = pendingSetup->cpuShooter;
        ctx.keeper = profile;
        ctx.cpuShoots = true;
        ctx.cpuKeeps = false;
        ctx.keeperHabitBias = 0.0;
        ctx.pressure = activeShootout->pressureFor(Side::Away);
        ctx.physics = PhysicsParams::competitive();
        ctx.playerDive = cmd2;

        Random rng(career->state().seed ^ (0xD1DEC0DEULL + static_cast<u64>(activeShootout->state().kicks.size()) * 0x94D049BB133111EBULL));
        ShotInput blank{};
        const ShotExecution exec = executeShot(blank, ctx, rng);

        KickRecord kr{};
        kr.side = Side::Away;
        kr.playerRole = PlayerRole::Keeper;
        kr.round = activeShootout->state().awayTaken;
        kr.sequence = static_cast<i32>(activeShootout->state().kicks.size());
        /* kr.shotInput — implicit in kr.shot */
        kr.shot = exec.resolution;
        
        
        kr.suddenDeath = activeShootout->inSuddenDeath();
        activeShootout->recordKick(kr);
        lastPlayback = exec.playback;
        hasPlayback = true;

        njson out;
        out["kick"] = json_io::toJson(kr);
        out["resolution"] = json_io::toJson(exec.resolution);
        out["shootout"] = shootoutStateJson(activeShootout->state());
        out["finished"] = activeShootout->isFinished();
        if (activeShootout->isFinished()) {
            if (activeShootout->state().winner.has_value()) {
                out["winner"] = messageKey(activeShootout->state().winner.value());
            }
        }
        return okPayload(std::move(out));
    }
    if (cmd == "finishMatch") {
        if (!activeShootout) return unexpected(makeError(errc::kNoMatch, "no active match"));
        if (!activeShootout->isFinished()) return unexpected(makeError(errc::kBadParam, "shootout not finished"));
        if (!pendingSetup) return unexpected(makeError(errc::kNoMatch, "no setup"));
        // Zbuduj MatchOutcome.
        MatchOutcome out;
        out.matchIndex = pendingSetup->fixture.matchIndex;
        out.shootoutRounds = activeShootout->state().suddenDeathRounds > 5
                                 ? activeShootout->state().suddenDeathRounds
                                 : 5;
        out.playerGoals = activeShootout->state().homeScore;
        out.opponentGoals = activeShootout->state().awayScore;
        out.playerWon = out.playerGoals > out.opponentGoals;
        // Proste statystyki z rzutów.
        for (const auto& k : activeShootout->state().kicks) {
            if (k.playerRole == PlayerRole::Shooter) {
                out.playerShotsTaken++;
                if (k.scored()) out.playerGoalsScored++;
                if (k.shot.woodwork != Woodwork::None) out.woodworkHits++;
            } else if (k.playerRole == PlayerRole::Keeper) {
                out.playerSavesAttempted++;
                if (k.shot.outcome == ShotOutcome::Saved) out.playerSaves++;
            }
        }
        out.cleanShootout = (out.opponentGoals == 0 && out.playerGoals > 0);
        out.perfectShootout = (out.playerGoalsScored >= 5 && out.opponentGoals == 0);
        out.kicks = activeShootout->state().kicks;

        Random rng(career->state().seed ^ 0xF1F1F1F1ULL);
        const auto r = career->applyMatchOutcome(out, rng);
        if (!r) return unexpected(makeError(r.error().code, r.error().message));
        njson reply;
        reply["outcome"] = {
            {"playerGoals", out.playerGoals},
            {"opponentGoals", out.opponentGoals},
            {"shootoutRounds", out.shootoutRounds},
            {"playerWon", out.playerWon},
            {"playerShotsTaken", out.playerShotsTaken},
            {"playerGoalsScored", out.playerGoalsScored},
            {"playerSavesAttempted", out.playerSavesAttempted},
            {"playerSaves", out.playerSaves},
            {"woodworkHits", out.woodworkHits},
            {"cleanShootout", out.cleanShootout},
            {"perfectShootout", out.perfectShootout}
        };
        reply["careerPhase"] = messageKey(career->state().phase);
        return okPayload(std::move(reply));
    }
    if (cmd == "nextRound") {
        if (career->state().phase != CareerPhase::AfterMatch) {
            // OK, ponawiamy rozpoczęcie meczu (kolejna kolejka).
        }
        if (career->state().phase == CareerPhase::SeasonSummary) {
            return unexpected(makeError(errc::kBadParam, "season finished"));
        }
        activeShootout.reset();
        pendingSetup.reset();
        return okPayload(njson{{"phase", messageKey(career->state().phase)}});
    }
    if (cmd == "seasonSummary") {
        Random rng(career->state().seed ^ 0xDEADC0DEULL);
        const auto summary = career->finishSeasonIfComplete(rng);
        if (!summary) return unexpected(makeError(summary.error().code, summary.error().message));
        return okPayload(json_io::toJson(summary.value()));
    }
    if (cmd == "history") {
        const auto& h = career->state().history;
        njson arr = njson::array();
        for (const auto& r : h) arr.push_back(njson::parse(json_io::toJson(r)));
        return okPayload(njson{{"history", arr}});
    }
    if (cmd == "acknowledgements") {
        njson data;
        data["attribution"] = "OpenStreetMap contributors, ODbL 1.0";
        data["license"] = "GPL-3.0-or-later";
        data["source"] = "https://openstreetmap.org";
        data["retrievalDate"] = "2026-09-30";
        return okPayload(std::move(data));
    }

    return unexpected(makeError(errc::kUnknownCommand, std::string("unknown command: ") + std::string(cmd)));
}

std::string Facade::command(std::string_view jsonCommand) {
    njson args;
    try {
        args = njson::parse(jsonCommand);
    } catch (const std::exception& e) {
        return dump(errPayload("error.bad_json", std::string("parse: ") + e.what()));
    }
    if (!args.is_object() || !args.contains("cmd") || !args["cmd"].is_string()) {
        return dump(errPayload("error.bad_json", "missing cmd field"));
    }
    const std::string cmd = args["cmd"].get<std::string>();
    njson rest = args;
    rest.erase("cmd");
    try {
        auto r = impl_->dispatch(cmd, rest);
        if (!r) return dump(errPayload(r.error().code, r.error().message));
        // dispatch() zwraca już kompletny envelope {"ok":true,"data":...} —
        // nie opakowujemy go drugi raz.
        return dump(std::move(r.value()));
    } catch (const std::exception& e) {
        return dump(errPayload("error.internal", std::string("exception: ") + e.what()));
    } catch (...) {
        return dump(errPayload("error.internal", "unknown exception"));
    }
}

}  // namespace bkh