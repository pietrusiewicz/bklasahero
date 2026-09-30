// Implementacja modelu bramkarza (kinematyka nurkowania, decyzja CPU).
// Zob. core/include/bkh/keeper.h dla kontraktu.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "bkh/keeper.h"

#include <cmath>

namespace bkh {

KeeperProfile KeeperProfile::forTier(i32 tierIndex, f64 progression) {
    auto clamp01 = [](f64 v) {
        if (!(v > 0.0)) return 0.0;
        if (v > 1.0) return 1.0;
        return v;
    };
    const i32 tier = (tierIndex < 0) ? 0 : (tierIndex > 7 ? 7 : tierIndex);
    const f64 p = clamp01(progression);
    KeeperProfile kp;
    kp.reactionTimeS   = 0.34 - 0.028 * static_cast<f64>(tier) - 0.04 * p;
    kp.diveSpeedMs     = 4.6  + 0.18  * static_cast<f64>(tier) + 0.30 * p;
    kp.standingReachM  = 2.20 + 0.025 * static_cast<f64>(tier);
    kp.lateralReachM   = 0.85 + 0.025 * static_cast<f64>(tier);
    kp.diveExtensionM  = 2.30 + 0.045 * static_cast<f64>(tier);
    kp.readingSkill    = clamp01(0.28 + 0.075 * static_cast<f64>(tier) + 0.10 * p);
    kp.handlingSkill   = clamp01(0.35 + 0.080 * static_cast<f64>(tier) + 0.05 * p);
    kp.composure       = clamp01(0.35 + 0.070 * static_cast<f64>(tier) + 0.10 * p);
    kp.lowBallReflex   = clamp01(0.40 + 0.070 * static_cast<f64>(tier));
    if (kp.reactionTimeS < 0.10) kp.reactionTimeS = 0.10;
    return kp;
}

KeeperProfile KeeperProfile::fromAttributes(f64 reflexes, f64 reach, f64 reading,
                                           f64 composure, f64 handling) {
    auto cl = [](f64 v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); };
    KeeperProfile kp;
    kp.reactionTimeS  = 0.34 - 0.20 * cl(reflexes);
    kp.diveSpeedMs    = 4.6  + 1.4  * cl(reflexes);
    kp.standingReachM = 2.20 + 0.20 * cl(reach);
    kp.lateralReachM  = 0.85 + 0.20 * cl(reach);
    kp.diveExtensionM = 2.30 + 0.30 * cl(reach);
    kp.readingSkill   = cl(reading);
    kp.handlingSkill  = cl(handling);
    kp.composure      = cl(composure);
    kp.lowBallReflex  = cl(0.5 * (reflexes + composure));
    if (kp.reactionTimeS < 0.10) kp.reactionTimeS = 0.10;
    return kp;
}

Vec2 diveTarget(DiveSide side, DiveHeight height, const KeeperProfile& profile) {
    f64 x = 0.0;
    f64 y = 1.20;
    switch (side) {
        case DiveSide::Left:   x = -profile.diveExtensionM * 0.85; break;
        case DiveSide::Right:  x =  profile.diveExtensionM * 0.85; break;
        case DiveSide::Center: x = 0.0; break;
    }
    switch (height) {
        case DiveHeight::Low:  y = 0.45; break;
        case DiveHeight::Mid:  y = 1.20; break;
        case DiveHeight::High: y = std::min(profile.standingReachM - 0.10, 2.20); break;
    }
    return {x, y};
}

f64 effectiveReactionTime(const KeeperProfile& profile, f64 timingErrorS) {
    // Za wczesny start (timingErrorS < 0): reaguje szybciej, ale traci zasięg
    // przez wcześniejsze lądowanie. Za późny (timingErrorS > 0): reaguje później,
    // nie zdąży sięgnąć. Wyrażamy to jakościowo przez modyfikator planu.
    f64 r = profile.reactionTimeS + timingErrorS * 0.5;
    if (r < 0.05) r = 0.05;
    if (r > 0.60) r = 0.60;
    return r;
}

KeeperPlan planKeeperDive(const KeeperProfile& profile, const DiveCommand& command) {
    KeeperPlan plan;
    plan.command = command;
    plan.targetM = diveTarget(command.side, command.height, profile);
    plan.startBodyM = Vec3{0.0, 0.95, static_cast<f64>(pitch::kPenaltyDistance) - 0.30};
    plan.startHandsM = Vec3{0.0, 1.60, static_cast<f64>(pitch::kPenaltyDistance) - 0.30};
    plan.reactionTimeS = effectiveReactionTime(profile, command.timingErrorS);

    // Czas nurkowania: odległość rąk / prędkość nurkowania (z poprawką na styl).
    const f64 handsDist = std::hypot(plan.targetM.x - plan.startHandsM.x,
                                     plan.targetM.y - plan.startHandsM.y);
    const f64 diveTime = std::max(0.25, std::min(0.75, handsDist / std::max(profile.diveSpeedMs, 1.0)));
    plan.diveDurationS = diveTime;

    plan.catchRadiusM = 0.28 + 0.04 * profile.handlingSkill;
    plan.bodyRadiusM = 0.30;

    // Stay-center: nie nurkuje, czeka w środku — większa objętość ciała.
    plan.staysCenter = !command.commit;
    if (plan.staysCenter) {
        plan.reactionTimeS *= 0.85;
        plan.catchRadiusM = 0.22;
        plan.bodyRadiusM = 0.42;
        plan.targetM = Vec2{0.0, 1.10};
        plan.command.side = DiveSide::Center;
    }
    return plan;
}

KeeperPlan decideCpuKeeperDive(const KeeperProfile& profile, const ShotInput& shot,
                               f64 shooterHabitBias, Random& rng) {
    // Nawyk strzelca: dodatni bias = strzelec częściej wybiera +x.
    const f64 bias = (shooterHabitBias > 1.0) ? 1.0 : (shooterHabitBias < -1.0 ? -1.0 : shooterHabitBias);
    DiveSide side;
    DiveHeight height = DiveHeight::Mid;
    if (rng.chance(profile.readingSkill)) {
        // Odczyt strzału.
        if (shot.aimM.x < -0.2) side = DiveSide::Left;
        else if (shot.aimM.x > 0.2) side = DiveSide::Right;
        else side = DiveSide::Center;
        // Wybór wysokości z celem.
        if (shot.aimM.y > 1.7) height = DiveHeight::High;
        else if (shot.aimM.y < 0.7) height = DiveHeight::Low;
        else height = DiveHeight::Mid;
    } else {
        // Losowa decyzja, przechylona nawykiem bramkarza.
        const f64 leftW  = 0.45 - 0.20 * bias;
        const f64 centerW = 0.10;
        const f64 rightW = 1.0 - leftW - centerW;
        f64 w = rng.uniform(0.0, leftW + centerW + rightW);
        if (w < leftW) side = DiveSide::Left;
        else if (w < leftW + centerW) side = DiveSide::Center;
        else side = DiveSide::Right;
        const f64 heights[] = {0.0, 0.55, 0.35, 0.10};
        const f64 r = rng.uniform(0.0, 1.0);
        if (r < heights[1]) height = DiveHeight::Low;
        else if (r < heights[1] + heights[2]) height = DiveHeight::Mid;
        else height = DiveHeight::High;
    }

    DiveCommand cmd;
    cmd.side = side;
    cmd.height = height;
    cmd.timingErrorS = rng.boundedNormal(0.0, 0.04, 2.0);
    cmd.commit = true;
    return planKeeperDive(profile, cmd);
}

void initKeeper(KeeperState& state, const KeeperPlan& plan, const KeeperProfile&) {
    state.bodyM = plan.startBodyM;
    state.handsM = plan.startHandsM;
    state.diveProgress = 0.0;
    state.diving = false;
    state.timeS = 0.0;
}

void stepKeeper(KeeperState& state, f64 dt, const KeeperPlan& plan, const KeeperProfile&) {
    state.timeS += dt;
    const f64 reactionEnd = plan.reactionTimeS;
    if (state.timeS < reactionEnd) {
        // Oczekiwanie — delikatne kołysanie.
        const f64 phase = state.timeS * 8.0;
        state.bodyM.x = 0.02 * std::sin(phase);
        state.handsM.x = state.bodyM.x;
        return;
    }

    if (!plan.staysCenter) {
        const f64 sRaw = (state.timeS - reactionEnd) / std::max(plan.diveDurationS, 0.05);
        const f64 s = clamp(sRaw, 0.0, 1.0);
        // Krzywa "fast start": 1 - (1-s)^2.
        const f64 ease = 1.0 - (1.0 - s) * (1.0 - s);
        // Ciało podąża za rękami wolniej (do 0.5 x).
        const f64 bodyEase = 1.0 - (1.0 - ease) * (1.0 - ease);
        const Vec3 bodyTarget{plan.targetM.x * 0.55,
                              plan.command.height == DiveHeight::Low ? 0.45
                                       : plan.command.height == DiveHeight::High ? 1.50
                                                                                 : 0.95,
                              plan.startBodyM.z};
        state.bodyM = lerp(plan.startBodyM, bodyTarget, bodyEase);
        state.handsM = lerp(plan.startHandsM,
                            Vec3{plan.targetM.x, plan.targetM.y, plan.startHandsM.z}, ease);
        state.diveProgress = ease;
        state.diving = true;
    } else {
        // Stay-center — delikatne ustawienie.
        state.bodyM = lerp(state.bodyM, plan.startBodyM, clamp(dt * 5.0, 0.0, 1.0));
        state.handsM = lerp(state.handsM, plan.startHandsM, clamp(dt * 5.0, 0.0, 1.0));
    }
}

KeeperContact keeperContact(const KeeperState& state, const Vec3& ballPositionM, const KeeperPlan& plan) {
    KeeperContact contact{};
    if (state.timeS < plan.reactionTimeS) {
        return contact;
    }
    const f64 ballR = pitch::kBallRadius;
    const f64 dH = distance(state.handsM, ballPositionM) - (plan.catchRadiusM + ballR);
    const f64 dB = distance(state.bodyM, ballPositionM) - (plan.bodyRadiusM + ballR);
    if (dH <= 0.0 || dB <= 0.0) {
        contact.touched = true;
        contact.byBody = dB < dH;
        contact.timeS = state.timeS;
        contact.pointM = contact.byBody ? state.bodyM : state.handsM;
        contact.distanceM = std::min(dH, dB);
    }
    return contact;
}

bool catchesBall(const KeeperContact& contact, const KeeperProfile& profile,
                 f64 ballSpeedMs, Random& rng) {
    if (!contact.touched) return false;
    const f64 speedFactor = clamp((ballSpeedMs - 18.0) / 30.0, 0.0, 1.0);
    f64 p;
    if (contact.byBody) {
        p = 0.12 + 0.20 * profile.handlingSkill - 0.10 * speedFactor;
    } else {
        p = 0.55 + 0.30 * profile.handlingSkill - 0.20 * speedFactor;
    }
    p = clamp(p, 0.05, 0.95);
    return rng.chance(p);
}

Vec3 deflectBall(const Vec3& velocityMs, const Vec3& contactPointM, const Vec3& keeperHandsM, Random& rng) {
    // Wektor od rąk do kontaktu (piłka leciała "w ręce") → odbicie w przeciwną stronę.
    Vec3 dir = contactPointM - keeperHandsM;
    const f64 len = length(dir);
    if (len > 1e-6) {
        dir = dir / len;
    } else {
        // Zrzut awaryjny: piłka leciała dokładnie w punkt rąk.
        dir = Vec3{rng.uniform(-1.0, 1.0), rng.uniform(-1.0, 1.0), rng.uniform(-0.5, 0.5)};
        const f64 l = length(dir);
        if (l > 1e-6) dir = dir / l;
    }
    // Odbicie względem kierunku kontaktu, z silnym tłumieniem (parry).
    const f64 vDotD = dot(velocityMs, dir);
    Vec3 reflected = velocityMs - dir * (2.0 * std::max(vDotD, 0.0));
    // Skalowanie i losowe odchylenie.
    const f64 damp = 0.45;
    reflected *= damp;
    // Mały losowy "spin" z rękawicy.
    const f64 jitter = 1.5;
    reflected += Vec3{rng.boundedNormal(0.0, jitter, 2.0),
                      rng.boundedNormal(0.0, jitter, 2.0),
                      rng.boundedNormal(0.0, jitter, 2.0)};
    return reflected;
}

const char* messageKey(DiveSide side) {
    switch (side) {
        case DiveSide::Left: return "dive_side.left";
        case DiveSide::Center: return "dive_side.center";
        case DiveSide::Right: return "dive_side.right";
    }
    return "dive_side.unknown";
}

const char* messageKey(DiveHeight height) {
    switch (height) {
        case DiveHeight::Low: return "dive_height.low";
        case DiveHeight::Mid: return "dive_height.mid";
        case DiveHeight::High: return "dive_height.high";
    }
    return "dive_height.unknown";
}

}  // namespace bkh