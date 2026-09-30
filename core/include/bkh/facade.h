// B-Klasa Hero — fasada rdzenia: jedyny punkt wejścia dla mostka JNI.
//
// Dlaczego fasada, a nie dziesiątki funkcji JNI:
//   * protokół to JSON w obie strony → dodanie nowej funkcji gry NIE wymaga
//     zmiany ani bridge'a, ani sygnatur `external native`, ani kodu Kotlin
//     (wystarczy nowa wartość pola "cmd"),
//   * zero wyjątków przez granicę JNI: `command()` łapie wszystko i zwraca
//     {"ok":false,"error":{...}},
//   * jeden osobny kanał binarny dla animacji (`writeFrame`), żeby 60 fps nie
//     generowało alokacji i parsowania JSON po stronie Kotlin.
//
// PROTOKÓŁ KONTROLNY (JSON) — pełna lista poleceń w docs/PROTOCOL.md.
//   żądanie : {"cmd":"<nazwa>", ...pola}
//   sukces  : {"ok":true, "data":{...}}
//   błąd    : {"ok":false, "error":{"code":"error.xxx","message":"..."}}
//
// BUFOR KLATEK (float[]) — układ zdefiniowany niżej jako stałe `frame::*`.
// Warstwa Kotlin MUSI używać tych samych stałych (mirror w FrameBuffer.kt).
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include "bkh/career.h"
#include "bkh/league.h"
#include "bkh/place.h"
#include "bkh/result.h"
#include "bkh/shot.h"
#include "bkh/shootout.h"
#include "bkh/types.h"

namespace bkh {

/// Wersja protokołu — bumpowana przy każdej niekompatybilnej zmianie.
/// Warstwa Kotlin sprawdza ją przy starcie (rozkaz "version").
inline constexpr i32 kProtocolVersion = 1;

/// Układ bufora klatek (kanał renderujący).
namespace frame {
inline constexpr i32 kLayoutVersion = 1;
inline constexpr i32 kHeaderFloats = 8;   // [0] wersja układu
                                          // [1] liczba próbek piłki
                                          // [2] liczba próbek bramkarza
                                          // [3] czas trwania [s]
                                          // [4] czas kontaktu z bramkarzem [s] (0 = brak)
                                          // [5] czas przekroczenia linii [s] (0 = brak)
                                          // [6] punkt przecięcia z linią bramkową — x [m]
                                          // [7] punkt przecięcia z linią bramkową — y [m]
inline constexpr i32 kBallFloatsPerSample = 7;    // t, x, y, z, vx, vy, vz
inline constexpr i32 kKeeperFloatsPerSample = 6;  // t, x, y, z, diveProgress, side(0/1/2)

[[nodiscard]] inline std::size_t requiredFloats(std::size_t ballSamples, std::size_t keeperSamples) {
    return static_cast<std::size_t>(kHeaderFloats) +
           ballSamples * static_cast<std::size_t>(kBallFloatsPerSample) +
           keeperSamples * static_cast<std::size_t>(kKeeperFloatsPerSample);
}
}  // namespace frame

class Facade {
public:
    Facade();
    ~Facade();
    Facade(const Facade&) = delete;
    Facade& operator=(const Facade&) = delete;
    Facade(Facade&&) noexcept;
    Facade& operator=(Facade&&) noexcept;

    // --- Kanał kontrolny -----------------------------------------------------
    /// Wykonuje polecenie JSON i zwraca odpowiedź JSON. NIGDY nie rzuca.
    [[nodiscard]] std::string command(std::string_view jsonCommand);

    // --- Kanał renderujący ---------------------------------------------------
    /// Wypełnia bufor danymi ostatnio rozegranego rzutu.
    /// Zwraca liczbę zapisanych floatów, albo 0 gdy brak odtwarzania.
    /// Jeśli `capacity` jest za mały, zapisuje tyle, ile się mieści, i zwraca
    /// wymaganą liczbę (ujemną wartość `-required`), żeby Kotlin mógł powiększyć bufor.
    int writeFrame(float* destination, int capacity) const;
    /// Liczba floatów potrzebna dla ostatniego rzutu.
    [[nodiscard]] int frameFloatCount() const;
    /// Czy jest coś do odtworzenia (ostatni rzut ma trajektorię).
    [[nodiscard]] bool hasPlayback() const;
    /// Czyści bufor odtwarzania (po wyświetleniu animacji).
    void clearPlayback();

    // --- Dane (przekazywane przez warstwę platformową) ------------------------
    /// Ładuje katalog miejscowości z bufora bajtów (zawartość assets/places_pl.csv).
    /// Rdzeń nie otwiera plików — to celowa granica (ADR-0001).
    bool loadPlaces(const char* data, std::size_t size, std::string& errorOut);
    [[nodiscard]] bool placesLoaded() const;
    [[nodiscard]] std::size_t placesCount() const;

    // --- Save (duże bloby poza protokołem JSON poleceń) -----------------------
    [[nodiscard]] std::string saveCareerToJson(i64 savedAtEpochMs = 0) const;
    bool loadCareerFromJson(std::string_view json, std::string& errorOut);
    [[nodiscard]] bool hasCareer() const;

    // --- Metainformacje -------------------------------------------------------
    [[nodiscard]] static const char* coreVersion();
    [[nodiscard]] static i32 saveSchemaVersion();
    [[nodiscard]] static i32 protocolVersion() { return kProtocolVersion; }

    // --- Dostęp do stanu (używane przez testy integracyjne mostka) ------------
    [[nodiscard]] const PlaceCatalog* catalog() const;
    [[nodiscard]] const Career* career() const;
    [[nodiscard]] const Shootout* activeShootout() const;
    [[nodiscard]] Lang language() const;
    void setLanguage(Lang lang);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace bkh
