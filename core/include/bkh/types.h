// B-Klasa Hero — podstawowe typy przenośnego rdzenia.
//
// Ten plik NIE może zawierać żadnych zależności platformowych (JNI, Android, STL
// kontenery specyficzne dla kompilatora). Jedyna używana biblioteka to <cmath>
// i nagłówki standardowe.
//
// Konwencje:
//   * wszystkie obliczenia fizyczne w `double` (f64) — unikamy cichych konwersji
//     i różnic precyzji między hostem (aarch64) a CI (x86_64);
//   * jednostki: metry, sekundy, kilogramy, radiany (jawne sufiksy w nazwach pól);
//   * układ współrzędnych boiska (prawoskrętny, „oczy strzelca"):
//         x — w bok, dodatni w PRAWO z perspektywy strzelca [m]
//         y — w górę, 0 = murawa [m]
//         z — w głąb, od piłki (punkt karny) w stronę bramki [m]
//     Piłka startuje w z = 0, linia bramkowa jest w z = kPenaltyDistance (11 m).
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

namespace bkh {

using f64 = double;
using i32 = std::int32_t;
using i64 = std::int64_t;
using u8 = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

inline constexpr f64 kPi = 3.14159265358979323846;

// ---------------------------------------------------------------------------
// Wymiary boiska i piłki (wartości oficjalne FIFA/IFAB)
// ---------------------------------------------------------------------------
namespace pitch {
/// Szerokość bramki (odstęp między wewnętrznymi krawędziami słupków) [m].
inline constexpr f64 kGoalWidth = 7.32;
/// Wysokość bramki (dolna krawędź poprzeczki nad murawą) [m].
inline constexpr f64 kGoalHeight = 2.44;
/// Odległość punktu karnego od linii bramkowej [m].
inline constexpr f64 kPenaltyDistance = 11.0;
/// Promień słupka / poprzeczki [m].
inline constexpr f64 kPostRadius = 0.06;
/// Głębokość „wnęki" bramki, w której piłka jest jeszcze w grze [m].
inline constexpr f64 kGoalDepth = 2.0;
/// Promień piłki (rozmiar 5) [m].
inline constexpr f64 kBallRadius = 0.11;
/// Masa piłki (rozmiar 5) [kg].
inline constexpr f64 kBallMass = 0.43;
/// Połowa szerokości bramki — często używana granica.
inline constexpr f64 kHalfGoalWidth = kGoalWidth / 2.0;
}  // namespace pitch

// ---------------------------------------------------------------------------
// Wektory
// ---------------------------------------------------------------------------
struct Vec2 {
    f64 x{};
    f64 y{};

    constexpr Vec2() = default;
    constexpr Vec2(f64 x_, f64 y_) : x(x_), y(y_) {}

    constexpr Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator*(f64 s) const { return {x * s, y * s}; }
    constexpr Vec2 operator/(f64 s) const { return {x / s, y / s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator*=(f64 s) { x *= s; y *= s; return *this; }
    constexpr bool operator==(const Vec2&) const = default;
};

struct Vec3 {
    f64 x{};
    f64 y{};
    f64 z{};

    constexpr Vec3() = default;
    constexpr Vec3(f64 x_, f64 y_, f64 z_) : x(x_), y(y_), z(z_) {}

    constexpr Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator*(f64 s) const { return {x * s, y * s, z * s}; }
    constexpr Vec3 operator/(f64 s) const { return {x / s, y / s, z / s}; }
    constexpr Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(f64 s) { x *= s; y *= s; z *= s; return *this; }
    constexpr bool operator==(const Vec3&) const = default;

    /// Składowa pozioma (rzut na murawę).
    constexpr Vec2 horizontal() const { return {x, z}; }
};

constexpr f64 dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
constexpr f64 dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
constexpr Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline f64 length(const Vec3& v) { return std::sqrt(dot(v, v)); }
inline f64 length(const Vec2& v) { return std::sqrt(dot(v, v)); }
inline f64 lengthSquared(const Vec3& v) { return dot(v, v); }
inline f64 distance(const Vec3& a, const Vec3& b) { return length(a - b); }

/// Normalizacja bezpieczna: dla wektora zerowego zwraca wektor zerowy.
inline Vec3 normalized(const Vec3& v) {
    const f64 len = length(v);
    return len > 1e-12 ? v / len : Vec3{};
}

// ---------------------------------------------------------------------------
// Matematyka pomocnicza
// ---------------------------------------------------------------------------
[[nodiscard]] constexpr f64 clamp(f64 v, f64 lo, f64 hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
[[nodiscard]] constexpr f64 lerp(f64 a, f64 b, f64 t) { return a + (b - a) * t; }
[[nodiscard]] constexpr f64 degToRad(f64 deg) { return deg * kPi / 180.0; }
[[nodiscard]] constexpr f64 radToDeg(f64 rad) { return rad * 180.0 / kPi; }

/// Interpolacja liniowa wektorów.
[[nodiscard]] constexpr Vec3 lerp(const Vec3& a, const Vec3& b, f64 t) {
    return a + (b - a) * t;
}

/// Zaokrąglenie do najbliższej liczby całkowitej z jawnym rzutem (bez UB przy NaN).
[[nodiscard]] inline i32 roundToInt(f64 v) {
    if (!(v > -2147483648.0 && v < 2147483647.0)) {
        return 0;
    }
    return static_cast<i32>(std::lround(v));
}

/// Stabilny, niezależny od platformy hash FNV-1a dla stringów (używany m.in. do
/// deterministycznego wyboru nazw klubów i do wersjonowania save'ów).
[[nodiscard]] inline u64 fnv1a(std::string_view text, u64 seed = 1469598103934665603ULL) {
    u64 h = seed;
    for (const char c : text) {
        h ^= static_cast<u64>(static_cast<u8>(c));
        h *= 1099511628211ULL;
    }
    return h;
}

// ---------------------------------------------------------------------------
// Język interfejsu
// ---------------------------------------------------------------------------
/// Rdzeń NIE zwraca przetłumaczonych zdań — zwraca stabilne klucze ASCII,
/// a warstwa Android mapuje je na zasoby strings.xml (PL/EN).
/// Dzięki temu tłumaczenia żyją w jednym miejscu i przechodzą review F-Droid.
enum class Lang : u8 { Pl = 0, En = 1 };

/// Nazwa języka używana w logach i eksportach.
[[nodiscard]] constexpr std::string_view langCode(Lang lang) {
    return lang == Lang::Pl ? "pl" : "en";
}

}  // namespace bkh
