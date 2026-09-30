// B-Klasa Hero — sprzężona symulacja pojedynczego rzutu karnego.
//
// `executeShot()` to serce gry: łączy fizykę piłki (physics.h) z kinematyką
// bramkarza (keeper.h) w jednej pętli o sztywnym kroku czasowym i rozstrzyga
// wynik. Zwraca dwie rzeczy:
//   * ShotResolution — mały, serializowalny rekord (trafia do save'a i statystyk),
//   * ShotPlayback — próbki trajektorii piłki i bramkarza (tylko do renderu,
//     nigdy nie zapisywane; renderer pobiera je buforem float przez JNI).
//
// Kolejność testów w kroku jest ZAWSZE ta sama (kolizje z obramowaniem →
// kontakt z bramkarzem → przekroczenie linii bramkowej), bo od niej zależy
// powtarzalność wyniku na każdym urządzeniu.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vector>

#include "bkh/keeper.h"
#include "bkh/physics.h"
#include "bkh/random.h"
#include "bkh/shooter.h"
#include "bkh/types.h"

namespace bkh {

/// Wszystko, co determinuje pojedynczy rzut karny.
struct ShotContext {
    ShooterProfile shooter{};
    KeeperProfile keeper{};
    PhysicsParams physics = PhysicsParams::standard();
    f64 pressure = 0.0;          // [0..1] presja sytuacyjna
    f64 keeperHabitBias = 0.0;   // [-1..1] nawyk bramkarza (przechyla jego decyzję)
    bool cpuShoots = false;      // true = strzela CPU (plan generowany w rdzeniu)
    bool cpuKeeps = true;        // true = bramkarz CPU; false = bramkarz sterowany grą UI
    DiveCommand playerDive{};    // polecenie gracza, gdy cpuKeeps == false
    u64 kickSeed = 0;            // jawny seed strumienia tego rzutu (odtwarzalność)
};

/// Rozstrzygnięcie rzutu — serializowalne, bez trajektorii.
struct ShotResolution {
    ShotOutcome outcome = ShotOutcome::Stopped;
    Woodwork woodwork = Woodwork::None;
    bool keeperTouched = false;
    bool caught = false;
    bool fumbled = false;      // odbił, ale piłka i tak wpadła
    bool crossedGoalLine = false;
    bool bouncedOnGround = false;

    f64 crossingTimeS = 0.0;
    f64 flightTimeS = 0.0;
    f64 contactTimeS = 0.0;    // 0 = brak kontaktu z bramkarzem
    f64 crossingSpeedMs = 0.0;
    f64 maxSpeedMs = 0.0;
    f64 saveDistanceM = 0.0;   // o ile zabrakło bramkarzowi (>0 = nie sięgnął)
    f64 marginXm = 0.0;        // zapas względem słupka (>0 = w świetle bramki)
    f64 marginYm = 0.0;        // zapas względem poprzeczki

    Vec3 crossingPointM{};
    Vec2 intendedAimM{};       // gdzie strzelec CELOWAŁ
    Vec2 actualAimM{};         // gdzie realnie leciała piłka (po błędzie technicznym)
    f64 speedMs = 0.0;         // prędkość początkowa
    f64 sideSpinRps = 0.0;
    f64 topSpinRps = 0.0;
    TargetZone zone = TargetZone::LowRight;
    DiveSide keeperSide = DiveSide::Center;
    DiveHeight keeperHeight = DiveHeight::Mid;
    f64 keeperTimingErrorS = 0.0;

    /// Czy strzał był celny (w światło bramki), niezależnie od obrony.
    [[nodiscard]] bool onTarget() const {
        return outcome == ShotOutcome::Goal || outcome == ShotOutcome::Saved ||
               outcome == ShotOutcome::WoodworkIn || outcome == ShotOutcome::WoodworkOut;
    }
    /// Czy padła bramka (także po obramowaniu lub po odbiciu od bramkarza).
    [[nodiscard]] bool isGoal() const {
        return outcome == ShotOutcome::Goal || outcome == ShotOutcome::WoodworkIn;
    }
};

/// Dane do odtworzenia animacji rzutu. Nie są zapisywane w save'ie.
struct ShotPlayback {
    std::vector<TrajectorySample> ball;
    std::vector<KeeperSample> keeper;
    f64 durationS = 0.0;
    f64 contactTimeS = 0.0;
    f64 crossingTimeS = 0.0;
    Vec2 impactPointM{};   // gdzie piłka minęła linię bramkową (podświetlenie w UI)
    bool valid = false;
};

struct ShotExecution {
    ShotResolution resolution{};
    ShotPlayback playback{};
};

/// Pełna symulacja rzutu karnego.
///
/// `intent` jest ignorowana, gdy `ctx.cpuShoots == true` (plan generuje rdzeń).
/// `rng` służy wyłącznie do rozstrzygnięć losowych tego rzutu — wywołanie z tym
/// samym stanem `rng` i tymi samymi parametrami daje IDENTYCZNY wynik.
[[nodiscard]] ShotExecution executeShot(const ShotInput& intent, const ShotContext& ctx, Random& rng);

/// Skrót dla strzału CPU (bez jawnej intencji).
[[nodiscard]] ShotExecution executeCpuShot(const ShotContext& ctx, Random& rng);

// ---------------------------------------------------------------------------
// Mapowanie wejścia gracza na intencję strzału
// ---------------------------------------------------------------------------

/// Wejście gracza-strzelca z UI: punkt celowania w bramce (przeciąganie),
/// poziom mocy [0..1] i suwak rotacji [-1..1].
struct PlayerShotInput {
    Vec2 aimM{};             // wybrany punkt w bramce [m] (może być poza światłem!)
    f64 effort = 0.75;       // [0..1] moc
    f64 spin = 0.0;          // [-1..1] rotacja boczna (ujemna = w lewo)
    f64 loft = 0.0;          // [-1..1] podcięcie (ujemna = podcina, dodatnia = dociska)

    /// Przeliczenie na intencję dla danego profilu (bez błędu technicznego).
    [[nodiscard]] ShotInput toShotInput(const ShooterProfile& profile) const;
};

/// Walidacja wejścia gracza (przycina wartości do zakresów dopuszczalnych).
[[nodiscard]] PlayerShotInput sanitize(const PlayerShotInput& input);

/// Wejście gracza-bramkarza: kierunek + wysokość + timing z paska wyczucia.
struct PlayerDiveInput {
    DiveSide side = DiveSide::Center;
    DiveHeight height = DiveHeight::Mid;
    f64 timingErrorS = 0.0;  // wyliczane z różnicy między momentem naciśnięcia a strzałem
    bool commit = true;
};

[[nodiscard]] DiveCommand toDiveCommand(const PlayerDiveInput& input);

/// Błąd wyczucia [s] na podstawie okna czasowego: `windowS` to szerokość okna
/// „idealnie", `offsetS` — odchylenie od środka okna (dodatnie = za późno).
[[nodiscard]] f64 timingErrorFromWindow(f64 offsetS, f64 windowS = 0.12);

}  // namespace bkh
