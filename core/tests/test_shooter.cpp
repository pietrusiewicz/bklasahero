// Testy rdzenia: profil strzelca i planowanie strzału.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include "bkh/shooter.h"
#include "bkh/random.h"

using namespace bkh;

namespace {

TEST(ShooterTest, ProfilesRiseWithTier) {
    ShooterProfile a = ShooterProfile::forTier(0);
    ShooterProfile b = ShooterProfile::forTier(7);
    EXPECT_LT(a.accuracy, b.accuracy);
    EXPECT_LE(b.power, 1.0);
    EXPECT_GE(b.power, 0.0);
}

TEST(ShooterTest, AimSigmaShrinksWithAccuracy) {
    ShooterProfile precise = ShooterProfile::fromAttributes(0.5, 0.9, 0.5, 0.5);
    ShooterProfile sloppy  = ShooterProfile::fromAttributes(0.5, 0.2, 0.5, 0.5);
    EXPECT_LT(precise.aimSigmaM(0.5), sloppy.aimSigmaM(0.5));
}

TEST(ShooterTest, ExecutionErrorStaysWithinBounds) {
    Random rng(1);
    ShooterProfile p = ShooterProfile::forTier(4);
    for (int i = 0; i < 200; ++i) {
        ExecutionError e = rollExecutionError(p, 0.5, rng);
        EXPECT_LE(std::abs(e.dxM), 4.0);
        EXPECT_LE(std::abs(e.dyM), 4.0);
        EXPECT_GE(e.speedFactor, 0.5);
        EXPECT_LE(e.speedFactor, 1.2);
    }
}

TEST(ShooterTest, CpuShotHasReasonableEffort) {
    Random rng(1);
    ShooterProfile p = ShooterProfile::forTier(3);
    CpuShotPlan plan = planCpuShot(p, 0.3, rng);
    EXPECT_GE(plan.effort, 0.0);
    EXPECT_LE(plan.effort, 1.0);
    EXPECT_GE(plan.intent.speedMs, 10.0);
    EXPECT_LE(plan.intent.speedMs, 35.0);
}

TEST(ShooterTest, ZoneRoundTrip) {
    Vec2 centers[9];
    for (int i = 0; i < 9; ++i) {
        centers[i] = zoneCenter(static_cast<TargetZone>(i));
    }
    for (int i = 0; i < 9; ++i) {
        TargetZone z = zoneOf(centers[i]);
        EXPECT_EQ(static_cast<int>(z), i);
    }
}

TEST(ShooterTest, PressureIncreasesUnderStress) {
    EXPECT_LT(situationalPressure(0, 5, false, false),
              situationalPressure(0, 1, false, true));
    EXPECT_LT(situationalPressure(0, 1, false, true),
              situationalPressure(-1, 0, true, true));
}

}  // namespace