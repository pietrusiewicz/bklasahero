// Testy rdzenia: fizyka piłki (lot, obramowanie, klasyfikacja wyniku).
// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <cmath>

#include "bkh/physics.h"
#include "bkh/random.h"

using namespace bkh;

namespace {

constexpr f64 kLineZ = 11.0;

ShotInput straightShot(f64 aimX, f64 aimY, f64 speed = 22.0) {
    ShotInput in{};
    in.startPosM = Vec3{0.0, 0.11, 0.0};
    in.aimM = Vec2{aimX, aimY};
    in.speedMs = speed;
    return in;
}

TEST(PhysicsTest, FreeFallHitsGround) {
    // Piłka wystrzelona poziomo w próżnię powinna spaść na murawę.
    ShotInput in = straightShot(0.0, 0.0, 10.0);
    BallFlight f = simulateBallFlight(in, PhysicsParams::standard(), false);
    EXPECT_EQ(f.outcome, ShotOutcome::Stopped);
    EXPECT_GT(f.flightTimeS, 0.5);
    EXPECT_LT(f.flightTimeS, 6.0);
}

TEST(PhysicsTest, ShotToCenterOfGoalScores) {
    ShotInput in = straightShot(0.0, 1.5, 28.0);
    BallFlight f = simulateBallFlight(in, PhysicsParams::standard(), false);
    EXPECT_TRUE(f.crossedGoalLine);
    EXPECT_EQ(f.outcome, ShotOutcome::Goal);
    EXPECT_GT(f.marginXm, 0.0);
    EXPECT_GT(f.marginYm, 0.0);
}

TEST(PhysicsTest, ShotWideRightMisses) {
    ShotInput in = straightShot(7.0, 1.5, 26.0);  // mocno w prawo, poza słupek
    BallFlight f = simulateBallFlight(in, PhysicsParams::standard(), false);
    EXPECT_NE(f.outcome, ShotOutcome::Goal);
    EXPECT_NE(f.outcome, ShotOutcome::Saved);
    EXPECT_NE(f.outcome, ShotOutcome::WoodworkIn);
}

TEST(PhysicsTest, ShotTooLowGoesUnderBar) {
    // Próba zbyt płaska → odbije się od murawy zanim doleci do bramki.
    ShotInput in = straightShot(0.0, 0.05, 14.0);
    BallFlight f = simulateBallFlight(in, PhysicsParams::standard(), false);
    EXPECT_TRUE(f.bouncedOnGround);
}

TEST(PhysicsTest, WoodworkIsDetected) {
    // Wyceluj w sam słupek lewy (x = -3.66, y = 2.0) → powinno dotknąć obramowania.
    ShotInput in = straightShot(-3.66, 2.0, 26.0);
    BallFlight f = simulateBallFlight(in, PhysicsParams::standard(), false);
    EXPECT_TRUE(f.touchedWoodwork);
}

TEST(PhysicsTest, CrossbarIsDetected) {
    ShotInput in = straightShot(0.0, 2.5, 26.0);  // tuż nad poprzeczkę
    BallFlight f = simulateBallFlight(in, PhysicsParams::standard(), false);
    // Powinno albo dotknąć poprzeczki, albo wyjść nad.
    EXPECT_TRUE(f.touchedWoodwork || f.outcome == ShotOutcome::OverBar);
}

TEST(PhysicsTest, MagnusCurvesTrajectory) {
    // Silne podkręcenie boczne powinno zakrzywić tor w prawo.
    ShotInput leftAim = straightShot(-3.0, 1.5, 26.0);
    leftAim.sideSpinRps = 60.0;
    BallFlight l = simulateBallFlight(leftAim, PhysicsParams::standard(), false);
    ShotInput rightAim = straightShot(-3.0, 1.5, 26.0);
    rightAim.sideSpinRps = -60.0;
    BallFlight r = simulateBallFlight(rightAim, PhysicsParams::standard(), false);
    EXPECT_NE(l.outcome, r.outcome);
}

TEST(PhysicsTest, SamplesContainBallAndCrossing) {
    ShotInput in = straightShot(0.0, 1.5, 26.0);
    BallFlight f = simulateBallFlight(in, PhysicsParams::standard(), true);
    EXPECT_GE(f.samples.size(), 2u);
    if (f.crossedGoalLine) {
        EXPECT_GT(f.crossingTimeS, 0.0);
        EXPECT_GE(f.crossingSpeedMs, 0.0);
    }
}

TEST(PhysicsTest, MakeBallStateNeverZero) {
    // Przy celu tuż nad głową bramkarza model musi znaleźć kąt.
    ShotInput in = straightShot(0.0, 2.4, 22.0);
    BallState s = makeBallState(in);
    EXPECT_GT(length(s.velocityMs), 0.0);
}

TEST(PhysicsTest, MessageKeysAreStable) {
    EXPECT_STREQ(messageKey(ShotOutcome::Goal), "outcome.goal");
    EXPECT_STREQ(messageKey(ShotOutcome::Saved), "outcome.saved");
    EXPECT_STREQ(messageKey(Woodwork::Crossbar), "woodwork.crossbar");
}

}  // namespace