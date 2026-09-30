// Implementacja kariery: atrybuty gracza, postęp XP, kosmetyka, osiągnięcia,
// budowanie sezonu i przebieg meczu. Patrz: core/include/bkh/career.h.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "bkh/career.h"

#include <algorithm>
#include <unordered_set>

namespace bkh {

// ---------------------------------------------------------------------------
// Atrybuty gracza
// ---------------------------------------------------------------------------

const char* messageKey(AttributeKind kind) {
    switch (kind) {
        case AttributeKind::ShotPower: return "attribute.shot_power";
        case AttributeKind::ShotAccuracy: return "attribute.shot_accuracy";
        case AttributeKind::Composure: return "attribute.composure";
        case AttributeKind::Curve: return "attribute.curve";
        case AttributeKind::Reflexes: return "attribute.reflexes";
        case AttributeKind::Reach: return "attribute.reach";
        case AttributeKind::Reading: return "attribute.reading";
        case AttributeKind::Handling: return "attribute.handling";
        case AttributeKind::Count: return "attribute.count";
    }
    return "attribute.unknown";
}

i32 PlayerAttributes::get(AttributeKind kind) const {
    const auto idx = static_cast<std::size_t>(kind);
    if (idx >= values.size()) return kMin;
    return values[idx];
}

void PlayerAttributes::set(AttributeKind kind, i32 value) {
    const auto idx = static_cast<std::size_t>(kind);
    if (idx >= values.size()) return;
    values[idx] = clamp(value, kMin, kMax);
}

bool PlayerAttributes::increase(AttributeKind kind) {
    const auto idx = static_cast<std::size_t>(kind);
    if (idx >= values.size()) return false;
    if (values[idx] >= kMax) return false;
    values[idx]++;
    return true;
}

ShooterProfile PlayerAttributes::toShooterProfile() const {
    const f64 power    = (get(AttributeKind::ShotPower)    - 1.0) / 19.0;
    const f64 accuracy = (get(AttributeKind::ShotAccuracy) - 1.0) / 19.0;
    const f64 composure = (get(AttributeKind::Composure)   - 1.0) / 19.0;
    const f64 curve    = (get(AttributeKind::Curve)       - 1.0) / 19.0;
    return ShooterProfile::fromAttributes(power, accuracy, composure, curve);
}

KeeperProfile PlayerAttributes::toKeeperProfile() const {
    const f64 reflexes = (get(AttributeKind::Reflexes)  - 1.0) / 19.0;
    const f64 reach    = (get(AttributeKind::Reach)     - 1.0) / 19.0;
    const f64 reading  = (get(AttributeKind::Reading)   - 1.0) / 19.0;
    const f64 composure = (get(AttributeKind::Composure) - 1.0) / 19.0;
    const f64 handling = (get(AttributeKind::Handling)  - 1.0) / 19.0;
    return KeeperProfile::fromAttributes(reflexes, reach, reading, composure, handling);
}

// ---------------------------------------------------------------------------
// Progression
// ---------------------------------------------------------------------------

i32 ProgressionRules::xpForLevel(i32 level) const {
    if (level < 1) return 0;
    f64 v = static_cast<f64>(levelXpBase);
    for (i32 i = 1; i < level; ++i) v *= levelXpGrowth;
    return static_cast<i32>(std::round(v / 10.0)) * 10;
}

i32 Career::xpToNextLevel() const {
    const auto& p = state_.progression;
    const auto& r = state_.rules;
    if (p.level >= r.maxLevel) return 0;
    return r.xpForLevel(p.level);
}

i32 Career::awardXp(i32 amount) {
    if (amount <= 0) return 0;
    auto& p = state_.progression;
    auto& r = state_.rules;
    p.totalXp += amount;
    p.xp += amount;
    i32 newLevels = 0;
    while (p.level < r.maxLevel && p.xp >= r.xpForLevel(p.level)) {
        const i32 cost = r.xpForLevel(p.level);
        p.xp -= cost;
        p.level++;
        p.skillPoints += r.skillPointsPerLevel;
        ++newLevels;
    }
    if (p.level >= r.maxLevel) {
        p.level = r.maxLevel;
        p.xp = 0;
    }
    state_.stats.seasonsPlayed = state_.stats.seasonsPlayed;  // zachowaj
    return newLevels;
}

// ---------------------------------------------------------------------------
// Kosmetyka
// ---------------------------------------------------------------------------

const char* messageKey(CosmeticKind kind) {
    switch (kind) {
        case CosmeticKind::Kit: return "cosmetic.kit";
        case CosmeticKind::Ball: return "cosmetic.ball";
        case CosmeticKind::Gloves: return "cosmetic.gloves";
        case CosmeticKind::Net: return "cosmetic.net";
        case CosmeticKind::Count: return "cosmetic.count";
    }
    return "cosmetic.unknown";
}

namespace {
constexpr struct {
    const char* id;
    CosmeticKind kind;
    bool defaultItem;
    u32 primary;
    u32 secondary;
    const char* unlockKey;
} kCosmetics[] = {
    {"kit.bklasa.green",   CosmeticKind::Kit,    true,  0xFF2E7D32, 0xFFFFFFFF, ""},
    {"kit.ekstraklasa.blue", CosmeticKind::Kit, false, 0xFF1976D2, 0xFFFFFFFF, "achieve.first_promotion"},
    {"kit.klasyk.red",     CosmeticKind::Kit,    false, 0xFFD32F2F, 0xFFFFFFFF, "achieve.champion"},
    {"kit.gornik.yellow",  CosmeticKind::Kit,    false, 0xFFFFA000, 0xFF212121, "achieve.iv_league"},
    {"kit.amber",          CosmeticKind::Kit,    false, 0xFFFFB300, 0xFF1A237E, "achieve.ekstraklasa"},
    {"ball.bialy",         CosmeticKind::Ball,   true,  0xFFFFFFFF, 0xFF212121, ""},
    {"ball.pomaranczowy",  CosmeticKind::Ball,   false, 0xFFFF8F00, 0xFFFFFFFF, "achieve.first_win"},
    {"ball.zimowy",        CosmeticKind::Ball,   false, 0xFFB0BEC5, 0xFF263238, "achieve.five_clean_sheets"},
    {"ball.zloty",         CosmeticKind::Ball,   false, 0xFFFFD600, 0xFF212121, "achieve.ekstraklasa"},
    {"gloves.klasyk",      CosmeticKind::Gloves, true,  0xFFFFEB3B, 0xFF1565C0, ""},
    {"gloves.czerwone",    CosmeticKind::Gloves, false, 0xFFD32F2F, 0xFF212121, "achieve.ten_saves"},
    {"gloves.prof",        CosmeticKind::Gloves, false, 0xFF1A237E, 0xFFFFEB3B, "achieve.perfect_shootout"},
    {"net.klasyk",         CosmeticKind::Net,    true,  0xFFFFFFFF, 0xFF424242, ""},
    {"net.zoltasieci",     CosmeticKind::Net,    false, 0xFFFFC107, 0xFF212121, "achieve.first_goal"},
};
}  // namespace

std::vector<CosmeticItem> defaultCosmetics() {
    std::vector<CosmeticItem> items;
    items.reserve(sizeof(kCosmetics) / sizeof(kCosmetics[0]));
    for (const auto& c : kCosmetics) {
        CosmeticItem ci{};
        ci.id = c.id;
        ci.kind = c.kind;
        ci.unlocked = c.defaultItem;
        ci.defaultItem = c.defaultItem;
        ci.colorPrimary = c.primary;
        ci.colorSecondary = c.secondary;
        ci.unlockRequirementKey = c.unlockKey;
        ci.nameKey = std::string("cosmetic.item.") + c.id;
        items.push_back(std::move(ci));
    }
    return items;
}

const CosmeticItem* CareerState::equipped(CosmeticKind kind) const {
    const std::string* id = nullptr;
    switch (kind) {
        case CosmeticKind::Kit:    id = &equippedKit; break;
        case CosmeticKind::Ball:   id = &equippedBall; break;
        case CosmeticKind::Gloves: id = &equippedGloves; break;
        case CosmeticKind::Net:    id = &equippedNet; break;
        case CosmeticKind::Count: return nullptr;
    }
    if (!id || id->empty()) return nullptr;
    return findById(*id);
}

const CosmeticItem* CareerState::findById(const std::string& id) const {
    for (const CosmeticItem& c : cosmetics) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

Result<void> Career::unlockCosmetic(const std::string& id) {
    for (auto& c : state_.cosmetics) {
        if (c.id == id) {
            if (!c.unlocked) {
                c.unlocked = true;
            }
            return {};
        }
    }
    return unexpected(makeError(errc::kNotFound, "unknown cosmetic id"));
}

Result<void> Career::equipCosmetic(CosmeticKind kind, const std::string& id) {
    std::string* slot = nullptr;
    switch (kind) {
        case CosmeticKind::Kit:    slot = &state_.equippedKit; break;
        case CosmeticKind::Ball:   slot = &state_.equippedBall; break;
        case CosmeticKind::Gloves: slot = &state_.equippedGloves; break;
        case CosmeticKind::Net:    slot = &state_.equippedNet; break;
        case CosmeticKind::Count:  return unexpected(makeError(errc::kBadParam, "invalid cosmetic kind"));
    }
    const CosmeticItem* item = state_.findById(id);
    if (!item) return unexpected(makeError(errc::kNotFound, "unknown cosmetic id"));
    if (item->kind != kind) return unexpected(makeError(errc::kBadParam, "wrong cosmetic kind"));
    if (!item->unlocked) return unexpected(makeError(errc::kNotUnlocked, "cosmetic locked"));
    *slot = id;
    return {};
}

Result<void> Career::upgradeAttribute(AttributeKind kind) {
    if (state_.phase == CareerPhase::NoCareer) {
        return unexpected(makeError(errc::kNoCareer, "career not started"));
    }
    if (state_.progression.skillPoints <= 0) {
        return unexpected(makeError(errc::kNoSkillPoints, "no skill points"));
    }
    if (!state_.attributes.increase(kind)) {
        return unexpected(makeError(errc::kMaxedOut, "attribute at maximum"));
    }
    state_.progression.skillPoints--;
    return {};
}

// ---------------------------------------------------------------------------
// Fazy kariery
// ---------------------------------------------------------------------------

const char* messageKey(CareerPhase phase) {
    switch (phase) {
        case CareerPhase::NoCareer: return "career.no_career";
        case CareerPhase::BeforeMatch: return "career.before_match";
        case CareerPhase::MatchInProgress: return "career.match_in_progress";
        case CareerPhase::AfterMatch: return "career.after_match";
        case CareerPhase::SeasonSummary: return "career.season_summary";
    }
    return "career.unknown";
}

// ---------------------------------------------------------------------------
// Osiągnięcia
// ---------------------------------------------------------------------------

namespace {
bool achieveFirstGoal(const CareerState& s) { return s.stats.goalsScored >= 1; }
bool achieveFirstSave(const CareerState& s) { return s.stats.saves >= 1; }
bool achieveFirstWin(const CareerState& s) { return s.stats.wins >= 1; }
bool achieveFirstPromotion(const CareerState& s) { return s.stats.promotions >= 1; }
bool achieveChampion(const CareerState& s) { return s.stats.championships >= 1; }
bool achieveRelegation(const CareerState& s) { return s.stats.relegations >= 1; }
bool achieveFiveCleanSheets(const CareerState& s) { return s.stats.cleanShootouts >= 5; }
bool achieveTenSaves(const CareerState& s) { return s.stats.saves >= 10; }
bool achievePerfectShootout(const CareerState& s) { return s.stats.perfectShootouts >= 1; }
bool achieveIvLeague(const CareerState& s) { return s.tierIndex >= 3; }
bool achieveEkstraklasa(const CareerState& s) { return s.tierIndex >= 7; }
bool achieveTenGoals(const CareerState& s) { return s.stats.goalsScored >= 10; }
bool achieveFiftyMatches(const CareerState& s) { return s.stats.matches >= 50; }
bool achieveThreeInARow(const CareerState& s) { return s.stats.bestWinStreak >= 3; }
bool achieveFirstTierChampion(const CareerState& s) {
    return s.tierIndex == 0 && s.stats.championships >= 1;
}
bool achieveTopLeagueChampion(const CareerState& s) {
    return s.tierIndex == Pyramid::kTopTier && s.stats.championships >= 1;
}
}  // namespace

const std::vector<AchievementRule>& achievementRules() {
    static const std::vector<AchievementRule> rules = {
        {"achieve.first_goal",         "achieve.first_goal.name",        &achieveFirstGoal},
        {"achieve.first_save",         "achieve.first_save.name",        &achieveFirstSave},
        {"achieve.first_win",          "achieve.first_win.name",         &achieveFirstWin},
        {"achieve.first_promotion",    "achieve.first_promotion.name",   &achieveFirstPromotion},
        {"achieve.champion",           "achieve.champion.name",          &achieveChampion},
        {"achieve.relegation",         "achieve.relegation.name",        &achieveRelegation},
        {"achieve.five_clean_sheets",  "achieve.five_clean_sheets.name", &achieveFiveCleanSheets},
        {"achieve.ten_saves",          "achieve.ten_saves.name",         &achieveTenSaves},
        {"achieve.perfect_shootout",   "achieve.perfect_shootout.name",  &achievePerfectShootout},
        {"achieve.iv_league",          "achieve.iv_league.name",         &achieveIvLeague},
        {"achieve.ekstraklasa",        "achieve.ekstraklasa.name",       &achieveEkstraklasa},
        {"achieve.ten_goals",          "achieve.ten_goals.name",         &achieveTenGoals},
        {"achieve.fifty_matches",      "achieve.fifty_matches.name",     &achieveFiftyMatches},
        {"achieve.three_in_a_row",     "achieve.three_in_a_row.name",    &achieveThreeInARow},
        {"achieve.b_klasa_champion",   "achieve.b_klasa_champion.name",  &achieveFirstTierChampion},
        {"achieve.ekstraklasa_champion","achieve.ekstraklasa_champion.name",&achieveTopLeagueChampion},
    };
    return rules;
}

std::vector<std::string> Career::evaluateAchievements() {
    std::unordered_set<std::string> already(state_.unlockedAchievements.begin(),
                                            state_.unlockedAchievements.end());
    std::vector<std::string> newly;
    for (const auto& rule : achievementRules()) {
        if (already.contains(rule.key)) continue;
        if (rule.isMet(state_)) {
            state_.unlockedAchievements.push_back(rule.key);
            newly.push_back(rule.key);
            // Odblokowanie powiązanego kosmetyka.
            for (auto& ci : state_.cosmetics) {
                if (ci.unlockRequirementKey == rule.key) {
                    ci.unlocked = true;
                }
            }
        }
    }
    return newly;
}

// ---------------------------------------------------------------------------
// Budowanie sezonu
// ---------------------------------------------------------------------------

Result<void> Career::buildSeason(i32 tierIndex, i32 seasonNumber, const PlaceCatalog* catalog, Random& rng) {
    auto& s = state_;
    s.tierIndex = tierIndex;
    s.seasonNumber = seasonNumber;

    Place synth{};
    synth.name = s.homeCity.empty() ? std::string("Centrum") : s.homeCity;
    synth.osmId = s.homeOsmId;
    synth.voivodeship = s.homeVoivodeship;
    synth.type = PlaceType::City;
    synth.population = 0;
    synth.lat = 51.9189;
    synth.lon = 19.1343;

    ClubGenerator generator(*catalog);
    std::size_t playerIdx = 0;
    std::vector<Club> clubs = generator.makeLeague(synth, tierIndex,
                                                   Pyramid::tier(tierIndex).clubCount, rng, playerIdx);

    const u64 seasonSeed = s.seed ^ (0xCBF29CE484222325ULL +
                                    static_cast<u64>(seasonNumber) * 0x9E3779B97F4A7C15ULL);
    League league0(std::move(clubs), tierIndex, seasonSeed, static_cast<i32>(playerIdx));
    s.league.clubs = league0.clubs();
    s.league.fixtures = league0.fixtures();
    s.league.results.assign(league0.fixtures().size(), MatchResultRecord{});
    s.league.tierIndex = tierIndex;
    s.league.playerClubIndex = league0.playerClubIndex();
    s.league.seasonSeed = league0.seasonSeed();
    s.phase = CareerPhase::BeforeMatch;
    return {};
}

Result<Career> Career::startNew(const NewCareerParams& params, const PlaceCatalog& catalog) {
    Career c;
    auto& s = c.state_;
    s.schemaVersion = CareerState::kSchemaVersion;
    s.language = params.language;
    s.nickname = params.nickname;
    s.seed = params.seed != 0 ? params.seed
                              : fnv1a("career:" + params.nickname + ":" + std::to_string(params.homeOsmId));
    s.homeOsmId = params.homeOsmId;
    const Place* home = catalog.findByOsmId(params.homeOsmId);
    s.homeCity = home ? home->name : params.nickname;
    s.homeVoivodeship = home ? home->voivodeship : Place::kUnknownVoivodeship;

    s.tierIndex = params.startingTier;
    s.seasonNumber = 1;

    s.attributes = PlayerAttributes{};
    s.progression = Progression{};
    s.rules = ProgressionRules{};
    s.stats = CareerStats{};
    s.cosmetics = defaultCosmetics();
    for (const auto& ci : s.cosmetics) {
        if (ci.defaultItem) {
            switch (ci.kind) {
                case CosmeticKind::Kit:    if (s.equippedKit.empty()) s.equippedKit = ci.id; break;
                case CosmeticKind::Ball:   if (s.equippedBall.empty()) s.equippedBall = ci.id; break;
                case CosmeticKind::Gloves: if (s.equippedGloves.empty()) s.equippedGloves = ci.id; break;
                case CosmeticKind::Net:    if (s.equippedNet.empty()) s.equippedNet = ci.id; break;
                case CosmeticKind::Count: break;
            }
        }
    }
    s.history.clear();
    s.unlockedAchievements.clear();

    Random rng(s.seed);
    ClubGenerator gen(catalog);
    const i32 clubCount = Pyramid::tier(s.tierIndex).clubCount;
    std::size_t playerIdx = 0;
    Place synth = home ? *home : Place{};
    if (!home) {
        synth.name = s.homeCity;
        synth.osmId = s.homeOsmId;
        synth.voivodeship = s.homeVoivodeship;
        synth.type = PlaceType::City;
        synth.population = 0;
        synth.lat = 51.9189;
        synth.lon = 19.1343;
    }
    std::vector<Club> clubs = gen.makeLeague(synth, s.tierIndex, clubCount, rng, playerIdx);

    League league(std::move(clubs), s.tierIndex, s.seed ^ 0xCBF29CE484222325ULL, static_cast<i32>(playerIdx));
    s.league.clubs = league.clubs();
    s.league.fixtures = league.fixtures();
    s.league.results.assign(league.fixtures().size(), MatchResultRecord{});
    s.league.tierIndex = league.tierIndex();
    s.league.playerClubIndex = league.playerClubIndex();
    s.league.seasonSeed = league.seasonSeed();
    s.phase = CareerPhase::BeforeMatch;
    return c;
}

League Career::makeLeague() const {
    League l(state_.league.clubs, state_.league.tierIndex, state_.league.seasonSeed,
            state_.league.playerClubIndex);
    l.restore(state_.league.fixtures, state_.league.results);
    return l;
}

// ---------------------------------------------------------------------------
// Przebieg meczu
// ---------------------------------------------------------------------------

Result<MatchSetup> Career::beginMatch(Random& rng) {
    auto& s = state_;
    if (s.phase != CareerPhase::BeforeMatch && s.phase != CareerPhase::AfterMatch) {
        return unexpected(makeError(errc::kWrongPhase, "career not in BeforeMatch"));
    }
    League league = makeLeague();
    if (league.isSeasonComplete()) {
        return unexpected(makeError(errc::kNoMatch, "season already finished"));
    }
    const i32 round = league.nextIncompleteRound();
    const Fixture* fix = league.playerFixture(round);
    if (!fix) {
        return unexpected(makeError(errc::kNoMatch, "no player fixture in next round"));
    }
    // Zasymuluj pozostałe mecze w kolejce (poza meczem gracza).
    league.simulateRoundExceptPlayer(round, rng);
    // Przepisz wyniki do snapshotu.
    for (std::size_t i = 0; i < league.fixtures().size(); ++i) {
        s.league.results[i] = league.results()[i];
    }

    const i32 oppIdx = (fix->home == league.playerClubIndex()) ? fix->away : fix->home;
    const Club& opp = league.clubs()[oppIdx];
    const Club& pc = league.clubs()[league.playerClubIndex()];

    MatchSetup setup{};
    setup.fixture = *fix;
    setup.round = round;
    setup.opponent = opp;
    setup.playerClub = pc;
    setup.rules = ShootoutRules{};
    setup.playerShootsFirst = (fix->home == league.playerClubIndex()) ? Side::Home : Side::Away;
    setup.playerShooter = playerShooterProfile();
    setup.playerKeeper = playerKeeperProfile();
    // Prosty model CPU: profil proporcjonalny do siły (średnia ligi ~50).
    const f64 oppStrength = opp.strength;
    const f64 att = 0.30 + 0.07 * (oppStrength - 38.0);
    const f64 def = 0.30 + 0.07 * (oppStrength - 38.0);
    setup.cpuShooter = ShooterProfile::fromAttributes(
        clamp(att, 0.0, 1.0), clamp(att - 0.02, 0.0, 1.0), clamp(att + 0.02, 0.0, 1.0), clamp(att - 0.04, 0.0, 1.0));
    setup.cpuKeeper = KeeperProfile::forTier(s.tierIndex, oppStrength / 100.0);
    setup.leagueLabel = Pyramid::leagueLabel(s.tierIndex, s.homeVoivodeship, s.language);
    setup.seasonNumber = s.seasonNumber;
    setup.isDecisive = (round >= league.roundCount() - 2);

    s.phase = CareerPhase::MatchInProgress;
    return setup;
}

Result<void> Career::applyMatchOutcome(const MatchOutcome& outcome, Random& rng) {
    auto& s = state_;
    if (s.phase != CareerPhase::MatchInProgress) {
        return unexpected(makeError(errc::kWrongPhase, "career not in MatchInProgress"));
    }
    League league = makeLeague();
    // Znajdź mecz gracza w najbliższej nierozgranej kolejce.
    const i32 nextRound = league.nextIncompleteRound();
    const Fixture* fix = league.playerFixture(nextRound);
    if (!fix) return unexpected(makeError(errc::kNoMatch, "no fixture to record"));
    MatchResultRecord rec{};
    rec.played = true;
    rec.homeGoals = (fix->home == league.playerClubIndex()) ? outcome.playerGoals : outcome.opponentGoals;
    rec.awayGoals = (fix->away == league.playerClubIndex()) ? outcome.playerGoals : outcome.opponentGoals;
    rec.homeWins = rec.homeGoals > rec.awayGoals ? 1 : 0;
    rec.awayWins = rec.awayGoals > rec.homeGoals ? 1 : 0;
    rec.shootoutRounds = outcome.shootoutRounds;
    rec.playerPlayed = true;
    rec.playerWon = outcome.playerWon;
    league.recordResult(fix->matchIndex, rec);
    // Skopiuj wyniki z powrotem.
    for (std::size_t i = 0; i < league.fixtures().size(); ++i) {
        s.league.results[i] = league.results()[i];
    }

    // Aktualizacja statystyk.
    s.stats.matches++;
    if (outcome.playerWon) {
        s.stats.wins++;
        s.stats.currentWinStreak++;
        if (s.stats.currentWinStreak > s.stats.bestWinStreak) s.stats.bestWinStreak = s.stats.currentWinStreak;
    } else {
        s.stats.losses++;
        s.stats.currentWinStreak = 0;
    }
    s.stats.shotsTaken += outcome.playerShotsTaken;
    s.stats.goalsScored += outcome.playerGoalsScored;
    s.stats.savesAttempted += outcome.playerSavesAttempted;
    s.stats.saves += outcome.playerSaves;
    s.stats.woodworkHits += outcome.woodworkHits;
    s.stats.goalsConceded += outcome.opponentGoals - outcome.playerGoals < 0
                                ? 0 : (outcome.opponentGoals - outcome.playerGoals);
    if (outcome.cleanShootout) s.stats.cleanShootouts++;
    if (outcome.perfectShootout) s.stats.perfectShootouts++;
    if (outcome.shootoutRounds > 5) s.stats.suddenDeathMatches++;

    // XP.
    i32 xp = s.rules.xpBasePerMatch;
    if (outcome.playerWon) xp += s.rules.xpBonusWin;
    xp += outcome.playerGoalsScored * s.rules.xpPerGoal;
    xp += outcome.playerSaves * s.rules.xpPerSave;
    if (outcome.cleanShootout) xp += s.rules.xpPerCleanShootout;
    if (!outcome.playerWon) xp += s.rules.xpForLossConsolation;
    awardXp(xp);

    s.phase = CareerPhase::AfterMatch;
    (void)rng;
    return {};
}

Result<SeasonSummary> Career::finishSeasonIfComplete(Random& rng) {
    auto& s = state_;
    League league = makeLeague();
    if (!league.isSeasonComplete()) {
        return unexpected(makeError(errc::kIncomplete, "season not finished"));
    }
    // Dolicz XP za lokatę.
    League::PlayerFate fate = league.playerFate();
    i32 xpExtra = 0;
    switch (fate) {
        case League::PlayerFate::Champion: xpExtra = s.rules.xpChampionship; s.stats.championships++; break;
        case League::PlayerFate::Promoted: xpExtra = s.rules.xpPromotion; s.stats.promotions++; break;
        case League::PlayerFate::Relegated: s.stats.relegations++; break;
        case League::PlayerFate::Stayed:
        case League::PlayerFate::Unknown: break;
    }
    if (xpExtra > 0) awardXp(xpExtra);

    // Zapisz SeasonRecord.
    SeasonRecord rec{};
    rec.seasonNumber = s.seasonNumber;
    rec.tierIndex = s.tierIndex;
    rec.leagueLabel = Pyramid::leagueLabel(s.tierIndex, s.homeVoivodeship, s.language);
    const i32 pos = league.positionOf(league.playerClubIndex());
    rec.position = pos;
    rec.champion = (fate == League::PlayerFate::Champion);
    rec.promoted = (fate == League::PlayerFate::Promoted);
    rec.relegated = (fate == League::PlayerFate::Relegated);
    for (const auto& row : league.table()) {
        if (row.clubIndex == league.playerClubIndex()) {
            rec.wins = row.wins;
            rec.losses = row.losses;
            rec.goalsFor = row.goalsFor;
            rec.goalsAgainst = row.goalsAgainst;
        }
    }
    s.history.push_back(rec);

    // Rozstrzygnij awans/spadek.
    SeasonSummary summary{};
    summary.finalTable = league.table();
    summary.record = rec;
    summary.promotedClubs = league.promotionPlaces();
    summary.relegatedClubs = league.relegationPlaces();
    summary.playerFate = fate;
    summary.xpGained = xpExtra;
    summary.nextTierIndex = s.tierIndex;
    summary.seasonAdvanced = false;

    if (fate == League::PlayerFate::Promoted && s.tierIndex < Pyramid::kTopTier) {
        summary.nextTierIndex = s.tierIndex + 1;
    } else if (fate == League::PlayerFate::Relegated && s.tierIndex > 0) {
        summary.nextTierIndex = s.tierIndex - 1;
    } else if (fate == League::PlayerFate::Champion && s.tierIndex < Pyramid::kTopTier) {
        summary.nextTierIndex = s.tierIndex + 1;
    }

    // Osiągnięcia.
    summary.newAchievements = evaluateAchievements();

    s.phase = CareerPhase::SeasonSummary;
    (void)rng;
    return summary;
}

// ---------------------------------------------------------------------------
// Profile gracza
// ---------------------------------------------------------------------------

ShooterProfile Career::playerShooterProfile() const {
    ShooterProfile base = state_.attributes.toShooterProfile();
    // Lekka poprawka do progresu kariery: 0..60% → do 5% bonusu do mocy.
    const f64 p = clamp(static_cast<f64>(state_.progression.level) / 60.0, 0.0, 1.0);
    base.power = clamp(base.power + 0.05 * p, 0.0, 1.0);
    return base;
}

KeeperProfile Career::playerKeeperProfile() const {
    KeeperProfile kp = state_.attributes.toKeeperProfile();
    // Progres poprawia refleks nieznacznie.
    const f64 p = clamp(static_cast<f64>(state_.progression.level) / 60.0, 0.0, 1.0);
    kp.reactionTimeS = std::max(0.08, kp.reactionTimeS - 0.04 * p);
    return kp;
}

}  // namespace bkh