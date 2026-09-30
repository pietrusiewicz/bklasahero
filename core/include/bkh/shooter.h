// B-Klasa Hero — profil strzelca i wykonanie strzału.
//
// Rozdzielenie odpowiedzialności:
//   * INTENCJA (ShotInput) — dokąd strzelec chce trafić i z jaką siłą;
//   * WYKONANIE (ExecutionError) — ile realnie odchyli piłkę od zamiaru,
//     zależnie od umiejętności, presji sytuacji i losowości (boundedNormal);
//   * SKUTEK (ShotResolution w shot.h) — wynik sprzężonej symulacji z bramkarzem.
//
// Dzięki temu ta sama intencja gracza (wybrany punkt + siła + rotacja) daje
// różne skutki w zależności od atrybutów postaci, a CPU strzela według
// czytelnych, testowalnych reguł (celuje w strefy, myli się pod presją).
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "bkh/keeper.h"
#include "bkh/physics.h"
#include "bkh/random.h"
#include "bkh/types.h"

namespace bkh {

/// Umiejętności strzelca. Atrybuty gracza (career.h) mapują się na ten profil.
struct ShooterProfile {
    f64 power = 0.55;        // [0..1] wpływa na prędkość początkową
    f64 accuracy = 0.55;     // [0..1] odwrotność błędu technicznego
    f64 composure = 0.55;    // [0..1] odporność na presję decydujących rzutów
    f64 curve = 0.50;        // [0..1] skuteczność rotacji (Magnus)
    f64 consistency = 0.55;  // [0..1] mniej = większy rozrzut między strzałami

    /// Profil strzelca CPU dla ligi o danym poziomie (0 = B klasa … 7 = Ekstraklasa).
    [[nodiscard]] static ShooterProfile forTier(i32 tierIndex);

    /// Profil zawodnika z jego atrybutów.
    [[nodiscard]] static ShooterProfile fromAttributes(f64 power, f64 accuracy, f64 composure,
                                                       f64 curve);

    /// Prędkość początkowa [m/s] dla zadanego „poziomu mocy" [0..1].
    [[nodiscard]] f64 shotSpeed(f64 effort) const;

    /// Odchylenie standardowe błędu technicznego [m] w płaszczyźnie bramki.
    [[nodiscard]] f64 aimSigmaM(f64 pressure) const;
};

/// Błąd wykonania doliczany do intencji strzelca.
struct ExecutionError {
    f64 dxM = 0.0;        // boczne odchylenie celu [m]
    f64 dyM = 0.0;        // pionowe odchylenie celu [m]
    f64 speedFactor = 1.0;   // mnożnik prędkości (np. 0.94 = „niedokręcony")
    f64 spinFactor = 1.0;    // mnożnik rotacji (technika podkręcenia)
};

/// Strefy bramki używane przez CPU i przez podpowiedzi UI (siatka 3×3 + rogi).
enum class TargetZone : u8 {
    TopLeft = 0, TopCenter = 1, TopRight = 2,
    MidLeft = 3, MidCenter = 4, MidRight = 5,
    LowLeft = 6, LowCenter = 7, LowRight = 8,
};

/// Punkt w bramce dla strefy (z małym jitterem, jeśli podano `rng`).
[[nodiscard]] Vec2 zoneCenter(TargetZone zone);
[[nodiscard]] Vec2 zonePoint(TargetZone zone, f64 offsetX, f64 offsetY);
[[nodiscard]] TargetZone zoneOf(const Vec2& pointM);
[[nodiscard]] const char* messageKey(TargetZone zone);

/// Presja sytuacyjna [0..1]: rośnie, gdy rzut decyduje o wyniku, a maleje przy
/// bezpiecznym prowadzeniu. Wykorzystywana przez accuracy/composure.
[[nodiscard]] f64 situationalPressure(i32 scoreDiff, i32 remainingKicks, bool isSuddenDeath,
                                     bool mustScore);

/// Wylicza błąd wykonania dla danego strzelca (deterministycznie z `rng`).
[[nodiscard]] ExecutionError rollExecutionError(const ShooterProfile& profile, f64 pressure,
                                               Random& rng);

/// Zastosowanie błędu wykonania do intencji — zwraca faktyczny strzał.
[[nodiscard]] ShotInput applyExecutionError(const ShotInput& intent, const ExecutionError& error,
                                           f64 curveSkill);

/// Wybór strefy przez strzelca CPU: waży ryzyko (róg = trudniej, ale skuteczniej)
/// względem umiejętności i presji. Deterministyczny względem `rng`.
[[nodiscard]] TargetZone chooseCpuTarget(const ShooterProfile& profile, f64 pressure, Random& rng);

/// Pełna intencja strzału CPU (strefa + moc + rotacja + nawyk strony).
struct CpuShotPlan {
    ShotInput intent{};
    TargetZone zone = TargetZone::LowRight;
    f64 effort = 0.75;
    f64 habitBias = 0.0;  // [-1..1] nawyk strony — podpowiedź dla bramkarza CPU/gracza
};

[[nodiscard]] CpuShotPlan planCpuShot(const ShooterProfile& profile, f64 pressure, Random& rng);

/// Zakresy prędkości [m/s] dla UI (pasek mocy): min/max przy danej sile strzelca.
struct SpeedRange { f64 minMs; f64 maxMs; };
[[nodiscard]] SpeedRange speedRangeFor(const ShooterProfile& profile);

}  // namespace bkh
