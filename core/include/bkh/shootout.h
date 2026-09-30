// B-Klasa Hero — maszyna stanów konkursu rzutów karnych.
//
// Zasady (IFAB, „kopnięcia z punktu karnego"):
//   * po 5 rzutów na stronę, naprzemiennie (domyślnie gospodarz zaczyna),
//   * konkurs kończy się WCZEŚNIEJ, gdy jedna ze stron ma już matematycznie
//     zapewnione zwycięstwo (np. 3:0 po trzech seriach),
//   * remis po piątce → „nagła śmierć": po jednym rzucie na stronę, porównanie
//     po każdej parze.
//
// W karierze gracz gra OBYDWIE role w jednym konkursie: gdy jego klub strzela —
// gracz jest strzelcem, gdy strzela rywal — gracz jest bramkarzem. Stąd pole
// `PlayerRole` w każdym rekordzie rzutu.
//
// Ta klasa jest CZYSTĄ logiką: nie liczy fizyki, nie losuje (poza jawnym `rng`
// przekazanym przez wywołującego), nie zna Androida. Można ją w całości
// przetestować na hoście — i tak właśnie robimy (core/tests/test_shootout.cpp).
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <optional>
#include <vector>

#include "bkh/shot.h"
#include "bkh/types.h"

namespace bkh {

enum class Side : u8 { Home = 0, Away = 1 };

[[nodiscard]] constexpr Side opponentOf(Side side) {
    return side == Side::Home ? Side::Away : Side::Home;
}
[[nodiscard]] const char* messageKey(Side side);

/// Rola gracza w danym rzucie.
enum class PlayerRole : u8 { None = 0, Shooter = 1, Keeper = 2 };
[[nodiscard]] const char* messageKey(PlayerRole role);

struct ShootoutRules {
    i32 kicksPerSide = 5;
    bool suddenDeath = true;
    Side firstKicker = Side::Home;
    /// Maksymalna liczba serii nagłej śmierci (bezpiecznik przed nieskończonością;
    /// po jej przekroczeniu zwycięzcę rozstrzyga `tieBreakerShootout`).
    i32 maxSuddenDeathRounds = 20;

    [[nodiscard]] bool operator==(const ShootoutRules&) const = default;
};

/// Pojedynczy rzut wraz z całym kontekstem potrzebnym do statystyk i save'a.
struct KickRecord {
    i32 sequence = 0;        // globalny numer rzutu (od 0)
    i32 round = 0;           // numer serii (od 0; 0..4 = pierwsza piątka)
    Side side = Side::Home;  // kto strzelał
    bool suddenDeath = false;
    PlayerRole playerRole = PlayerRole::None;

    i32 shooterClubId = 0;
    i32 keeperClubId = 0;

    ShotResolution shot{};

    [[nodiscard]] bool scored() const { return shot.isGoal(); }
};

enum class ShootoutStatus : u8 { NotStarted = 0, InProgress = 1, Finished = 2 };

struct ShootoutState {
    ShootoutRules rules{};
    i32 homeScore = 0;
    i32 awayScore = 0;
    i32 homeTaken = 0;
    i32 awayTaken = 0;
    ShootoutStatus status = ShootoutStatus::NotStarted;
    std::optional<Side> winner{};
    bool decidedEarly = false;     // rozstrzygnięty przed wyczerpaniem pierwszej piątki
    bool decidedByTieBreaker = false;  // po przekroczeniu limitu nagłej śmierci
    i32 suddenDeathRounds = 0;
    std::vector<KickRecord> kicks;
};

class Shootout {
public:
    Shootout() = default;
    explicit Shootout(ShootoutRules rules) : state_(ShootoutState{rules}) {}

    [[nodiscard]] const ShootoutState& state() const { return state_; }
    ShootoutState& mutableState() { return state_; }
    void setState(ShootoutState state) { state_ = std::move(state); }

    [[nodiscard]] const ShootoutRules& rules() const { return state_.rules; }

    [[nodiscard]] bool isFinished() const { return state_.status == ShootoutStatus::Finished; }
    [[nodiscard]] std::optional<Side> winner() const { return state_.winner; }

    [[nodiscard]] i32 score(Side side) const {
        return side == Side::Home ? state_.homeScore : state_.awayScore;
    }
    [[nodiscard]] i32 taken(Side side) const {
        return side == Side::Home ? state_.homeTaken : state_.awayTaken;
    }

    /// Kto wykonuje następny rzut (niezdefiniowane po zakończeniu — zwraca
    /// `firstKicker`, a `isFinished()` mówi, czy nie pytać).
    [[nodiscard]] Side nextKicker() const;

    /// Numer serii (0-based) dla następnego rzutu.
    [[nodiscard]] i32 nextRound() const;

    /// Czy konkurs jest już w fazie nagłej śmierci.
    [[nodiscard]] bool inSuddenDeath() const;

    /// Pozostałe rzuty strony w podstawowej piątce (0 w nagłej śmierci).
    [[nodiscard]] i32 remainingRegularKicks(Side side) const;

    /// Czy strona może jeszcze wygrać/zremisować — używane do komunikatów UI
    /// („rzut o wszystko") i do wyliczania presji.
    [[nodiscard]] bool canStillWin(Side side) const;

    /// Czy ten rzut jest „o wszystko" dla strony `side` (pudło kończy konkurs).
    [[nodiscard]] bool isMustScore(Side side) const;

    /// Presja sytuacyjna [0..1] dla następnego rzutu strony `side`.
    [[nodiscard]] f64 pressureFor(Side side) const;

    /// Zapisuje rezultat rzutu i przelicza stan konkursu. Zwraca true, jeśli
    /// rzut został przyjęty (konkurs nie był zakończony).
    bool recordKick(const KickRecord& kick);

    /// Wymusza rozstrzygnięcie po przekroczeniu limitu nagłej śmierci.
    /// Wygrywa strona z lepszym bilansem serii; przy pełnym remisie — `fallback`.
    void resolveTieBreaker(Side fallback);

    /// Skrót: czy konkurs zakończył się bez rozgrywania wszystkich 10 rzutów.
    [[nodiscard]] bool wasDecidedEarly() const { return state_.decidedEarly; }

    /// Liczba rzutów obu stron razem.
    [[nodiscard]] std::size_t kickCount() const { return state_.kicks.size(); }

private:
    void evaluateCompletion();

    ShootoutState state_{};
};

// ---------------------------------------------------------------------------
// Pomocnicze funkcje czyste (używane też przez testy i przez symulację ligi)
// ---------------------------------------------------------------------------

/// Czy przy danym stanie konkurs jest już rozstrzygnięty matematycznie?
/// Zwraca zwycięzcę albo std::nullopt. Nie uwzględnia nagłej śmierci.
[[nodiscard]] std::optional<Side> earlyWinner(i32 homeScore, i32 awayScore, i32 homeTaken,
                                             i32 awayTaken, const ShootoutRules& rules);

/// Szybka symulacja konkursu dla meczów ligowych rozgrywanych „bez udziału gracza"
/// (reszta kolejki). Zwraca wynik w rzutach, np. {4, 3}. Deterministyczna z `rng`.
struct ShootoutScore {
    i32 home = 0;
    i32 away = 0;
    i32 rounds = 5;  // liczba serii (5 = podstawowa, >5 = nagła śmierć)
    [[nodiscard]] Side winner() const { return home > away ? Side::Home : Side::Away; }
};
[[nodiscard]] ShootoutScore simulateShootout(f64 homeStrength, f64 awayStrength, Random& rng);

/// Symulacja wyniku meczu ligowego: konkurs karnych decyduje o wyniku, ale dla
/// tabeli potrzebujemy też „bramek" (suma trafień z konkursu po obu stronach).
struct LeagueMatchScore {
    i32 homeGoals = 0;
    i32 awayGoals = 0;
    ShootoutScore shootout{};
};
[[nodiscard]] LeagueMatchScore simulateLeagueMatch(f64 homeStrength, f64 awayStrength, Random& rng);

}  // namespace bkh
