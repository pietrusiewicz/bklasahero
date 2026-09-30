// B-Klasa Hero — deterministyczny generator liczb pseudolosowych.
//
// Dlaczego własny RNG, a nie std::mt19937:
//   * std::mt19937 nie gwarantuje tego samego strumienia między implementacjami
//     biblioteki standardowej (libstdc++ vs libc++ z NDK), a save gry musi być
//     odtwarzalny na dowolnym urządzeniu;
//   * potrzebujemy jawnych podstrumieni (fork) — osobny strumień dla fizyki,
//     bramkarza, symulacji ligi — żeby zmiana jednego nie przestawiała reszty;
//   * potrzebujemy rozkładu normalnego z pamiętaniem drugiego wyniku Box-Muller.
//
// Algorytm: xoshiro256** (2^256 okres, świetna jakość, ~2 ns/losowanie),
// seedowanie przez splitmix64. Implementacja wyłącznie na uint64_t — przenośna.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>

#include "bkh/types.h"

namespace bkh {

class Random {
public:
    using State = std::array<u64, 4>;

    Random() : Random(0x5EED20260930ULL) {}
    explicit Random(u64 seed) { reseed(seed); }

    /// Nowe, niezależne ziarno dla całej kariery.
    static Random forCareer(u64 seed) { return Random(seed); }

    /// Ustawia stan z pojedynczego ziarna (splitmix64 — miesza nawet ziarno 0).
    void reseed(u64 seed);

    /// Podstrumień deterministycznie wywiedziony z bieżącego stanu i etykiety.
    /// Wynik NIE zmienia stanu tego obiektu — dzięki temu dwa podsystemy mogą
    /// losować niezależnie i powtarzalnie.
    [[nodiscard]] Random fork(u64 streamTag) const;

    [[nodiscard]] u64 nextU64();
    [[nodiscard]] u32 nextU32() { return static_cast<u32>(nextU64() >> 32); }

    /// Jednostajnie w [0, 1).
    [[nodiscard]] f64 uniform();
    /// Jednostajnie w [a, b). Dla b <= a zwraca a.
    [[nodiscard]] f64 uniform(f64 a, f64 b);
    /// Jednostajnie w [lo, hi] (oba końce włącznie). Dla hi < lo zwraca lo.
    [[nodiscard]] i32 uniformInt(i32 lo, i32 hi);
    /// Zdarzenie z prawdopodobieństwem p (przycinane do [0, 1]).
    [[nodiscard]] bool chance(f64 p);
    /// Rozkład normalny (Box-Muller) — średnia i odchylenie standardowe.
    [[nodiscard]] f64 normal(f64 mean, f64 stddev);
    /// Normalny przycięty do [mean - k*stddev, mean + k*stddev] (domyślnie 3 sigma).
    [[nodiscard]] f64 boundedNormal(f64 mean, f64 stddev, f64 k = 3.0);
    /// Indeks w [0, n). Dla n == 0 zwraca 0.
    [[nodiscard]] std::size_t pickIndex(std::size_t n);
    /// Ważony wybór: zwraca indeks pierwszego przekroczenia sumy wag.
    /// Wagi muszą być >= 0; jeśli suma jest 0, zwraca 0.
    template <typename It>
    [[nodiscard]] std::size_t weightedPick(It weightsBegin, It weightsEnd);

    /// Przetasowanie Fishera-Yatesa (w przód).
    template <typename It>
    void shuffle(It begin, It end);

    [[nodiscard]] u64 seed() const { return seed_; }
    [[nodiscard]] const State& state() const { return state_; }
    void setState(const State& s) { state_ = s; hasCachedNormal_ = false; }

    /// Licznik wywołań — przydatny w testach determinizmu i w logach.
    [[nodiscard]] u64 draws() const { return draws_; }

private:
    static u64 rotl(u64 x, int k) { return (x << k) | (x >> (64 - k)); }
    static u64 splitmix64(u64& z);

    State state_{};
    u64 seed_{};
    u64 draws_{};
    f64 cachedNormal_{};
    bool hasCachedNormal_{false};
};

// --- implementacja szablonów -----------------------------------------------

template <typename It>
std::size_t Random::weightedPick(It weightsBegin, It weightsEnd) {
    f64 total = 0.0;
    for (It it = weightsBegin; it != weightsEnd; ++it) {
        const f64 w = static_cast<f64>(*it);
        if (w > 0.0) {
            total += w;
        }
    }
    if (!(total > 0.0)) {
        return 0;
    }
    f64 target = uniform(0.0, total);
    std::size_t index = 0;
    for (It it = weightsBegin; it != weightsEnd; ++it, ++index) {
        const f64 w = static_cast<f64>(*it);
        if (w <= 0.0) {
            continue;
        }
        if (target < w) {
            return index;
        }
        target -= w;
    }
    return index > 0 ? index - 1 : 0;
}

template <typename It>
void Random::shuffle(It begin, It end) {
    const auto count = static_cast<std::size_t>(std::distance(begin, end));
    for (std::size_t i = count; i > 1; --i) {
        const std::size_t j = pickIndex(i);
        using std::swap;
        swap(*std::next(begin, static_cast<std::ptrdiff_t>(i - 1)),
             *std::next(begin, static_cast<std::ptrdiff_t>(j)));
    }
}

}  // namespace bkh
