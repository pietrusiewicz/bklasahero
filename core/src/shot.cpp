// Implementacja sprzężonej symulacji rzutu karnego (piłka + bramkarz).
// Zob. core/include/bkh/shot.h dla kontraktu i core/include/bkh/physics.h,
// core/include/bkh/keeper.h dla poszczególnych modeli.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "bkh/shot.h"

#include <cmath>
#include <limits>

namespace bkh {

ShotInput PlayerShotInput::toShotInput(const ShooterProfile& profile) const {
    ShotInput out;
    out.startPosM = ShotInput::defaultStart();
    out.aimM = aimM;
    out.effort = clamp(effort, 0.0, 1.0);
    out.speedMs = profile.shotSpeed(effort);
    const f64 maxSideSpin = 30.0 + 30.0 * profile.curve;   // rad/s
    const f64 maxTopSpin  = 18.0 + 20.0 * profile.curve;
    out.sideSpinRps = clamp(spin, -1.0, 1.0) * maxSideSpin;
    out.topSpinRps  = clamp(loft, -1.0, 1.0) * maxTopSpin;
    return out;
}

PlayerShotInput sanitize(const PlayerShotInput& input) {
    PlayerShotInput p = input;
    p.aimM.x = clamp(p.aimM.x, -6.0, 6.0);
    p.aimM.y = clamp(p.aimM.y, -1.0, 4.0);
    p.effort = clamp(p.effort, 0.0, 1.0);
    p.spin = clamp(p.spin, -1.0, 1.0);
    p.loft = clamp(p.loft, -1.0, 1.0);
    return p;
}

DiveCommand toDiveCommand(const PlayerDiveInput& input) {
    DiveCommand cmd;
    cmd.side = input.side;
    cmd.height = input.height;
    cmd.timingErrorS = clamp(input.timingErrorS, -0.30, 0.30);
    cmd.commit = input.commit;
    return cmd;
}

f64 timingErrorFromWindow(f64 offsetS, f64 windowS) {
    const f64 half = std::max(windowS * 0.5, 1e-3);
    const f64 clamped = clamp(offsetS, -half, half);
    return offsetS - clamped;
}

namespace {

/// Klasyfikacja wyniku po przecięciu linii bramkowej.
ShotOutcome classifyCrossing(const Vec3& crossingPoint, bool touchedWoodwork) {
    const Vec2 cp{crossingPoint.x, crossingPoint.y};
    if (goal::isInsideFrame(cp)) {
        return touchedWoodwork ? ShotOutcome::WoodworkIn : ShotOutcome::Goal;
    }
    if (touchedWoodwork) return ShotOutcome::WoodworkOut;
    if (cp.y > static_cast<f64>(pitch::kGoalHeight)) return ShotOutcome::OverBar;
    if (cp.x < 0.0) return ShotOutcome::WideLeft;
    return ShotOutcome::WideRight;
}

/// Klasyfikacja po "śmierci" piłki bez przekroczenia linii (zatrzymana, wyrzucona).
ShotOutcome classifyNoCrossing(const Vec3& lastPos, bool touchedWoodwork, bool bounced) {
    if (touchedWoodwork) return ShotOutcome::WoodworkOut;
    if (lastPos.y > static_cast<f64>(pitch::kGoalHeight)) return ShotOutcome::OverBar;
    if (lastPos.x < -static_cast<f64>(pitch::kHalfGoalWidth)) return ShotOutcome::WideLeft;
    if (lastPos.x >  static_cast<f64>(pitch::kHalfGoalWidth)) return ShotOutcome::WideRight;
    if (bounced) return ShotOutcome::Stopped;
    return ShotOutcome::WideRight;
}

}  // namespace

ShotExecution executeShot(const ShotInput& intent, const ShotContext& ctx, Random& rng) {
    ShotExecution exe;

    // 1. Wybór intencji.
    ShotInput actualIntent;
    if (ctx.cpuShoots) {
        const CpuShotPlan plan = planCpuShot(ctx.shooter, ctx.pressure, rng);
        actualIntent = plan.intent;
        exe.resolution.zone = plan.zone;
    } else {
        actualIntent = intent;
        exe.resolution.zone = zoneOf(intent.aimM);
    }

    // 2. Błąd wykonania. Mocniejszy strzał gracza = trudniejszy do kontroli
    //    (kompromis siła / precyzja — dzięki temu pasek mocy ma sens).
    ExecutionError err = rollExecutionError(ctx.shooter, ctx.pressure, rng);
    if (!ctx.cpuShoots) {
        const f64 effortFactor = 0.55 + 0.75 * clamp(intent.effort, 0.0, 1.0);
        err.dxM *= effortFactor;
        err.dyM *= effortFactor;
    }
    actualIntent = applyExecutionError(actualIntent, err, ctx.shooter.curve);

    // 3. Plan bramkarza.
    KeeperPlan keeperPlan;
    if (ctx.cpuKeeps) {
        keeperPlan = decideCpuKeeperDive(ctx.keeper, actualIntent, ctx.keeperHabitBias, rng);
    } else {
        keeperPlan = planKeeperDive(ctx.keeper, ctx.playerDive);
    }
    exe.resolution.keeperSide = keeperPlan.command.side;
    exe.resolution.keeperHeight = keeperPlan.command.height;
    exe.resolution.keeperTimingErrorS = keeperPlan.command.timingErrorS;

    // 4. Symulacja sprzężona.
    BallState ball = makeBallState(actualIntent);
    KeeperState keeper{};
    initKeeper(keeper, keeperPlan, ctx.keeper);

    const f64 dt = ctx.physics.timeStep;
    const f64 lineZ = static_cast<f64>(pitch::kPenaltyDistance);
    f64 sampleAccum = 0.0;
    f64 minKeeperMiss = std::numeric_limits<f64>::infinity();

    if (ctx.physics.sampleInterval > 0.0) {
        exe.playback.ball.push_back({0.0, ball.positionM, ball.velocityMs});
        exe.playback.keeper.push_back({0.0, keeper.bodyM, keeper.handsM,
                                       keeper.diveProgress, keeperPlan.command.side});
    }

    Vec3 prevPos = ball.positionM;
    bool resolution = false;
    Vec3 lastPos = ball.positionM;

    while (ball.alive && ball.timeS < ctx.physics.maxFlightTime && !resolution) {
        const IntegrationEvent bev = integrateBall(ball, dt, ctx.physics);
        stepKeeper(keeper, dt, keeperPlan, ctx.keeper);
        sampleAccum += dt;
        lastPos = ball.positionM;
        if (length(ball.velocityMs) > exe.resolution.maxSpeedMs) {
            exe.resolution.maxSpeedMs = length(ball.velocityMs);
        }

        // Kontakt z bramkarzem.
        const KeeperContact contact = keeperContact(keeper, ball.positionM, keeperPlan);
        if (contact.touched) {
            const f64 speedAtContact = length(ball.velocityMs);
            const bool caught = catchesBall(contact, ctx.keeper, speedAtContact, rng);
            exe.resolution.keeperTouched = true;
            exe.resolution.contactTimeS = contact.timeS;
            exe.playback.contactTimeS = contact.timeS;
            if (caught) {
                exe.resolution.caught = true;
                exe.resolution.outcome = ShotOutcome::Saved;
                exe.resolution.saveDistanceM = std::min(0.0, contact.distanceM);
                resolution = true;
                break;
            }
            exe.resolution.fumbled = true;
            // Odbicie piłki — kontynuujemy lot, może wpaść.
            ball.velocityMs = deflectBall(ball.velocityMs, contact.pointM, keeper.handsM, rng);
        }

        const f64 miss = contact.touched ? 0.0
                                         : (distance(keeper.handsM, ball.positionM) -
                                            (keeperPlan.catchRadiusM + pitch::kBallRadius));
        if (!contact.touched && miss < minKeeperMiss) minKeeperMiss = miss;

        // Przekroczenie linii.
        if (prevPos.z < lineZ && ball.positionM.z >= lineZ) {
            const f64 dzStep = ball.positionM.z - prevPos.z;
            const f64 t = dzStep > 1e-9 ? (lineZ - prevPos.z) / dzStep : 0.0;
            const Vec3 p = prevPos + (ball.positionM - prevPos) * t;
            const f64 crossingSpeed = length(ball.velocityMs);
            const bool touchedWood = bev.woodwork != Woodwork::None || exe.resolution.keeperTouched;
            exe.resolution.outcome = classifyCrossing(p, touchedWood);
            exe.resolution.crossedGoalLine = true;
            exe.resolution.crossingPointM = p;
            exe.resolution.crossingTimeS = ball.timeS;
            exe.resolution.crossingSpeedMs = crossingSpeed;
            exe.resolution.flightTimeS = ball.timeS;
            if (bev.woodwork != Woodwork::None) exe.resolution.woodwork = bev.woodwork;
            exe.resolution.marginXm = (static_cast<f64>(pitch::kHalfGoalWidth) - pitch::kBallRadius) - std::abs(p.x);
            exe.resolution.marginYm = (static_cast<f64>(pitch::kGoalHeight) - pitch::kBallRadius) - p.y;
            exe.playback.crossingTimeS = ball.timeS;
            exe.playback.impactPointM = Vec2{p.x, p.y};
            resolution = true;
            break;
        }

        if (bev.outOfPlay || bev.stopped) {
            const bool touchedWood = exe.resolution.keeperTouched || bev.woodwork != Woodwork::None;
            exe.resolution.outcome = classifyNoCrossing(lastPos, touchedWood, bev.groundBounce);
            if (bev.woodwork != Woodwork::None) exe.resolution.woodwork = bev.woodwork;
            exe.resolution.flightTimeS = ball.timeS;
            resolution = true;
            break;
        }

        if (sampleAccum >= ctx.physics.sampleInterval) {
            sampleAccum = 0.0;
            exe.playback.ball.push_back({ball.timeS, ball.positionM, ball.velocityMs});
            exe.playback.keeper.push_back({ball.timeS, keeper.bodyM, keeper.handsM,
                                           keeper.diveProgress, keeperPlan.command.side});
        }

        prevPos = ball.positionM;
    }

    // Domyślne uzupełnienie, jeśli pętla zakończyła się timeoutem.
    if (!resolution) {
        const bool touchedWood = exe.resolution.keeperTouched;
        exe.resolution.outcome = classifyNoCrossing(lastPos, touchedWood, false);
        exe.resolution.flightTimeS = ball.timeS;
    }

    if (exe.resolution.saveDistanceM == 0.0 && !exe.resolution.keeperTouched) {
        exe.resolution.saveDistanceM = std::isfinite(minKeeperMiss) ? std::max(0.0, minKeeperMiss) : 0.0;
    }

    exe.resolution.intendedAimM = intent.aimM;
    exe.resolution.actualAimM   = actualIntent.aimM;
    exe.resolution.speedMs      = actualIntent.speedMs;
    exe.resolution.sideSpinRps  = actualIntent.sideSpinRps;
    exe.resolution.topSpinRps   = actualIntent.topSpinRps;

    exe.playback.durationS = ball.timeS;
    exe.playback.valid = !exe.playback.ball.empty();
    return exe;
}

ShotExecution executeCpuShot(const ShotContext& ctx, Random& rng) {
    ShotContext cpuCtx = ctx;
    cpuCtx.cpuShoots = true;
    return executeShot(ShotInput{}, cpuCtx, rng);
}

}  // namespace bkh