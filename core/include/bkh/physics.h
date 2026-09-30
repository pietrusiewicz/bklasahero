// B-Klasa Hero — fizyka lotu piłki.
//
// Model (zob. docs/ADR/0003-ball-physics.md):
//   * punkt materialny z rotacją; siły: grawitacja, opór powietrza (kwadratowy),
//     efekt Magnusa (a = C_m · ω × v),
//   * całkowanie pół-jawnym Eulerem ze SZTYWNYM krokiem (domyślnie 1/480 s) —
//     stały krok jest warunkiem odtwarzalności wyniku między urządzeniami,
//   * odbicie od murawy (restitution + tarcie) — przydaje się przy strzałach
//     podciętych i przy „szczupakach" bramkarza,
//   * kolizja ze słupkami i poprzeczką jako z nieskończonymi walcami
//     (odbicie ze współczynnikiem restitution),
//   * klasyfikacja wyniku w chwili przekroczenia płaszczyzny linii bramkowej.
//
// Rdzeń nie rysuje i nie wie o Androidzie: zwraca próbki trajektorii, które
// warstwa Kotlin mapuje na rzut pseudo-3D na Canvasie.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <optional>
#include <vector>

#include "bkh/random.h"
#include "bkh/types.h"

namespace bkh {

// ---------------------------------------------------------------------------
// Parametry modelu
// ---------------------------------------------------------------------------
struct PhysicsParams {
    f64 airDensity = 1.2;             // [kg/m^3]
    f64 dragCoefficient = 0.25;       // [-] piłka meczowa, Re ~ 2e5
    f64 magnusCoefficient = 0.0060;   // [-] a_M = C_m · (ω × v), dobrane pod „czucie" gry
    f64 gravity = 9.80665;            // [m/s^2]
    f64 groundRestitution = 0.55;     // [-] pionowa sprężystość murawy
    f64 groundFriction = 0.82;        // [-] zachowanie składowych poziomych po odbiciu
    f64 spinDampingOnBounce = 0.60;   // [-] ile rotacji zostaje po koźle
    f64 woodworkRestitution = 0.62;   // [-] odbicie od słupka/poprzeczki
    f64 airSpinDecay = 0.10;          // [1/s] wykładnicze wygasanie rotacji w locie
    f64 timeStep = 1.0 / 480.0;       // [s] sztywny krok całkowania
    f64 maxFlightTime = 3.5;          // [s] po tym czasie lot uznajemy za zakończony
    f64 sampleInterval = 1.0 / 60.0;  // [s] gęstość próbek dla renderera
    f64 stopSpeed = 0.35;             // [m/s] poniżej tej prędkości po murawie piłka „staje"

    /// Domyślne parametry = „realistyczne z lekkim tuningiem arcade".
    [[nodiscard]] static PhysicsParams standard() { return PhysicsParams{}; }

    /// Wariant trudniejszy: mniejsza pomoc z rotacji, szybsza piłka (wyższe ligi).
    [[nodiscard]] static PhysicsParams competitive();
};

// ---------------------------------------------------------------------------
// Wejście strzału
// ---------------------------------------------------------------------------
/// Opisuje INTENCJĘ strzelca: dokąd celuje i z jaką siłą/podkręceniem.
/// Dokładność wykonania (błąd techniczny) dolicza symulacja na podstawie
/// profilu strzelca — patrz shooter.h.
struct ShotInput {
    Vec2 aimM{};          // cel w płaszczyźnie bramki: x = bok [m], y = wysokość [m]
    f64 speedMs = 22.0;   // prędkość początkowa środka piłki [m/s]
    f64 sideSpinRps = 0.0;   // rotacja boczna [rad/s], >0 = w prawo strzelca (fałsz prawy)
    f64 topSpinRps = 0.0;    // >0 = topspin (dociąża), <0 = backspin (podcina)
    Vec3 startPosM{0.0, pitch::kBallRadius, 0.0};  // punkt karny

    /// Kąt wzniosu wyliczony z celu — używany przez UI do podglądu.
    [[nodiscard]] f64 launchElevationDeg() const;
    /// Domyślna pozycja piłki na punkcie karnym.
    [[nodiscard]] static Vec3 defaultStart() { return {0.0, pitch::kBallRadius, 0.0}; }
};

// ---------------------------------------------------------------------------
// Wynik lotu
// ---------------------------------------------------------------------------
enum class ShotOutcome : u8 {
    Goal = 0,        // piłka w bramce (bez kontaktu z bramkarzem)
    Saved = 1,       // obroniona / złapana
    WoodworkIn = 2,  // po kontakcie z obramowaniem wpadła
    WoodworkOut = 3, // słupek lub poprzeczka, piłka poza bramką
    WideLeft = 4,    // niecelna obok lewego słupka
    WideRight = 5,   // niecelna obok prawego słupka
    OverBar = 6,     // nad poprzeczką
    Stopped = 7,     // piłka zatrzymała się przed linią (praktycznie niemożliwe przy karnych)
};

enum class Woodwork : u8 { None = 0, LeftPost = 1, RightPost = 2, Crossbar = 3 };

/// Stabilny klucz ASCII wyniku — warstwa Android mapuje go na strings.xml.
/// Rdzeń celowo NIE zwraca przetłumaczonego tekstu (zob. ADR-0005).
[[nodiscard]] const char* messageKey(ShotOutcome outcome);
[[nodiscard]] const char* messageKey(Woodwork woodwork);

struct TrajectorySample {
    f64 timeS{};
    Vec3 positionM{};
    Vec3 velocityMs{};
};

struct BallFlight {
    ShotOutcome outcome = ShotOutcome::Stopped;
    Woodwork woodwork = Woodwork::None;
    bool crossedGoalLine = false;
    f64 crossingTimeS = 0.0;    // 0 = brak przekroczenia
    Vec3 crossingPointM{};      // punkt przebicia płaszczyzny z = 11 m
    f64 crossingSpeedMs = 0.0;
    f64 flightTimeS = 0.0;      // czas do końca symulacji
    f64 marginXm = 0.0;         // odległość od bliższego słupka (>0 = w światle bramki)
    f64 marginYm = 0.0;         // odległość od poprzeczki (>0 = pod poprzeczką)
    f64 maxSpeedMs = 0.0;       // prędkość maksymalna (statystyki, „radar")
    f64 speedAtCrossingMs = 0.0;
    bool bouncedOnGround = false;
    bool touchedWoodwork = false;
    std::vector<TrajectorySample> samples;  // próbki co sampleInterval, dla renderera

    /// Czy strzał był „w światle bramki" (niezależnie od obrony).
    [[nodiscard]] bool wasOnTarget() const {
        return outcome == ShotOutcome::Goal || outcome == ShotOutcome::Saved ||
               outcome == ShotOutcome::WoodworkIn || outcome == ShotOutcome::WoodworkOut;
    }
};

// ---------------------------------------------------------------------------
// Stan piłki i pojedynczy krok całkowania
// ---------------------------------------------------------------------------
struct BallState {
    Vec3 positionM{};
    Vec3 velocityMs{};
    Vec3 spinRps{};
    f64 timeS = 0.0;
    bool alive = true;  // false po złapaniu/zatrzymaniu
};

/// Zdarzenia wykryte w pojedynczym kroku całkowania.
struct IntegrationEvent {
    bool groundBounce = false;
    Woodwork woodwork = Woodwork::None;
    bool crossedGoalLine = false;
    Vec3 goalLinePointM{};   // punkt przebicia płaszczyzny bramki
    f64 goalLineTimeS = 0.0;
    f64 goalLineSpeedMs = 0.0;
    bool outOfPlay = false;  // piłka poza strefą zainteresowania (daleko za/dobok bramki)
    bool stopped = false;
};

/// Jeden krok symulacji o `dt` sekund. Modyfikuje `state` i zwraca zdarzenia.
/// Czysta funkcja stanu — bez alokacji, bez RNG, bez wiedzy o bramkarzu.
IntegrationEvent integrateBall(BallState& state, f64 dt, const PhysicsParams& params);

/// Przeliczenie `ShotInput` na stan początkowy piłki (wektor prędkości z celu).
[[nodiscard]] BallState makeBallState(const ShotInput& input);

/// Pełny lot piłki BEZ bramkarza — do testów fizyki i do podglądu trajektorii.
[[nodiscard]] BallFlight simulateBallFlight(const ShotInput& input,
                                           const PhysicsParams& params = PhysicsParams::standard(),
                                           bool recordSamples = true);

// ---------------------------------------------------------------------------
// Pomocnicze: geometria bramki
// ---------------------------------------------------------------------------
namespace goal {

/// Płaszczyzna linii bramkowej w z.
inline constexpr f64 kLineZ = pitch::kPenaltyDistance;

/// Czy punkt (środek piłki) w płaszczyźnie bramki mieści się w świetle bramki
/// z zapasem na promień piłki.
[[nodiscard]] inline bool isInsideFrame(const Vec2& pointM, f64 ballRadius = pitch::kBallRadius) {
    return std::abs(pointM.x) <= (pitch::kHalfGoalWidth - ballRadius) &&
           pointM.y >= ballRadius && pointM.y <= (pitch::kGoalHeight - ballRadius);
}

/// Odległość środka piłki od osi lewego/prawego słupka w płaszczyźnie z = kLineZ.
[[nodiscard]] inline f64 distanceToPost(const Vec2& pointM, bool leftPost) {
    const f64 postX = leftPost ? -pitch::kHalfGoalWidth : pitch::kHalfGoalWidth;
    return std::hypot(pointM.x - postX, pointM.y - 0.0);
}

/// Odległość od osi poprzeczki (walec poziomy na wysokości kGoalHeight).
[[nodiscard]] inline f64 distanceToCrossbar(const Vec2& pointM) {
    return std::abs(pointM.y - pitch::kGoalHeight);
}

/// Zapas względem słupka/poprzeczki (>0 = w świetle bramki) — do komentarzy UI.
[[nodiscard]] inline Vec2 margins(const Vec2& pointM, f64 ballRadius = pitch::kBallRadius) {
    const f64 mx = (pitch::kHalfGoalWidth - ballRadius) - std::abs(pointM.x);
    const f64 my = (pitch::kGoalHeight - ballRadius) - pointM.y;
    return {mx, my};
}

}  // namespace goal

}  // namespace bkh
