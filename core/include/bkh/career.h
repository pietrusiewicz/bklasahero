// B-Klasa Hero — kariera: postać, rozwój, kosmetyka, statystyki i stan sezonu.
//
// `Career` to agregat stanu zapisywalnego (save). Nie zna Androida, nie zna
// zegara (czas podaje warstwa platformowa), nie wykonuje operacji wejścia-wyjścia
// — serializacja jest w json_io.h, a transport w facade.h.
//
// Zasady projektowe:
//   * wszystko losowe przechodzi przez jawne ziarno i `Random::fork(tag)`,
//     więc ten sam save + ta sama akcja = ten sam skutek (testowalne na hoście),
//   * atrybuty gracza są liczbami całkowitymi 1..20 (czytelne w UI i w save'ie),
//     na profile fizyczne mapują funkcje `toShooterProfile()` / `toKeeperProfile()`,
//   * postęp (XP/poziomy/punkty umiejętności) jest regułami w `ProgressionRules`,
//     żeby balans dało się stroić w jednym miejscu i testować.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <string>
#include <vector>

#include "bkh/club.h"
#include "bkh/keeper.h"
#include "bkh/league.h"
#include "bkh/place.h"
#include "bkh/random.h"
#include "bkh/result.h"
#include "bkh/shooter.h"
#include "bkh/shootout.h"
#include "bkh/types.h"

namespace bkh {

// ---------------------------------------------------------------------------
// Atrybuty gracza
// ---------------------------------------------------------------------------
enum class AttributeKind : u8 {
    ShotPower = 0,    // moc strzału
    ShotAccuracy = 1, // precyzja (mniejszy błąd techniczny)
    Composure = 2,    // opanowanie pod presją
    Curve = 3,        // rotacja (efektywność Magnusa)
    Reflexes = 4,     // czas reakcji w bramce
    Reach = 5,        // zasięg nurkowania
    Reading = 6,      // czytanie strzelca (podpowiedź kierunku)
    Handling = 7,     // pewność chwytu (mniej odbić)
    Count = 8,
};

[[nodiscard]] const char* messageKey(AttributeKind kind);
/// Czy atrybut dotyczy bronienia (true) czy strzelania (false).
[[nodiscard]] constexpr bool isKeeperAttribute(AttributeKind kind) {
    return kind >= AttributeKind::Reflexes && kind < AttributeKind::Count;
}

struct PlayerAttributes {
    static constexpr i32 kMin = 1;
    static constexpr i32 kMax = 20;
    static constexpr i32 kStart = 6;

    std::array<i32, static_cast<std::size_t>(AttributeKind::Count)> values{};

    PlayerAttributes() { values.fill(kStart); }

    [[nodiscard]] i32 get(AttributeKind kind) const;
    void set(AttributeKind kind, i32 value);
    bool increase(AttributeKind kind);  // zwraca false, gdy już maksymalny

    /// Profil strzelca z atrybutów (znormalizowany do [0..1]).
    [[nodiscard]] ShooterProfile toShooterProfile() const;
    /// Profil bramkarza z atrybutów.
    [[nodiscard]] KeeperProfile toKeeperProfile() const;

    [[nodiscard]] bool operator==(const PlayerAttributes&) const = default;
};

// ---------------------------------------------------------------------------
// Postęp i rozwój
// ---------------------------------------------------------------------------
struct ProgressionRules {
    i32 xpBasePerMatch = 40;
    i32 xpBonusWin = 60;
    i32 xpPerGoal = 12;
    i32 xpPerSave = 14;
    i32 xpPerCleanShootout = 50;  // wszystkie 5 rzutów obronione/strzelone
    i32 xpPromotion = 400;
    i32 xpChampionship = 700;
    i32 xpForLossConsolation = 20;

    i32 levelXpBase = 120;    // XP potrzebny na poziom 2
    f64 levelXpGrowth = 1.22; // mnożnik na każdy kolejny poziom
    i32 skillPointsPerLevel = 1;
    i32 maxLevel = 60;

    /// XP potrzebny, żeby wejść z `level` na `level + 1`.
    [[nodiscard]] i32 xpForLevel(i32 level) const;
};

struct Progression {
    i32 level = 1;
    i32 xp = 0;               // XP w bieżącym poziomie
    i32 skillPoints = 0;      // niewydane punkty umiejętności
    i32 totalXp = 0;

    [[nodiscard]] bool operator==(const Progression&) const = default;
};

// ---------------------------------------------------------------------------
// Kosmetyka (bez mikropłatności — odblokowywana osiągnięciami)
// ---------------------------------------------------------------------------
enum class CosmeticKind : u8 { Kit = 0, Ball = 1, Gloves = 2, Net = 3, Count = 4 };
[[nodiscard]] const char* messageKey(CosmeticKind kind);

struct CosmeticItem {
    std::string id;        // stabilny identyfikator ASCII, np. "kit.bklasa.green"
    std::string nameKey;   // klucz tłumaczenia, np. "cosmetic.kit.bklasa.green"
    CosmeticKind kind = CosmeticKind::Kit;
    bool unlocked = false;
    bool defaultItem = false;
    u32 colorPrimary = 0xFF2E7D32;
    u32 colorSecondary = 0xFFFFFFFF;
    std::string unlockRequirementKey;  // klucz osiągnięcia, np. "achieve.first_promotion"

    [[nodiscard]] bool operator==(const CosmeticItem&) const = default;
};

/// Katalog wszystkich przedmiotów kosmetycznych (statyczny, bez alokacji danych
/// zewnętrznych). Odblokowania trzymane są w stanie kariery.
[[nodiscard]] std::vector<CosmeticItem> defaultCosmetics();

// ---------------------------------------------------------------------------
// Statystyki
// ---------------------------------------------------------------------------
struct CareerStats {
    i32 matches = 0;
    i32 wins = 0;
    i32 losses = 0;

    i32 shotsTaken = 0;
    i32 goalsScored = 0;
    i32 shotsOnTarget = 0;
    i32 woodworkHits = 0;

    i32 savesAttempted = 0;   // rzuty bronione (przeciwko graczowi)
    i32 saves = 0;
    i32 goalsConceded = 0;
    i32 cleanShootouts = 0;   // konkurs bez puszczonego gola

    i32 suddenDeathMatches = 0;
    i32 perfectShootouts = 0;  // 5/5 jako strzelec i 0 straconych jako bramkarz
    i32 bestWinStreak = 0;
    i32 currentWinStreak = 0;
    i32 promotions = 0;
    i32 relegations = 0;
    i32 championships = 0;
    i32 seasonsPlayed = 0;

    [[nodiscard]] f64 shotConversion() const {
        return shotsTaken > 0 ? static_cast<f64>(goalsScored) / static_cast<f64>(shotsTaken) : 0.0;
    }
    [[nodiscard]] f64 saveRate() const {
        return savesAttempted > 0 ? static_cast<f64>(saves) / static_cast<f64>(savesAttempted) : 0.0;
    }
    [[nodiscard]] bool operator==(const CareerStats&) const = default;
};

struct SeasonRecord {
    i32 seasonNumber = 1;
    i32 tierIndex = 0;
    std::string leagueLabel;  // wygenerowana etykieta ligi (zapisana, żeby historia
                              // była czytelna nawet po zmianie danych geo)
    i32 position = 0;
    i32 wins = 0;
    i32 losses = 0;
    i32 goalsFor = 0;
    i32 goalsAgainst = 0;
    bool champion = false;
    bool promoted = false;
    bool relegated = false;

    [[nodiscard]] bool operator==(const SeasonRecord&) const = default;
};

/// Pełny, zapisywalny stan ligi (kluby + terminarz + wyniki).
struct LeagueSnapshot {
    std::vector<Club> clubs;
    std::vector<Fixture> fixtures;
    std::vector<MatchResultRecord> results;
    i32 tierIndex = 0;
    i32 playerClubIndex = -1;
    u64 seasonSeed = 0;
};

// ---------------------------------------------------------------------------
// Stan kariery
// ---------------------------------------------------------------------------
enum class CareerPhase : u8 {
    NoCareer = 0,      // brak zapisanej kariery
    BeforeMatch = 1,   // trwa sezon, czeka na rozpoczęcie meczu
    MatchInProgress = 2,
    AfterMatch = 3,    // mecz rozegrany, czeka na zatwierdzenie/awans
    SeasonSummary = 4, // sezon zakończony, pokazujemy podsumowanie
};
[[nodiscard]] const char* messageKey(CareerPhase phase);

struct CareerState {
    static constexpr i32 kSchemaVersion = 1;

    i32 schemaVersion = kSchemaVersion;
    u64 seed = 0;
    Lang language = Lang::Pl;

    std::string nickname;
    i64 homeOsmId = 0;
    std::string homeCity;
    u8 homeVoivodeship = Place::kUnknownVoivodeship;

    i32 tierIndex = 0;
    i32 seasonNumber = 1;

    LeagueSnapshot league;
    PlayerAttributes attributes;
    Progression progression;
    ProgressionRules rules;
    CareerStats stats;
    std::vector<CosmeticItem> cosmetics;

    std::string equippedKit;
    std::string equippedBall;
    std::string equippedGloves;
    std::string equippedNet;

    std::vector<SeasonRecord> history;
    std::vector<std::string> unlockedAchievements;  // klucze ASCII

    CareerPhase phase = CareerPhase::NoCareer;
    i64 savedAtEpochMs = 0;  // podawane przez warstwę platformową (rdzeń nie zna zegara)

    [[nodiscard]] const CosmeticItem* equipped(CosmeticKind kind) const;
    [[nodiscard]] const CosmeticItem* findById(const std::string& id) const;
};

// ---------------------------------------------------------------------------
// Parametry nowej kariery i wyniki meczów
// ---------------------------------------------------------------------------
struct NewCareerParams {
    std::string nickname;
    i64 homeOsmId = 0;          // wybrana miejscowość (z katalogu)
    u64 seed = 0;               // 0 = wygeneruj deterministycznie z nickname
    Lang language = Lang::Pl;
    i32 startingTier = 0;       // domyślnie B klasa
    std::string preferredClubName;  // opcjonalnie: nazwa własnego klubu
};

/// Kontekst meczu przygotowany dla UI (rozpoczęcie konkursu).
struct MatchSetup {
    Fixture fixture{};
    i32 round = 0;
    Club opponent{};
    Club playerClub{};
    ShootoutRules rules{};
    Side playerShootsFirst = Side::Home;  // czy klub gracza zaczyna strzelanie
    ShooterProfile playerShooter{};
    KeeperProfile playerKeeper{};
    ShooterProfile cpuShooter{};
    KeeperProfile cpuKeeper{};
    std::string leagueLabel;
    i32 seasonNumber = 1;
    /// Czy mecz jest „o awans" (wpływa na presję i komunikaty).
    bool isDecisive = false;
};

/// Rezultat meczu gracza do zapisania w lidze i statystykach.
struct MatchOutcome {
    i32 matchIndex = 0;
    i32 playerGoals = 0;      // trafienia klubu gracza
    i32 opponentGoals = 0;    // trafienia przeciwnika
    i32 shootoutRounds = 5;
    bool playerWon = false;
    i32 playerShotsTaken = 0;
    i32 playerGoalsScored = 0;
    i32 playerSavesAttempted = 0;
    i32 playerSaves = 0;
    i32 woodworkHits = 0;
    bool cleanShootout = false;
    bool perfectShootout = false;
    std::vector<KickRecord> kicks;  // pełna historia rzutów (do podsumowania meczu)
};

struct SeasonSummary {
    std::vector<LeagueRow> finalTable;
    SeasonRecord record;
    std::vector<i32> promotedClubs;
    std::vector<i32> relegatedClubs;
    League::PlayerFate playerFate = League::PlayerFate::Unknown;
    i32 xpGained = 0;
    std::vector<std::string> newAchievements;
    i32 nextTierIndex = 0;
    bool seasonAdvanced = false;
};

// ---------------------------------------------------------------------------
// Kariera
// ---------------------------------------------------------------------------
class Career {
public:
    Career() = default;

    /// Tworzy nową karierę: generuje ligę startową wokół `homePlace`.
    [[nodiscard]] static Result<Career> startNew(const NewCareerParams& params,
                                                const PlaceCatalog& catalog);

    [[nodiscard]] const CareerState& state() const { return state_; }
    [[nodiscard]] CareerState& mutableState() { return state_; }
    void setState(CareerState state) { state_ = std::move(state); }

    /// Odtwarza ligę ze snapshotu (po wczytaniu save'a).
    [[nodiscard]] League makeLeague() const;

    // --- Mecz ----------------------------------------------------------------
    [[nodiscard]] Result<MatchSetup> beginMatch(Random& rng);
    Result<void> applyMatchOutcome(const MatchOutcome& outcome, Random& rng);

    /// Sprawdza, czy sezon się skończył; jeśli tak — rozstrzyga awanse/spadki,
    /// buduje nowy sezon i zwraca podsumowanie.
    [[nodiscard]] Result<SeasonSummary> finishSeasonIfComplete(Random& rng);

    // --- Rozwój --------------------------------------------------------------
    [[nodiscard]] i32 xpToNextLevel() const;
    [[nodiscard]] Result<void> upgradeAttribute(AttributeKind kind);
    /// Dolicza XP i przyznaje poziomy/punkty umiejętności. Zwraca liczbę nowych poziomów.
    i32 awardXp(i32 amount);

    // --- Kosmetyka -----------------------------------------------------------
    [[nodiscard]] Result<void> unlockCosmetic(const std::string& id);
    [[nodiscard]] Result<void> equipCosmetic(CosmeticKind kind, const std::string& id);

    // --- Osiągnięcia ---------------------------------------------------------
    /// Sprawdza reguły osiągnięć po meczu/sezonie; zwraca klucze nowo odblokowanych.
    std::vector<std::string> evaluateAchievements();

    // --- Profile fizyczne gracza ---------------------------------------------
    [[nodiscard]] ShooterProfile playerShooterProfile() const;
    [[nodiscard]] KeeperProfile playerKeeperProfile() const;

private:
    Result<void> buildSeason(i32 tierIndex, i32 seasonNumber, Random& rng);

    CareerState state_{};
};

/// Reguły osiągnięć — klucz ASCII + warunek. Zdeklarowane w career.cpp,
/// używane przez `evaluateAchievements()` i przez UI (ekran osiągnięć).
struct AchievementRule {
    const char* key;
    const char* nameKey;
    bool (*isMet)(const CareerState& state);
};
[[nodiscard]] const std::vector<AchievementRule>& achievementRules();

}  // namespace bkh
