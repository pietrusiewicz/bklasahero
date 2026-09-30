// B-Klasa Hero — model bramkarza.
//
// Bramkarz jest symulowany KINEMATYCZNIE (nie fizycznie): ma pozycję ciała i
// pozycję rąk w funkcji czasu, wyznaczoną przez plan nurkowania. Intercepcja to
// test odległości między środkiem piłki a sferą chwytu rąk (i sferą tułowia).
// To podejście jest: deterministyczne, tanie (kilka mnożeń na krok), łatwe do
// zbalansowania i — co kluczowe przy braku urządzenia testowego — w pełni
// testowalne na hoście.
//
// Dwie drogi sterowania:
//   * gracz jako bramkarz: UI podaje DiveCommand (kierunek + wysokość + timing),
//   * CPU jako bramkarz: decideCpuKeeperDive() losuje decyzję na podstawie
//     readingSkill (szansa „odczytania" strzelca) i nawyków strzelca.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "bkh/physics.h"
#include "bkh/random.h"
#include "bkh/types.h"

namespace bkh {

enum class DiveSide : u8 { Left = 0, Center = 1, Right = 2 };
enum class DiveHeight : u8 { Low = 0, Mid = 1, High = 2 };

/// Umiejętności bramkarza. Wartości znormalizowane tam, gdzie to oznaczono.
struct KeeperProfile {
    f64 reactionTimeS = 0.22;     // bazowy czas reakcji [s] (0.14 — elita, 0.34 — B klasa)
    f64 diveSpeedMs = 5.6;        // prędkość boczna nurkowania [m/s]
    f64 standingReachM = 2.30;    // maksymalna wysokość rąk stojąc [m]
    f64 lateralReachM = 0.95;     // zasięg ramion od środka ciała [m]
    f64 diveExtensionM = 2.55;    // maksymalny boczny zasięg nurkowania od osi bramki [m]
    f64 readingSkill = 0.45;      // [0..1] szansa odczytania zamiaru strzelca
    f64 handlingSkill = 0.60;     // [0..1] pewność chwytu (niskie = częstsze „wyplucia")
    f64 composure = 0.55;         // [0..1] odporność na presję (rzuty decydujące)
    f64 lowBallReflex = 0.55;     // [0..1] skuteczność przy strzałach po ziemi w środek

    /// Profil bramkarza ligi o danym poziomie (0 = B klasa … 7 = Ekstraklasa).
    /// `progression` [0..1] to dodatkowy bonus z rozwoju/odblokowanych ulepszeń.
    [[nodiscard]] static KeeperProfile forTier(i32 tierIndex, f64 progression = 0.0);

    /// Profil zawodnika — budowany z jego atrybutów (career.h).
    [[nodiscard]] static KeeperProfile fromAttributes(f64 reflexes, f64 reach, f64 reading,
                                                      f64 composure, f64 handling);
};

/// Polecenie nurkowania. `timingErrorS` to błąd wyczucia gracza:
/// < 0 = za wcześnie (bramkarz leci przed strzałem), > 0 = za późno.
struct DiveCommand {
    DiveSide side = DiveSide::Center;
    DiveHeight height = DiveHeight::Mid;
    f64 timingErrorS = 0.0;
    bool commit = true;  // false = bramkarz „stoi" i czeka (ryzykowne, ale bywa skuteczne)

    [[nodiscard]] bool operator==(const DiveCommand&) const = default;
};

/// Rozstrzygnięty plan nurkowania — wszystko, czego potrzebuje symulacja.
struct KeeperPlan {
    DiveCommand command{};
    Vec2 targetM{};          // cel rąk w płaszczyźnie bramki (x bok, y wysokość)
    Vec3 startBodyM{};       // pozycja ciała na linii bramkowej
    Vec3 startHandsM{};      // pozycja rąk przed nurkowaniem
    f64 reactionTimeS = 0.22;
    f64 diveDurationS = 0.45;
    f64 catchRadiusM = 0.30; // promień sfery chwytu rąk
    f64 bodyRadiusM = 0.32;  // promień sfery tułowia (blokowanie strzałów „w brzuch")
    bool staysCenter = false;
};

/// Stan kinematyczny bramkarza w danej chwili.
struct KeeperState {
    Vec3 bodyM{};
    Vec3 handsM{};
    f64 diveProgress = 0.0;  // 0 = stoi, 1 = pełne wyciągnięcie
    bool diving = false;
    f64 timeS = 0.0;
};

/// Próbka stanu bramkarza dla renderera (parowana z TrajectorySample piłki).
struct KeeperSample {
    f64 timeS{};
    Vec3 bodyM{};
    Vec3 handsM{};
    f64 diveProgress = 0.0;
    DiveSide side = DiveSide::Center;
};

/// Informacja o kontakcie piłki z bramkarzem.
struct KeeperContact {
    bool touched = false;
    bool caught = false;      // true = pewny chwyt, false = odbicie („fumble")
    bool byBody = false;      // true = tułów/nogi, false = ręce
    f64 timeS = 0.0;
    f64 distanceM = 0.0;      // odległość środka piłki od sfery chwytu w momencie kontaktu
    Vec3 pointM{};
};

// ---------------------------------------------------------------------------
// Planowanie
// ---------------------------------------------------------------------------

/// Punkt docelowy rąk dla danego kierunku i wysokości (w płaszczyźnie bramki).
[[nodiscard]] Vec2 diveTarget(DiveSide side, DiveHeight height, const KeeperProfile& profile);

/// Czas reakcji po uwzględnieniu wyczucia gracza. Za wczesny skok wydłuża czas
/// „w powietrzu" (bramkarz opada), za późny skraca dostępny zasięg.
[[nodiscard]] f64 effectiveReactionTime(const KeeperProfile& profile, f64 timingErrorS);

/// Budowa planu z jawnego polecenia (gracz jako bramkarz).
[[nodiscard]] KeeperPlan planKeeperDive(const KeeperProfile& profile, const DiveCommand& command);

/// Decyzja bramkarza CPU. `shooterHabitBias` [-1..1] przechyla wybór strony
/// (nawyk strzelca), `readsAim` = czy bramkarz zna cel (test readingSkill jest
/// wykonywany w środku — wynik jest deterministyczny względem `rng`).
[[nodiscard]] KeeperPlan decideCpuKeeperDive(const KeeperProfile& profile, const ShotInput& shot,
                                            f64 shooterHabitBias, Random& rng);

// ---------------------------------------------------------------------------
// Symulacja krokowa
// ---------------------------------------------------------------------------

/// Ustawia stan początkowy bramkarza zgodnie z planem.
void initKeeper(KeeperState& state, const KeeperPlan& plan, const KeeperProfile& profile);

/// Przesuwa bramkarza o `dt` sekund wzdłuż planu. Czysta funkcja stanu.
void stepKeeper(KeeperState& state, f64 dt, const KeeperPlan& plan, const KeeperProfile& profile);

/// Test kontaktu piłki z bramkarzem w bieżącym kroku.
[[nodiscard]] KeeperContact keeperContact(const KeeperState& state, const Vec3& ballPositionM,
                                          const KeeperPlan& plan);

/// Czy kontakt kończy się pewnym chwytem (true) czy odbiciem piłki (false).
/// Losowanie deterministyczne względem `rng`; zależy od handlingSkill i trudności
/// strzału (prędkość + odległość od środka ciała).
[[nodiscard]] bool catchesBall(const KeeperContact& contact, const KeeperProfile& profile,
                              f64 ballSpeedMs, Random& rng);

/// Zmiana prędkości piłki po odbiciu od bramkarza (parry). Zwraca nowy wektor
/// prędkości — piłka może po nim wpaść do bramki (dramaturgia rzutów karnych).
[[nodiscard]] Vec3 deflectBall(const Vec3& velocityMs, const Vec3& contactPointM,
                               const Vec3& keeperHandsM, Random& rng);

/// Kierunek nurkowania w stronę czytelną dla UI (klucz ASCII).
[[nodiscard]] const char* messageKey(DiveSide side);
[[nodiscard]] const char* messageKey(DiveHeight height);

/// Kierunek przeciwny (pomocnicze przy testach i przy „nawykach" strzelca).
[[nodiscard]] constexpr DiveSide oppositeSide(DiveSide side) {
    switch (side) {
        case DiveSide::Left: return DiveSide::Right;
        case DiveSide::Right: return DiveSide::Left;
        case DiveSide::Center: return DiveSide::Center;
    }
    return DiveSide::Center;
}

}  // namespace bkh
