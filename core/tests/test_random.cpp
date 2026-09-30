// Testy rdzenia: deterministyczny RNG i rozkłady.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <cmath>
#include <set>
#include <unordered_set>

#include "bkh/random.h"

using bkh::Random;
using bkh::u64;
using bkh::i32;
using bkh::f64;

namespace {

TEST(RandomTest, Determinism) {
    Random a(42), b(42);
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(a.nextU64(), b.nextU64());
    }
    for (int i = 0; i < 1000; ++i) {
        EXPECT_DOUBLE_EQ(a.uniform(), b.uniform());
    }
}

TEST(RandomTest, ForkIsIndependent) {
    Random master(7);
    Random a = master.fork(1);
    Random b = master.fork(2);
    // Zużywamy mastera — dzieci powinny zachować własny stan.
    for (int i = 0; i < 50; ++i) master.nextU64();
    const u64 firstA = a.nextU64();
    const u64 firstB = b.nextU64();
    EXPECT_NE(firstA, firstB);
}

TEST(RandomTest, UniformRange) {
    Random r(1);
    for (int i = 0; i < 10000; ++i) {
        const f64 v = r.uniform(-2.5, 1.5);
        EXPECT_GE(v, -2.5);
        EXPECT_LT(v, 1.5);
    }
}

TEST(RandomTest, UniformIntRange) {
    Random r(1);
    std::unordered_set<i32> seen;
    for (int i = 0; i < 5000; ++i) {
        const i32 v = r.uniformInt(0, 9);
        EXPECT_GE(v, 0);
        EXPECT_LE(v, 9);
        seen.insert(v);
    }
    EXPECT_EQ(seen.size(), 10u);
}

TEST(RandomTest, ChanceEdgeCases) {
    Random r(1);
    EXPECT_FALSE(r.chance(0.0));
    EXPECT_FALSE(r.chance(-1.0));
    EXPECT_TRUE(r.chance(1.0));
    EXPECT_TRUE(r.chance(2.0));
}

TEST(RandomTest, NormalMeanAndStddev) {
    Random r(1);
    constexpr int N = 50000;
    double sum = 0.0;
    double sumSq = 0.0;
    for (int i = 0; i < N; ++i) {
        const f64 v = r.normal(5.0, 2.0);
        sum += v;
        sumSq += v * v;
    }
    const f64 mean = sum / N;
    const f64 var = sumSq / N - mean * mean;
    EXPECT_NEAR(mean, 5.0, 0.08);
    EXPECT_NEAR(std::sqrt(var), 2.0, 0.10);
}

TEST(RandomTest, BoundedNormalClipped) {
    Random r(1);
    for (int i = 0; i < 10000; ++i) {
        const f64 v = r.boundedNormal(0.0, 1.0, 3.0);
        EXPECT_GE(v, -3.0 - 1e-9);
        EXPECT_LE(v,  3.0 + 1e-9);
    }
}

TEST(RandomTest, WeightedPickDistribution) {
    Random r(123);
    constexpr f64 weights[] = {0.5, 1.5, 0.2, 0.3};
    constexpr int N = 50000;
    std::vector<int> counts(4, 0);
    for (int i = 0; i < N; ++i) {
        const std::size_t idx = r.weightedPick(&weights[0], &weights[4]);
        counts[idx]++;
    }
    const f64 total = weights[0] + weights[1] + weights[2] + weights[3];
    for (int i = 0; i < 4; ++i) {
        const f64 expected = static_cast<f64>(N) * weights[i] / total;
        EXPECT_NEAR(static_cast<f64>(counts[i]), expected, expected * 0.07);
    }
}

TEST(RandomTest, PickIndexDistribution) {
    Random r(1);
    constexpr int N = 10000;
    std::vector<int> counts(5, 0);
    for (int i = 0; i < N; ++i) {
        ++counts[r.pickIndex(5)];
    }
    for (int i = 0; i < 5; ++i) {
        EXPECT_NEAR(counts[i], N / 5, N * 0.05);
    }
}

TEST(RandomTest, ShufflePreservesElements) {
    Random r(1);
    std::vector<i32> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const std::vector<int> original = v;
    r.shuffle(v.begin(), v.end());
    EXPECT_EQ(v.size(), original.size());
    std::set<int> got(v.begin(), v.end());
    std::set<int> exp(original.begin(), original.end());
    EXPECT_EQ(got, exp);
}

TEST(RandomTest, ReseedResetsState) {
    Random r(1);
    const u64 a = r.nextU64();
    const u64 b = r.nextU64();
    r.reseed(1);
    EXPECT_EQ(r.nextU64(), a);
    EXPECT_EQ(r.nextU64(), b);
}

}  // namespace