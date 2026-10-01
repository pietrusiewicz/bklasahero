// Implementacja fizyki piłki i lotu (zob. ADR-0003-ball-physics.md).
//
// Model: punkt materialny z rotacją; siły — grawitacja, opór powietrza,
// efekt Magnusa (a = C_m · ω × v). Całkowanie pół-jawnym Eulerem ze sztywnym
// krokiem. Kolizje: murawa, obramowanie (słupki/poprzeczka jako walce).
// Klasyfikacja wyniku w chwili przekroczenia płaszczyzny z = kPenaltyDistance.
//
// Jednostki: [m], [s], [rad/s]. Układ: x w bok, y w górę, z w stronę bramki.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "bkh/physics.h"

#include <cmath>

namespace bkh {

PhysicsParams PhysicsParams::competitive() {
    PhysicsParams p;
    p.dragCoefficient = 0.23;
    p.magnusCoefficient = 0.0045;
    p.woodworkRestitution = 0.68;
    return p;
}

f64 ShotInput::launchElevationDeg() const {
    const f64 dz = static_cast<f64>(pitch::kPenaltyDistance) - startPosM.z;
    const f64 dx = aimM.x - startPosM.x;
    const f64 dy = aimM.y - startPosM.y;
    const f64 horiz = std::sqrt(dx * dx + dz * dz);
    if (!(horiz > 1e-6)) return 0.0;
    return radToDeg(std::atan2(dy, horiz));
}

namespace {

/// Minimalna prędkość [m/s], by trafić punkt (h, dy) na zadanej odległości poziomej.
f64 minSpeedForTarget(f64 horizDist, f64 vertDelta) {
    const f64 g = 9.80665;
    if (!(horizDist > 0.0)) return 0.0;
    const f64 inside = vertDelta + std::sqrt(horizDist * horizDist + vertDelta * vertDelta);
    if (!(inside > 0.0)) return 0.0;
    return std::sqrt(g * inside);
}

}  // namespace

BallState makeBallState(const ShotInput& input) {
    BallState state;
    state.positionM = input.startPosM;
    state.velocityMs = Vec3{};
    state.spinRps = Vec3{input.topSpinRps, input.sideSpinRps, 0.0};
    state.timeS = 0.0;
    state.alive = true;

    const f64 targetX = input.aimM.x;
    const f64 targetY = input.aimM.y;
    const f64 targetZ = static_cast<f64>(pitch::kPenaltyDistance);

    const f64 dx = targetX - input.startPosM.x;
    const f64 dy = targetY - input.startPosM.y;
    const f64 dz = targetZ - input.startPosM.z;
    const f64 horiz = std::sqrt(dx * dx + dz * dz);

    if (!(horiz > 1e-6)) {
        const f64 sign = (dy >= 0.0) ? 1.0 : -1.0;
        state.velocityMs = Vec3{0.0, sign * input.speedMs, 0.0};
        return state;
    }

    // Równanie balistyczne bez oporu: tan θ = (v² ± sqrt(v⁴ - g(g·d² + 2·h·v²))) / (g·d)
    // Iteracyjne rozwiązanie kąta elewacji.
    const f64 g = 9.80665;
    f64 v = std::max(input.speedMs, 1.0);
    f64 theta = 0.0;
    for (int iter = 0; iter < 32; ++iter) {
        // Newton-Raphson na residuum: y(theta) - targetY = 0.
        // y = d·tan θ − g·d² / (2 v² cos² θ)
        const f64 ct = std::cos(theta);
        const f64 st = std::sin(theta);
        const f64 tn = st / ct;
        const f64 y = horiz * tn - (g * horiz * horiz) / (2.0 * v * v * ct * ct);
        const f64 residual = y - dy;
        if (std::abs(residual) < 1e-4) break;
        // Pochodna: dy/dθ = d / cos² θ − g·d² · sin θ / (v² · cos³ θ)
        const f64 dydtheta = horiz / (ct * ct) -
                             (g * horiz * horiz * st) / (v * v * ct * ct * ct);
        if (std::abs(dydtheta) < 1e-9) break;
        const f64 step = residual / dydtheta;
        theta -= step;
        // Ogranicz kąt do rozsądnego zakresu.
        if (theta > static_cast<f64>(degToRad(75.0))) theta = degToRad(75.0);
        if (theta < -static_cast<f64>(degToRad(15.0))) theta = -degToRad(15.0);
    }

    // Jeśli Newton się nie zbiegł (resztkowe residuum duże), podnieś v do minimum.
    const f64 ct = std::cos(theta);
    const f64 tn = std::tan(theta);
    const f64 yAtTheta = horiz * tn - (g * horiz * horiz) / (2.0 * v * v * ct * ct);
    if (!(std::abs(yAtTheta - dy) < 1e-2)) {
        const f64 vMin = minSpeedForTarget(horiz, dy);
        if (vMin > v) v = vMin * 1.02;
    }

    const f64 cosT = std::cos(theta);
    const f64 sinT = std::sin(theta);
    state.velocityMs = Vec3{
        (dx / horiz) * v * cosT,
        v * sinT,
        (dz / horiz) * v * cosT,
    };
    return state;
}

IntegrationEvent integrateBall(BallState& s, f64 dt, const PhysicsParams& p) {
    IntegrationEvent ev;

    s.spinRps *= std::exp(-p.airSpinDecay * dt);

    const Vec3 v = s.velocityMs;
    const f64 speed = length(v);
    Vec3 accel{0.0, -p.gravity, 0.0};

    const f64 area = kPi * pitch::kBallRadius * pitch::kBallRadius;
    const f64 dragCoeff = 0.5 * p.airDensity * p.dragCoefficient * area / pitch::kBallMass;
    if (speed > 1e-6) accel += v * (-dragCoeff * speed);

    accel += cross(s.spinRps, v) * p.magnusCoefficient;

    s.velocityMs += accel * dt;
    s.positionM += s.velocityMs * dt;
    s.timeS += dt;

    const f64 ballR = pitch::kBallRadius;
    const f64 lineZ = static_cast<f64>(pitch::kPenaltyDistance);
    const f64 postR = pitch::kPostRadius;

    // Kolizje z obramowaniem (słupki/poprzeczka) — szybki test: piłka w
    // cylindrycznej strefie wokół osi obramowania.
    const f64 distToGoalPlane = std::abs(s.positionM.z - lineZ);
    if (distToGoalPlane < postR + ballR + 0.10) {
        for (int post = 0; post < 2; ++post) {
            const f64 postX = (post == 0 ? -1.0 : 1.0) * static_cast<f64>(pitch::kHalfGoalWidth);
            if (s.positionM.y >= -0.2 && s.positionM.y <= static_cast<f64>(pitch::kGoalHeight) + postR + ballR + 0.1) {
                const f64 dx_p = s.positionM.x - postX;
                const f64 dz_p = s.positionM.z - lineZ;
                const f64 rSum = postR + ballR;
                const f64 distSq = dx_p * dx_p + dz_p * dz_p;
                if (distSq < rSum * rSum) {
                    const f64 dist = std::sqrt(distSq);
                    if (dist > 1e-9) {
                        const Vec3 n{dx_p / dist, 0.0, dz_p / dist};
                        s.positionM += n * (rSum - dist);
                        const f64 vDotN = dot(s.velocityMs, n);
                        if (vDotN < 0.0) {
                            s.velocityMs -= n * (vDotN * (1.0 + p.woodworkRestitution));
                            s.velocityMs.y *= 0.85;
                        }
                        ev.woodwork = post == 0 ? Woodwork::LeftPost : Woodwork::RightPost;
                    }
                }
            }
        }
        if (s.positionM.x >= -static_cast<f64>(pitch::kHalfGoalWidth) - postR &&
            s.positionM.x <=  static_cast<f64>(pitch::kHalfGoalWidth) + postR) {
            const f64 dy_b = s.positionM.y - static_cast<f64>(pitch::kGoalHeight);
            const f64 dz_b = s.positionM.z - lineZ;
            const f64 rSum = postR + ballR;
            const f64 distSq = dy_b * dy_b + dz_b * dz_b;
            if (distSq < rSum * rSum) {
                const f64 dist = std::sqrt(distSq);
                if (dist > 1e-9) {
                    const Vec3 n{0.0, dy_b / dist, dz_b / dist};
                    s.positionM += n * (rSum - dist);
                    const f64 vDotN = dot(s.velocityMs, n);
                    if (vDotN < 0.0) {
                        s.velocityMs -= n * (vDotN * (1.0 + p.woodworkRestitution));
                    }
                    ev.woodwork = Woodwork::Crossbar;
                }
            }
        }
    }

    if (s.positionM.y < ballR && s.velocityMs.y < 0.0) {
        s.positionM.y = ballR;
        s.velocityMs.y = -s.velocityMs.y * p.groundRestitution;
        s.velocityMs.x *= p.groundFriction;
        s.velocityMs.z *= p.groundFriction;
        s.spinRps *= p.spinDampingOnBounce;
        ev.groundBounce = true;
    }

    if (s.positionM.z > lineZ + 3.5 || s.positionM.z < -2.0 ||
        std::abs(s.positionM.x) > 18.0 || s.positionM.y > 8.0 || s.positionM.y < -1.0) {
        ev.outOfPlay = true;
    }
    if (s.positionM.y <= ballR + 1e-3 && length(s.velocityMs) < p.stopSpeed) {
        ev.stopped = true;
        s.alive = false;
    }

    return ev;
}

BallFlight simulateBallFlight(const ShotInput& input, const PhysicsParams& params, bool recordSamples) {
    BallFlight result;
    BallState state = makeBallState(input);
    const f64 dt = params.timeStep;
    const f64 lineZ = static_cast<f64>(pitch::kPenaltyDistance);

    result.maxSpeedMs = length(state.velocityMs);
    f64 sampleAccum = 0.0;

    if (recordSamples) {
        result.samples.push_back({0.0, state.positionM, state.velocityMs});
    }

    Vec3 prevPos = state.positionM;

    while (state.alive && state.timeS < params.maxFlightTime) {
        const IntegrationEvent ev = integrateBall(state, dt, params);
        sampleAccum += dt;
        const f64 lastSpeed = length(state.velocityMs);
        if (lastSpeed > result.maxSpeedMs) result.maxSpeedMs = lastSpeed;

        if (recordSamples && sampleAccum >= params.sampleInterval) {
            sampleAccum = 0.0;
            result.samples.push_back({state.timeS, state.positionM, state.velocityMs});
        }

        if (prevPos.z < lineZ && state.positionM.z >= lineZ && !result.crossedGoalLine) {
            const f64 dzStep = state.positionM.z - prevPos.z;
            const f64 t = dzStep > 1e-9 ? (lineZ - prevPos.z) / dzStep : 0.0;
            const Vec3 p = prevPos + (state.positionM - prevPos) * t;
            result.crossedGoalLine = true;
            result.crossingPointM = p;
            result.crossingTimeS = state.timeS;
            result.crossingSpeedMs = lastSpeed;
            result.marginXm = (static_cast<f64>(pitch::kHalfGoalWidth) - pitch::kBallRadius) - std::abs(p.x);
            result.marginYm = (static_cast<f64>(pitch::kGoalHeight) - pitch::kBallRadius) - p.y;
            break;
        }

        if (ev.woodwork != Woodwork::None) result.touchedWoodwork = true;
        if (ev.groundBounce) result.bouncedOnGround = true;
        if (ev.outOfPlay || ev.stopped) break;

        prevPos = state.positionM;
    }

    result.flightTimeS = state.timeS;
    result.speedAtCrossingMs = result.crossingSpeedMs;

    if (!result.crossedGoalLine) {
        if (state.positionM.y > static_cast<f64>(pitch::kGoalHeight)) {
            result.outcome = ShotOutcome::OverBar;
        } else if (state.positionM.x < -static_cast<f64>(pitch::kHalfGoalWidth)) {
            result.outcome = ShotOutcome::WideLeft;
        } else if (state.positionM.x > static_cast<f64>(pitch::kHalfGoalWidth)) {
            result.outcome = ShotOutcome::WideRight;
        } else if (!state.alive && result.bouncedOnGround) {
            result.outcome = ShotOutcome::Stopped;
        } else {
            result.outcome = ShotOutcome::Stopped;
        }
    } else {
        const Vec2 cp{result.crossingPointM.x, result.crossingPointM.y};
        if (goal::isInsideFrame(cp)) {
            result.outcome = result.touchedWoodwork ? ShotOutcome::WoodworkIn : ShotOutcome::Goal;
        } else {
            if (cp.y > static_cast<f64>(pitch::kGoalHeight)) {
                result.outcome = result.touchedWoodwork ? ShotOutcome::WoodworkOut : ShotOutcome::OverBar;
            } else if (cp.x < 0.0) {
                result.outcome = result.touchedWoodwork ? ShotOutcome::WoodworkOut : ShotOutcome::WideLeft;
            } else {
                result.outcome = result.touchedWoodwork ? ShotOutcome::WoodworkOut : ShotOutcome::WideRight;
            }
        }
    }

    result.woodwork = Woodwork::None;
    if (result.touchedWoodwork) {
        if (result.crossedGoalLine) {
            result.woodwork = result.crossingPointM.x < 0.0
                                  ? Woodwork::LeftPost
                                  : (result.crossingPointM.x > 0.0 ? Woodwork::RightPost : Woodwork::Crossbar);
        } else {
            result.woodwork = Woodwork::Crossbar;  // bliższa półka w razie wątpliwości
        }
    }
    return result;
}

const char* messageKey(ShotOutcome outcome) {
    switch (outcome) {
        case ShotOutcome::Goal: return "outcome.goal";
        case ShotOutcome::Saved: return "outcome.saved";
        case ShotOutcome::WoodworkIn: return "outcome.woodwork_in";
        case ShotOutcome::WoodworkOut: return "outcome.woodwork_out";
        case ShotOutcome::WideLeft: return "outcome.wide_left";
        case ShotOutcome::WideRight: return "outcome.wide_right";
        case ShotOutcome::OverBar: return "outcome.over_bar";
        case ShotOutcome::Stopped: return "outcome.stopped";
    }
    return "outcome.unknown";
}

const char* messageKey(Woodwork woodwork) {
    switch (woodwork) {
        case Woodwork::None: return "woodwork.none";
        case Woodwork::LeftPost: return "woodwork.left_post";
        case Woodwork::RightPost: return "woodwork.right_post";
        case Woodwork::Crossbar: return "woodwork.crossbar";
    }
    return "woodwork.unknown";
}

}  // namespace bkh