// B-Klasa Hero — liga: terminarz, tabela, symulacja kolejki, awanse i spadki.
//
// Model sezonu (ustalony z użytkownikiem):
//   * 10 klubów w lidze, jedna runda „każdy z każdym" → 9 kolejek po 5 meczów,
//   * każdy mecz rozstrzyga konkurs rzutów karnych → NIE MA remisów
//     (zwycięzca 3 pkt, przegrany 0 pkt),
//   * mecz gracza rozgrywany interaktywnie, pozostałe 4 mecze kolejki
//     symuluje rdzeń (deterministycznie z ziarna sezonu),
//   * awans: 2 najlepsze kluby, spadek: 2 najsłabsze (poza skrajnymi poziomami),
//   * do tabeli liczymy „bramki" = trafienia z konkursów (suma goli z karnych),
//     co daje sensowny bilans i naturalne kryterium pomocnicze.
//
// Kryteria kolejności w tabeli (jak w niższych ligach PZPN):
//   1. punkty, 2. punkty w bezpośrednich meczach zespołów równych,
//   3. różnica bramek w bezpośrednich, 4. różnica bramek ogółem,
//   5. bramki zdobyte ogółem, 6. mniejsza liczba porażek, 7. id klubu (determinizm).
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <vector>

#include "bkh/club.h"
#include "bkh/random.h"
#include "bkh/shootout.h"
#include "bkh/types.h"

namespace bkh {

/// Mecz w terminarzu. Indeksy odnoszą się do `League::clubs()`.
struct Fixture {
    i32 matchIndex = 0;  // globalny indeks meczu w sezonie (0..44)
    i32 round = 0;       // kolejka (0..8)
    i32 home = 0;        // indeks klubu gospodarza
    i32 away = 0;        // indeks klubu gościa

    [[nodiscard]] bool involves(i32 clubIndex) const {
        return home == clubIndex || away == clubIndex;
    }
    [[nodiscard]] bool operator==(const Fixture&) const = default;
};

/// Wynik meczu (zawsze rozstrzygnięty konkursem karnych).
struct MatchResultRecord {
    bool played = false;
    i32 homeGoals = 0;   // trafienia gospodarza w konkursie
    i32 awayGoals = 0;   // trafienia gościa w konkursie
    i32 homeWins = 0;    // 1/0
    i32 awayWins = 0;    // 1/0
    i32 shootoutRounds = 5;  // >5 = była nagła śmierć
    bool playerPlayed = false;
    bool playerWon = false;

    [[nodiscard]] Side winner() const {
        return homeGoals > awayGoals ? Side::Home : Side::Away;
    }
    [[nodiscard]] i32 winnerClubGoals(Side side) const {
        return side == Side::Home ? homeGoals : awayGoals;
    }
};

/// Wiersz tabeli (przed sortowaniem — `position` wypełnia `League::table()`).
struct LeagueRow {
    i32 clubIndex = 0;
    i32 position = 0;
    i32 played = 0;
    i32 wins = 0;
    i32 losses = 0;
    i32 goalsFor = 0;
    i32 goalsAgainst = 0;
    i32 points = 0;

    [[nodiscard]] i32 goalDifference() const { return goalsFor - goalsAgainst; }
};

class League {
public:
    League() = default;

    /// `clubs` musi mieć dokładnie tyle elementów, ile przewiduje poziom ligi.
    /// `playerClubIndex` wskazuje klub gracza (−1 = brak, np. liga symulowana).
    League(std::vector<Club> clubs, i32 tierIndex, u64 seasonSeed, i32 playerClubIndex);

    [[nodiscard]] const std::vector<Club>& clubs() const { return clubs_; }
    [[nodiscard]] i32 clubCount() const { return static_cast<i32>(clubs_.size()); }
    [[nodiscard]] i32 tierIndex() const { return tierIndex_; }
    [[nodiscard]] u64 seasonSeed() const { return seasonSeed_; }
    [[nodiscard]] i32 playerClubIndex() const { return playerClubIndex_; }
    void setPlayerClubIndex(i32 index) { playerClubIndex_ = index; }
    [[nodiscard]] const Club* playerClub() const;

    // --- Terminarz -----------------------------------------------------------
    [[nodiscard]] const std::vector<Fixture>& fixtures() const { return fixtures_; }
    [[nodiscard]] i32 roundCount() const;
    [[nodiscard]] std::vector<const Fixture*> roundFixtures(i32 round) const;
    [[nodiscard]] const Fixture* fixtureAt(i32 matchIndex) const;
    /// Mecz klubu gracza w danej kolejce (nullptr, jeśli brak).
    [[nodiscard]] const Fixture* playerFixture(i32 round) const;
    /// Przeciwnik klubu gracza w danej kolejce (nullptr, jeśli brak).
    [[nodiscard]] const Club* playerOpponent(i32 round) const;

    // --- Wyniki --------------------------------------------------------------
    [[nodiscard]] const std::vector<MatchResultRecord>& results() const { return results_; }
    [[nodiscard]] const MatchResultRecord& result(i32 matchIndex) const;
    void recordResult(i32 matchIndex, const MatchResultRecord& record);

    /// Czy kolejka jest kompletna (wszystkie 5 meczów rozegrane).
    [[nodiscard]] bool isRoundComplete(i32 round) const;
    /// Numer najbliższej nierozegranej kolejki (roundCount(), gdy sezon skończony).
    [[nodiscard]] i32 nextIncompleteRound() const;
    [[nodiscard]] bool isSeasonComplete() const;
    [[nodiscard]] i32 playedMatches() const;

    /// Symuluje wszystkie NIErozegrane mecze danej kolejki poza meczem gracza.
    /// Deterministyczne: strumień RNG wywiedziony z seasonSeed i numeru kolejki.
    void simulateRoundExceptPlayer(i32 round, Random& rng);

    /// Symuluje całą kolejkę (używane przy symulowaniu sezonu bez udziału gracza).
    void simulateWholeRound(i32 round, Random& rng);

    // --- Tabela --------------------------------------------------------------
    /// Pełna tabela po zastosowaniu kryteriów pomocniczych.
    [[nodiscard]] std::vector<LeagueRow> table() const;
    /// Pozycja klubu w tabeli (1-based; clubCount()+1, jeśli klub nie istnieje).
    [[nodiscard]] i32 positionOf(i32 clubIndex) const;

    /// Kluby awansujące (indeksy w kolejności z tabeli). Puste na najwyższym poziomie.
    [[nodiscard]] std::vector<i32> promotionPlaces() const;
    /// Kluby spadające. Puste na najniższym poziomie.
    [[nodiscard]] std::vector<i32> relegationPlaces() const;

    /// Czy klub gracza awansował / spadł / utrzymał się.
    enum class PlayerFate { Unknown = 0, Champion = 1, Promoted = 2, Stayed = 3, Relegated = 4 };
    [[nodiscard]] PlayerFate playerFate() const;
    [[nodiscard]] static const char* messageKey(PlayerFate fate);

    // --- Serializacja (uzupełnia json_io.cpp) --------------------------------
    void restore(const std::vector<Fixture>& fixtures, const std::vector<MatchResultRecord>& results);

    /// Buduje terminarz „każdy z każdym" metodą kołową (deterministyczny układ).
    [[nodiscard]] static std::vector<Fixture> buildRoundRobin(i32 clubCount);

private:
    std::vector<Club> clubs_;
    std::vector<Fixture> fixtures_;
    std::vector<MatchResultRecord> results_;
    i32 tierIndex_ = 0;
    i32 playerClubIndex_ = -1;
    u64 seasonSeed_ = 0;
};

}  // namespace bkh
