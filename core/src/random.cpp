// Implementacja deterministycznego RNG: xoshiro256** + splitmix64.
//
// UWAGA o przenośności: świadomie NIE używamy __int128 (dostępny w GCC/clang,
// ale poza standardem i sypie ostrzeżeniami przy -Wpedantic). Dlatego
// uniformInt/pickIndex używają próbkowania odrzucającego (rejection sampling) —
// koszt to średnio <1,0001 losowania na wywołanie.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "bkh/random.h"

#include <cmath>

namespace bkh {
namespace {

constexpr u64 kGoldenGamma = 0x9E3779B97F4A7C15ULL;  // stała złotego podziału
constexpr u64 kSplitmixA   = 0xBF58476D1CE4E5B9ULL;
constexpr u64 kSplitmixB   = 0x94D049BB133111EBULL;
constexpr u64 kXoshiroJump = 0xD1B54A32D192ED03ULL;
constexpr u64 kStreamMix   = 0xC2B2AE3D27D4EB4FULL;
constexpr f64 kInvPow2_53  = 1.0 / 9007199254740992.0;  // 2^-53

/// Próg odrzucania dla zakresu `range` (Lemire bez mnożenia 128-bitowego).
constexpr u64 rejectionThreshold(u64 range) {
    const u64 negated = 0ULL - range;
    return negated % range;
}

}  // namespace

u64 Random::splitmix64(u64& z) {
    z += kGoldenGamma;
    u64 x = z;
    x = (x ^ (x >> 30)) * kSplitmixA;
    x = (x ^ (x >> 27)) * kSplitmixB;
    return x ^ (x >> 31);
}

void Random::reseed(u64 seed) {
    seed_ = seed;
    u64 z = seed;
    for (u64& slot : state_) slot = splitmix64(z);
    if (state_[0] == 0 && state_[1] == 0 && state_[2] == 0 && state_[3] == 0) {
        state_[0] = kXoshiroJump;  // stan zerowy to punkt stały
    }
    hasCachedNormal_ = false;
    cachedNormal_ = 0.0;
    draws_ = 0;
}

Random Random::fork(u64 streamTag) const {
    u64 mixed = state_[0] ^ rotl(state_[1], 17);
    mixed = rotl(mixed, 23) + state_[2] + kXoshiroJump;
    mixed ^= state_[3] * kGoldenGamma;
    mixed ^= streamTag * kStreamMix;
    mixed ^= rotl(mixed, 31);
    return Random{mixed};
}

u64 Random::nextU64() {
    ++draws_;
    const u64 result = rotl(state_[1] * 5ULL, 7) * 9ULL;
    const u64 t = state_[1] << 17;
    state_[2] ^= state_[0];
    state_[3] ^= state_[1];
    state_[1] ^= state_[2];
    state_[0] ^= state_[3];
    state_[2] ^= t;
    state_[3] = rotl(state_[3], 45);
    return result;
}

f64 Random::uniform() {
    const u64 bits = nextU64() >> 11;
    return static_cast<f64>(bits) * kInvPow2_53;
}

f64 Random::uniform(f64 a, f64 b) {
    if (!(b > a)) return a;
    const f64 u = uniform();
    const f64 value = a + (b - a) * u;
    return value < b ? value : std::nextafter(b, 0.0);
}

i32 Random::uniformInt(i32 lo, i32 hi) {
    if (hi < lo) return lo;
    const u64 range = static_cast<u64>(static_cast<i64>(hi) - static_cast<i64>(lo)) + 1ULL;
    if (range == 0) return lo;
    const u64 threshold = rejectionThreshold(range);
    u64 x = nextU64();
    while (x < threshold) x = nextU64();
    const i64 offset = static_cast<i64>(x % range);
    return static_cast<i32>(static_cast<i64>(lo) + offset);
}

bool Random::chance(f64 p) {
    if (!(p > 0.0)) return false;
    if (p >= 1.0) return true;
    return uniform() < p;
}

f64 Random::normal(f64 mean, f64 stddev) {
    if (!(stddev > 0.0)) return mean;
    if (hasCachedNormal_) {
        hasCachedNormal_ = false;
        const f64 value = cachedNormal_;
        cachedNormal_ = 0.0;
        return mean + stddev * value;
    }
    const f64 u1 = 1.0 - uniform();  // ∈ (0, 1] — unikamy log(0)
    const f64 u2 = uniform();
    const f64 radius = std::sqrt(-2.0 * std::log(u1));
    const f64 theta = 2.0 * kPi * u2;
    cachedNormal_ = radius * std::sin(theta);
    hasCachedNormal_ = true;
    return mean + stddev * (radius * std::cos(theta));
}

f64 Random::boundedNormal(f64 mean, f64 stddev, f64 k) {
    if (!(stddev > 0.0)) return mean;
    const f64 limit = (k > 0.0 ? k : 3.0) * stddev;
    for (int i = 0; i < 3; ++i) {
        const f64 v = normal(0.0, stddev);
        if (std::abs(v) <= limit) return mean + v;
    }
    return mean + clamp(normal(0.0, stddev), -limit, limit);
}

std::size_t Random::pickIndex(std::size_t n) {
    if (n <= 1) return 0;
    const u64 range = static_cast<u64>(n);
    const u64 threshold = rejectionThreshold(range);
    u64 x = nextU64();
    while (x < threshold) x = nextU64();
    return static_cast<std::size_t>(x % range);
}

}  // namespace bkh