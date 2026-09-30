// Testy rdzenia: model bramkarza.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include "bkh/keeper.h"
#include "bkh/random.h"

using namespace bkh;

namespace {

TEST(KeeperTest, ProfileGrowsWithTier) {
    KeeperProfile a = KeeperProfile::forTier(0, 0.0);
    KeeperProfile b = KeeperProfile::forTier(7, 1.0);
    EXPECT_LT(a.readingSkill, b.readingSkill);
    EXPECT_TRUE(b.reactionTimeS <= a.reactionTimeS);
    EXPECT_GE(b.diveSpeedMs, a.diveSpeedMs);
    EXPECT_LE(a.reactionTimeS, 0.5);
    EXPECT_LE(b.reactionTimeS, 0.5);
}

TEST(KeeperTest, FromAttributesMatchRange) {
    KeeperProfile p = KeeperProfile::fromAttributes(1.0, 1.0, 1.0, 1.0, 1.0);
    EXPECT_NEAR(p.readingSkill, 1.0, 0.01);
    EXPECT_GE(p.reactionTimeS, 0.10);
}

TEST(KeeperTest, DiveTargetReachesSides) {
    KeeperProfile p = KeeperProfile::forTier(7, 1.0);
    Vec2 l = diveTarget(DiveSide::Left, DiveHeight::Low, p);
    Vec2 r = diveTarget(DiveSide::Right, DiveHeight::High, p);
    EXPECT_LT(l.x, 0.0);
    EXPECT_GT(r.x, 0.0);
    EXPECT_LT(l.y, r.y);
}

TEST(KeeperTest, CpuKeeperSometimesReads) {
    Random rng(42);
    KeeperProfile p = KeeperProfile::forTier(5, 0.7);
    ShotInput in{};
    in.aimM = Vec2{2.0, 1.5};
    int reads = 0;
    constexpr int N = 200;
    for (int i = 0; i < N; ++i) {
        KeeperPlan plan = decideCpuKeeperDive(p, in, 0.0, rng);
        if (plan.command.side == DiveSide::Right) ++reads;
    }
    EXPECT_GT(reads, N / 4);  // bramkarz z 70% czytania kieruje się w prawo zdecydowanie częściej
}

TEST(KeeperTest, KeeperContactsWhenBallClose) {
    KeeperProfile p = KeeperProfile::forTier(3, 0.5);
    DiveCommand cmd{};
    cmd.side = DiveSide::Right;
    cmd.height = DiveHeight::Mid;
    cmd.commit = true;
    KeeperPlan plan = planKeeperDive(p, cmd);
    KeeperState st{};
    initKeeper(st, plan, p);
    // Wymuśmy czas po reakcji.
    st.timeS = plan.reactionTimeS + plan.diveDurationS * 0.8;
    stepKeeper(st, 0.016, plan, p);
    KeeperContact c = keeperContact(st, st.handsM, plan);
    EXPECT_TRUE(c.touched);
    EXPECT_FALSE(c.byBody);
}

TEST(KeeperTest, CatchIsPossibleAtSlowSpeed) {
    Random rng(1);
    KeeperProfile p = KeeperProfile::forTier(3, 0.5);
    KeeperContact c{};
    c.touched = true;
    c.byBody = false;
    int saved = 0;
    constexpr int N = 200;
    for (int i = 0; i < N; ++i) {
        if (catchesBall(c, p, 10.0, rng)) ++saved;
    }
    EXPECT_GT(saved, N / 4);
}

TEST(KeeperTest, DeflectedBallHasReducedSpeed) {
    Random rng(1);
    Vec3 v{0, 0, 25};
    Vec3 keeperHands{0, 1.5, 10.7};
    Vec3 contact{0.1, 1.5, 10.7};
    Vec3 after = deflectBall(v, contact, keeperHands, rng);
    EXPECT_LE(length(after), length(v) * 0.7);
}

}  // namespace