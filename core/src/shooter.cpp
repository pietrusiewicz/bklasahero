// Implementacja profilu strzelca i planowania strzałów CPU.
// Zob. core/include/bkh/shooter.h dla kontraktu.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "bkh/shooter.h"

#include <cmath>

namespace bkh {

namespace {
f64 clamp01(f64 v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }
}  // namespace

ShooterProfile ShooterProfile::forTier(i32 tierIndex) {
    ShooterProfile s;
    const i32 tier = (tierIndex < 0) ? 0 : (tierIndex > 7 ? 7 : tierIndex);
    const f64 t = static_cast<f64>(tier);
    // B klasa → Ekstraklasa: profile rosną monotonicznie.
    const f64 base = 0.30 + 0.075 * t;
    s.power       = clamp01(base + 0.05);
    s.accuracy    = clamp01(base - 0.02);
    s.composure   = clamp01(base + 0.02);
    s.curve       = clamp01(base - 0.04);
    s.consistency = clamp01(base);
    return s;
}

ShooterProfile ShooterProfile::fromAttributes(f64 power, f64 accuracy, f64 composure, f64 curve) {
    ShooterProfile s;
    s.power = clamp01(power);
    s.accuracy = clamp01(accuracy);
    s.composure = clamp01(composure);
    s.curve = clamp01(curve);
    s.consistency = clamp01(0.5 * (s.accuracy + s.composure));
    return s;
}

f64 ShooterProfile::shotSpeed(f64 effort) const {
    const f64 e = clamp01(effort);
    const f64 base = 15.0 + 13.0 * power;        // [15..28] m/s
    const f64 v = base * (0.75 + 0.40 * e);      // [0.75..1.15] mnożnik
    return clamp(v, 10.0, 34.0);
}

f64 ShooterProfile::aimSigmaM(f64 pressure) const {
    const f64 base = (0.75 - 0.55 * accuracy);   // [0.20..0.75] m
    const f64 pressMul = 1.0 + 0.9 * clamp01(pressure) * (1.0 - composure);
    return std::max(0.10, base * pressMul);
}

ExecutionError rollExecutionError(const ShooterProfile& profile, f64 pressure, Random& rng) {
    const f64 sigma = profile.aimSigmaM(pressure);
    ExecutionError err;
    err.dxM = rng.boundedNormal(0.0, sigma, 2.5);
    err.dyM = rng.boundedNormal(0.0, sigma * 0.8, 2.5);
    const f64 speedSigma = 0.05 * (1.2 - profile.consistency);
    err.speedFactor = 1.0 + rng.boundedNormal(0.0, std::max(speedSigma, 0.01), 2.5);
    const f64 spinSigma = 0.15 * (1.2 - profile.consistency);
    err.spinFactor = 1.0 + rng.boundedNormal(0.0, std::max(spinSigma, 0.05), 2.5);
    return err;
}

ShotInput applyExecutionError(const ShotInput& intent, const ExecutionError& error, f64 curveSkill) {
    ShotInput out = intent;
    out.aimM.x += error.dxM;
    out.aimM.y += error.dyM;
    out.speedMs *= clamp(error.speedFactor, 0.7, 1.15);
    const f64 spinMul = clamp(error.spinFactor * (0.6 + 0.4 * clamp01(curveSkill)), 0.4, 1.6);
    out.sideSpinRps *= spinMul;
    out.topSpinRps *= spinMul;
    return out;
}

Vec2 zoneCenter(TargetZone zone) {
    static constexpr f64 kX[3] = {-2.6, 0.0, 2.6};
    static constexpr f64 kY[3] = {2.0, 1.2, 0.45};
    const int c = static_cast<int>(zone) % 3;
    const int r = static_cast<int>(zone) / 3;
    return {kX[c], kY[r]};
}

Vec2 zonePoint(TargetZone zone, f64 offsetX, f64 offsetY) {
    const Vec2 c = zoneCenter(zone);
    return {c.x + clamp(offsetX, -1.0, 1.0) * 0.80, c.y + clamp(offsetY, -1.0, 1.0) * 0.80};
}

TargetZone zoneOf(const Vec2& pointM) {
    const f64 x = pointM.x, y = pointM.y;
    const int col = x < -1.7 ? 0 : (x > 1.7 ? 2 : 1);
    const int row = y > 1.55 ? 0 : (y < 0.85 ? 2 : 1);
    return static_cast<TargetZone>(row * 3 + col);
}

const char* messageKey(TargetZone zone) {
    switch (zone) {
        case TargetZone::TopLeft: return "zone.top_left";
        case TargetZone::TopCenter: return "zone.top_center";
        case TargetZone::TopRight: return "zone.top_right";
        case TargetZone::MidLeft: return "zone.mid_left";
        case TargetZone::MidCenter: return "zone.mid_center";
        case TargetZone::MidRight: return "zone.mid_right";
        case TargetZone::LowLeft: return "zone.low_left";
        case TargetZone::LowCenter: return "zone.low_center";
        case TargetZone::LowRight: return "zone.low_right";
    }
    return "zone.unknown";
}

f64 situationalPressure(i32 scoreDiff, i32 remainingKicks, bool isSuddenDeath,
                       bool mustScore) {
    f64 p = 0.10;
    if (mustScore) p += 0.45;
    if (isSuddenDeath) p += 0.25;
    if (remainingKicks <= 1) p += 0.15;
    if (remainingKicks == 0) p += 0.05;
    const f64 close = static_cast<f64>(std::abs(scoreDiff));
    p += clamp01((3.0 - close) / 3.0) * 0.20;
    return clamp(p, 0.0, 1.0);
}

CpuShotPlan planCpuShot(const ShooterProfile& profile, f64 pressure, Random& rng) {
    CpuShotPlan plan;
    // Wagi stref: rogi > środek, ale presja obniża ryzyko.
    f64 weights[9];
    for (int i = 0; i < 9; ++i) {
        const int row = i / 3, col = i % 3;
        f64 w = 1.0;
        if (col != 1) w *= 1.6;                       // boki mocniejsze niż środek
        if (row == 0 || row == 2) w *= 1.3;           // rogi górne i dolne mocniejsze
        w *= 0.7 + 0.6 * profile.accuracy;            // lepsi strzelcy wybierają trudniejsze
        w *= (1.0 - 0.4 * pressure * (1.0 - profile.composure));
        weights[i] = w;
    }
    const std::size_t idx = rng.weightedPick(&weights[0], &weights[9]);
    plan.zone = static_cast<TargetZone>(idx);
    plan.intent.aimM = zonePoint(plan.zone,
                                 rng.boundedNormal(0.0, 0.30 * (1.0 - profile.accuracy), 2.0),
                                 rng.boundedNormal(0.0, 0.30 * (1.0 - profile.accuracy), 2.0));
    plan.effort = clamp01(0.55 + 0.40 * profile.power + rng.boundedNormal(0.0, 0.10, 2.0));
    plan.intent.speedMs = profile.shotSpeed(plan.effort);
    // Rotacja: lepsi strzelcy częściej dokładają podkręcenie.
    if (rng.chance(0.4 + 0.4 * profile.curve)) {
        plan.intent.sideSpinRps = rng.boundedNormal(0.0, 18.0 + 12.0 * profile.curve, 1.5);
        plan.intent.topSpinRps  = rng.boundedNormal(0.0, 10.0 +  8.0 * profile.curve, 1.5);
    } else {
        plan.intent.sideSpinRps = 0.0;
        plan.intent.topSpinRps  = 0.0;
    }
    plan.habitBias = clamp(plan.intent.aimM.x / 3.0, -1.0, 1.0);
    return plan;
}

SpeedRange speedRangeFor(const ShooterProfile& profile) {
    SpeedRange r;
    r.minMs = profile.shotSpeed(0.0);
    r.maxMs = profile.shotSpeed(1.0);
    return r;
}

}  // namespace bkh