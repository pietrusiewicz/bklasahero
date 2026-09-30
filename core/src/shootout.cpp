// Implementacja maszyny stanów konkursu rzutów karnych i symulacji ligowej.
// Zob. core/include/bkh/shootout.h dla kontraktu i core/include/bkh/shot.h
// dla executeShot (używanego przy symulacjach ligi).
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "bkh/shootout.h"

#include "bkh/keeper.h"
#include "bkh/shooter.h"
#include "bkh/shot.h"

#include <cmath>

namespace bkh {

const char* messageKey(Side side) {
    switch (side) {
        case Side::Home: return "side.home";
        case Side::Away: return "side.away";
    }
    return "side.unknown";
}

const char* messageKey(PlayerRole role) {
    switch (role) {
        case PlayerRole::None: return "role.none";
        case PlayerRole::Shooter: return "role.shooter";
        case PlayerRole::Keeper: return "role.keeper";
    }
    return "role.unknown";
}

std::optional<Side> earlyWinner(i32 homeScore, i32 awayScore, i32 homeTaken, i32 awayTaken,
                               const ShootoutRules& rules) {
    (void)rules;
    // Ujemny "remaining" oznacza, że strona skończyła swoją piątkę; reguła
    // matematyczna i tak działa poprawnie (drużyna ma już "0" szans).
    const i32 remainingHome  = std::max(0, rules.kicksPerSide - homeTaken);
    const i32 remainingAway  = std::max(0, rules.kicksPerSide - awayTaken);
    if (homeScore > awayScore + remainingAway) return Side::Home;
    if (awayScore > homeScore + remainingHome) return Side::Away;
    return std::nullopt;
}

Side Shootout::nextKicker() const {
    if (isFinished()) return state_.rules.firstKicker;
    if (state_.homeTaken == state_.awayTaken) return state_.rules.firstKicker;
    return state_.homeTaken < state_.awayTaken ? Side::Home : Side::Away;
}

i32 Shootout::nextRound() const {
    if (inSuddenDeath()) return state_.suddenDeathRounds;
    return std::max(state_.homeTaken, state_.awayTaken);
}

bool Shootout::inSuddenDeath() const {
    return state_.homeTaken >= state_.rules.kicksPerSide &&
           state_.awayTaken >= state_.rules.kicksPerSide;
}

i32 Shootout::remainingRegularKicks(Side side) const {
    if (inSuddenDeath()) return 0;
    const i32 taken = side == Side::Home ? state_.homeTaken : state_.awayTaken;
    return std::max(0, state_.rules.kicksPerSide - taken);
}

bool Shootout::canStillWin(Side side) const {
    if (isFinished()) return false;
    const Side other = opponentOf(side);
    const i32 s = score(side), o = score(other);
    if (inSuddenDeath()) {
        return s >= o;  // obie strony mają jeszcze szansę
    }
    const i32 remaining = remainingRegularKicks(side);
    return s + remaining >= o;
}

bool Shootout::isMustScore(Side side) const {
    if (isFinished()) return false;
    const Side other = opponentOf(side);
    const i32 s = score(side), o = score(other);
    if (inSuddenDeath()) {
        return s < o;  // w nagłej śmierci pudło = przegrana
    }
    const i32 remaining = remainingRegularKicks(side);
    return s + remaining - 1 < o;
}

f64 Shootout::pressureFor(Side side) const {
    const Side other = opponentOf(side);
    const i32 remaining = inSuddenDeath() ? 0 : remainingRegularKicks(side);
    return situationalPressure(score(side) - score(other),
                              remaining, inSuddenDeath(), isMustScore(side));
}

void Shootout::evaluateCompletion() {
    const ShootoutRules& r = state_.rules;
    // 1. Obie strony skończyły podstawową piątkę. Remis → nagła śmierć, różnica → koniec.
    if (state_.homeTaken == r.kicksPerSide && state_.awayTaken == r.kicksPerSide) {
        if (state_.homeScore != state_.awayScore) {
            state_.status = ShootoutStatus::Finished;
            state_.winner = state_.homeScore > state_.awayScore ? Side::Home : Side::Away;
        } else if (state_.suddenDeathRounds == 0) {
            state_.suddenDeathRounds = 1;
        }
        return;
    }
    // 2. W trakcie nagłej śmierci — po równej liczbie dodatkowych rzutów wynik rozstrzyga.
    if (inSuddenDeath()) {
        const i32 sdTaken = state_.homeTaken - r.kicksPerSide;
        const i32 sdTakenAway = state_.awayTaken - r.kicksPerSide;
        if (sdTaken == sdTakenAway && state_.homeScore != state_.awayScore) {
            state_.status = ShootoutStatus::Finished;
            state_.winner = state_.homeScore > state_.awayScore ? Side::Home : Side::Away;
        } else if (sdTaken == sdTakenAway && state_.suddenDeathRounds > r.maxSuddenDeathRounds) {
            state_.decidedByTieBreaker = true;
            state_.status = ShootoutStatus::Finished;
            state_.winner = std::nullopt;
        } else if (sdTaken == sdTakenAway) {
            state_.suddenDeathRounds++;
        }
        return;
    }
    // 3. Rozstrzygnięcie matematyczne w trakcie pierwszej piątki.
    const std::optional<Side> early = earlyWinner(state_.homeScore, state_.awayScore,
                                                 state_.homeTaken, state_.awayTaken, r);
    if (early.has_value()) {
        state_.status = ShootoutStatus::Finished;
        state_.winner = early;
        state_.decidedEarly = true;
    }
}

bool Shootout::recordKick(const KickRecord& kick) {
    if (isFinished()) return false;
    state_.kicks.push_back(kick);
    if (kick.side == Side::Home) {
        if (kick.scored()) state_.homeScore++;
        state_.homeTaken++;
    } else {
        if (kick.scored()) state_.awayScore++;
        state_.awayTaken++;
    }
    evaluateCompletion();
    return true;
}

void Shootout::resolveTieBreaker(Side fallback) {
    state_.decidedByTieBreaker = true;
    state_.status = ShootoutStatus::Finished;
    state_.winner = state_.homeScore > state_.awayScore ? Side::Home
                  : state_.awayScore > state_.homeScore ? Side::Away
                  : std::optional<Side>{fallback};
}

namespace {

/// Modeluje prawdopodobieństwo trafienia jednego karnego na podstawie różnicy siły.
f64 kickSuccessChance(f64 attackerStrength, f64 defenderStrength, f64 pressure) {
    const f64 diff = (attackerStrength - defenderStrength) / 100.0;
    f64 p = 0.74 + 0.20 * diff;
    p -= 0.10 * clamp(pressure, 0.0, 1.0) * (1.0 - clamp(attackerStrength / 100.0, 0.0, 1.0) * 0.5);
    return clamp(p, 0.40, 0.94);
}

}  // namespace

ShootoutScore simulateShootout(f64 homeStrength, f64 awayStrength, Random& rng) {
    ShootoutScore s;
    i32 home = 0, away = 0, homeTaken = 0, awayTaken = 0;
    bool sd = false;
    i32 sdRounds = 0;
    const i32 baseKicks = 5;
    for (i32 kick = 0; kick < 200; ++kick) {
        const bool homeAttacks = (kick % 2) == 0;
        const f64 pressure = clamp(0.20 + 0.30 * std::abs(home - away) / 5.0, 0.0, 1.0);
        if (homeAttacks) {
            const f64 p = kickSuccessChance(homeStrength, awayStrength, pressure);
            if (rng.chance(p)) home++;
            homeTaken++;
        } else {
            const f64 p = kickSuccessChance(awayStrength, homeStrength, pressure);
            if (rng.chance(p)) away++;
            awayTaken++;
        }
        if (!sd) {
            const i32 remainingHome = baseKicks - homeTaken;
            const i32 remainingAway = baseKicks - awayTaken;
            if (homeTaken >= baseKicks && awayTaken >= baseKicks) {
                if (home != away) break;
                sd = true;
                s.rounds = baseKicks;
            } else if (home > away + remainingAway || away > home + remainingHome) {
                s.rounds = std::max(homeTaken, awayTaken);
                break;
            }
        } else {
            sdRounds++;
            s.rounds = baseKicks + sdRounds;
            // Nagła śmierć — po jednej parze sprawdzamy wynik.
            if (homeTaken == awayTaken && home != away) break;
        }
        if (kick > 100) break;  // bezpiecznik
    }
    s.home = home;
    s.away = away;
    return s;
}

LeagueMatchScore simulateLeagueMatch(f64 homeStrength, f64 awayStrength, Random& rng) {
    LeagueMatchScore m;
    const ShootoutScore s = simulateShootout(homeStrength, awayStrength, rng);
    m.homeGoals = s.home;
    m.awayGoals = s.away;
    m.shootout = s;
    return m;
}

}  // namespace bkh