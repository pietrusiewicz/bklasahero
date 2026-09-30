// Implementacja ligi: terminarz kołowy, tabela z kryteriami pomocniczymi,
// symulacja kolejki (poza meczem gracza), rozstrzygnięcie sezonu.
// Zob. core/include/bkh/league.h dla kontraktu.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "bkh/league.h"

#include <algorithm>
#include <set>

namespace bkh {

const char* League::messageKey(PlayerFate fate) {
    switch (fate) {
        case PlayerFate::Unknown:    return "fate.unknown";
        case PlayerFate::Champion:   return "fate.champion";
        case PlayerFate::Promoted:   return "fate.promoted";
        case PlayerFate::Stayed:     return "fate.stayed";
        case PlayerFate::Relegated:  return "fate.relegated";
    }
    return "fate.unknown";
}

const Club* League::playerClub() const {
    if (playerClubIndex_ < 0 || playerClubIndex_ >= static_cast<i32>(clubs_.size())) return nullptr;
    return &clubs_[playerClubIndex_];
}

std::vector<Fixture> League::buildRoundRobin(i32 clubCount) {
    std::vector<Fixture> fixtures;
    if (clubCount < 2) return fixtures;
    const i32 n = clubCount;
    // Metoda kołowa: dla parzystego N każdy klub gra z każdym dokładnie raz,
    // naprzemiennie dom/wyjazd. Kanon: indeks 0 "siedzi" i kręcimy resztę.
    std::vector<i32> order(n);
    for (i32 i = 0; i < n; ++i) order[i] = i;
    const i32 rounds = n - 1;
    const i32 matchesPerRound = n / 2;
    i32 matchIndex = 0;
    for (i32 r = 0; r < rounds; ++r) {
        // Pary (order[0], order[1]), (order[2], order[3]), ...
        for (i32 k = 0; k < matchesPerRound; ++k) {
            Fixture f{};
            f.matchIndex = matchIndex++;
            f.round = r;
            // Naprzemiennie: gdy r jest parzysty → pierwsza drużyna pary gospodarzem.
            const bool homeFirst = (r % 2) == 0;
            const i32 a = order[2 * k];
            const i32 b = order[2 * k + 1];
            f.home = homeFirst ? a : b;
            f.away = homeFirst ? b : a;
            fixtures.push_back(f);
        }
        // Obrót o 1 pozycję, trzymając order[0] w miejscu.
        if (n >= 3) {
            const i32 last = order[n - 1];
            for (i32 i = n - 1; i > 1; --i) order[i] = order[i - 1];
            order[1] = last;
        }
    }
    return fixtures;
}

League::League(std::vector<Club> clubs, i32 tierIndex, u64 seasonSeed, i32 playerClubIndex)
        : clubs_(std::move(clubs)),
          tierIndex_(tierIndex),
          playerClubIndex_(playerClubIndex),
          seasonSeed_(seasonSeed) {
    const i32 expected = Pyramid::tier(tierIndex_).clubCount;
    const i32 actual = static_cast<i32>(clubs_.size());
    (void)expected;
    (void)actual;
    fixtures_ = buildRoundRobin(static_cast<i32>(clubs_.size()));
    results_.assign(fixtures_.size(), MatchResultRecord{});
}

i32 League::roundCount() const {
    return static_cast<i32>(clubs_.size()) - 1;
}

std::vector<const Fixture*> League::roundFixtures(i32 round) const {
    std::vector<const Fixture*> r;
    for (const Fixture& f : fixtures_) {
        if (f.round == round) r.push_back(&f);
    }
    return r;
}

const Fixture* League::fixtureAt(i32 matchIndex) const {
    if (matchIndex < 0 || matchIndex >= static_cast<i32>(fixtures_.size())) return nullptr;
    return &fixtures_[matchIndex];
}

const Fixture* League::playerFixture(i32 round) const {
    if (playerClubIndex_ < 0) return nullptr;
    for (const Fixture& f : fixtures_) {
        if (f.round == round && f.involves(playerClubIndex_)) return &f;
    }
    return nullptr;
}

const Club* League::playerOpponent(i32 round) const {
    const Fixture* f = playerFixture(round);
    if (!f) return nullptr;
    const i32 opp = f->home == playerClubIndex_ ? f->away : f->home;
    return &clubs_[opp];
}

const MatchResultRecord& League::result(i32 matchIndex) const {
    static const MatchResultRecord kEmpty{};
    if (matchIndex < 0 || matchIndex >= static_cast<i32>(results_.size())) return kEmpty;
    return results_[matchIndex];
}

void League::recordResult(i32 matchIndex, const MatchResultRecord& record) {
    if (matchIndex < 0 || matchIndex >= static_cast<i32>(results_.size())) return;
    results_[matchIndex] = record;
}

bool League::isRoundComplete(i32 round) const {
    for (const Fixture& f : fixtures_) {
        if (f.round == round && !results_[f.matchIndex].played) return false;
    }
    return true;
}

i32 League::nextIncompleteRound() const {
    for (i32 r = 0; r < roundCount(); ++r) {
        if (!isRoundComplete(r)) return r;
    }
    return roundCount();
}

bool League::isSeasonComplete() const {
    return isRoundComplete(roundCount() - 1) && nextIncompleteRound() >= roundCount();
}

i32 League::playedMatches() const {
    i32 count = 0;
    for (const auto& rr : results_) if (rr.played) ++count;
    return count;
}

namespace {

/// Wynik meczu ligowego (rozstrzygnięty konkursem karnych).
struct MatchAcc {
    i32 played = 0;
    i32 wins = 0;
    i32 losses = 0;
    i32 gf = 0;
    i32 ga = 0;
    i32 pts = 0;
};

std::vector<MatchAcc> buildAcc(const std::vector<Club>& clubs, const std::vector<MatchResultRecord>& results) {
    std::vector<MatchAcc> acc(clubs.size());
    for (const auto& r : results) {
        if (!r.played) continue;
    }
    return acc;
}

}  // namespace

void League::simulateRoundExceptPlayer(i32 round, Random& rng) {
    if (round < 0 || round >= roundCount()) return;
    for (const Fixture* f : roundFixtures(round)) {
        if (!f) continue;
        if (f->involves(playerClubIndex_)) continue;
        if (results_[f->matchIndex].played) continue;
        Random subRng = rng.fork(static_cast<u64>(seasonSeed_ ^ (static_cast<u64>(round) << 32) ^ static_cast<u64>(f->matchIndex)));
        const f64 home = clubs_[f->home].strength;
        const f64 away = clubs_[f->away].strength;
        const LeagueMatchScore ms = simulateLeagueMatch(home, away, subRng);
        MatchResultRecord rec{};
        rec.played = true;
        rec.homeGoals = ms.homeGoals;
        rec.awayGoals = ms.awayGoals;
        rec.homeWins = ms.homeGoals > ms.awayGoals ? 1 : 0;
        rec.awayWins = ms.awayGoals > ms.homeGoals ? 1 : 0;
        rec.shootoutRounds = ms.shootout.rounds;
        rec.playerPlayed = false;
        rec.playerWon = false;
        recordResult(f->matchIndex, rec);
    }
}

void League::simulateWholeRound(i32 round, Random& rng) {
    for (const Fixture* f : roundFixtures(round)) {
        if (!f) continue;
        if (results_[f->matchIndex].played) continue;
        Random subRng = rng.fork(static_cast<u64>(seasonSeed_ ^ (static_cast<u64>(round) << 32) ^ static_cast<u64>(f->matchIndex)));
        const f64 home = clubs_[f->home].strength;
        const f64 away = clubs_[f->away].strength;
        const LeagueMatchScore ms = simulateLeagueMatch(home, away, subRng);
        MatchResultRecord rec{};
        rec.played = true;
        rec.homeGoals = ms.homeGoals;
        rec.awayGoals = ms.awayGoals;
        rec.homeWins = ms.homeGoals > ms.awayGoals ? 1 : 0;
        rec.awayWins = ms.awayGoals > ms.homeGoals ? 1 : 0;
        rec.shootoutRounds = ms.shootout.rounds;
        recordResult(f->matchIndex, rec);
    }
}

std::vector<LeagueRow> League::table() const {
    const i32 n = static_cast<i32>(clubs_.size());
    std::vector<LeagueRow> rows(n);
    for (i32 i = 0; i < n; ++i) {
        rows[i].clubIndex = i;
        rows[i].position = 0;
    }
    for (const auto& rec : results_) {
        if (!rec.played) continue;
        const Fixture* f = fixtureAt(static_cast<i32>(&rec - &results_[0]));
        if (!f) continue;
        const i32 h = f->home, a = f->away;
        rows[h].played++; rows[a].played++;
        rows[h].goalsFor += rec.homeGoals; rows[h].goalsAgainst += rec.awayGoals;
        rows[a].goalsFor += rec.awayGoals; rows[a].goalsAgainst += rec.homeGoals;
        if (rec.homeGoals > rec.awayGoals) {
            rows[h].wins++; rows[h].points += 3;
            rows[a].losses++;
        } else if (rec.homeGoals < rec.awayGoals) {
            rows[a].wins++; rows[a].points += 3;
            rows[h].losses++;
        } else {
            // W naszym modelu brak remisów — ale gdyby wystąpił (np. dane testowe), daj po 1 pkt.
            rows[h].points++; rows[a].points++;
            rows[h].wins++; rows[a].wins++;
        }
    }

    // Budujemy grupy remisowe (po punktach) i stosujemy kryteria head-to-head
    // w obrębie grupy. Stabilność: identyfikator klubu jako tie-break ostateczny.
    std::vector<LeagueRow> sorted = rows;
    std::sort(sorted.begin(), sorted.end(), [](const LeagueRow& a, const LeagueRow& b) {
        if (a.points != b.points) return a.points > b.points;
        const i32 gda = a.goalsFor - a.goalsAgainst;
        const i32 gdb = b.goalsFor - b.goalsAgainst;
        if (gda != gdb) return gda > gdb;
        if (a.goalsFor != b.goalsFor) return a.goalsFor > b.goalsFor;
        if (a.losses != b.losses) return a.losses < b.losses;
        return a.clubIndex < b.clubIndex;
    });

    // Dla każdej grupy zespołów o równych punktach (min. 2) uwzględniamy ich
    // bezpośrednie spotkania jako dodatkowy tie-breaker.
    std::vector<LeagueRow> out = sorted;
    std::vector<bool> visited(n, false);
    for (i32 i = 0; i < n; ) {
        i32 j = i;
        while (j < n && sorted[j].points == sorted[i].points) ++j;
        const std::size_t groupSize = static_cast<std::size_t>(j - i);
        if (groupSize <= 1) {
            out[i].position = i + 1;
            visited[i] = true;
            ++i;
            continue;
        }
        // Oblicz statystyki bezpośrednich spotkań w grupie.
        std::vector<i32> idx(groupSize);
        std::vector<i32> pts2(groupSize, 0), gdg(groupSize, 0), gf2(groupSize, 0), losses2(groupSize, 0);
        for (std::size_t k = 0; k < groupSize; ++k) idx[k] = sorted[i + static_cast<i32>(k)].clubIndex;
        for (const auto& rec : results_) {
            if (!rec.played) continue;
            const Fixture* f = fixtureAt(static_cast<i32>(&rec - &results_[0]));
            if (!f) continue;
            const auto itH = std::find(idx.begin(), idx.end(), f->home);
            const auto itA = std::find(idx.begin(), idx.end(), f->away);
            if (itH == idx.end() || itA == idx.end()) continue;
            const std::size_t kh = static_cast<std::size_t>(itH - idx.begin());
            const std::size_t ka = static_cast<std::size_t>(itA - idx.begin());
            gf2[kh] += rec.homeGoals; gf2[ka] += rec.awayGoals;
            const i32 gaH = rec.awayGoals, gaA = rec.homeGoals;
            gdg[kh] += rec.homeGoals - gaH;
            gdg[ka] += rec.awayGoals - gaA;
            if (rec.homeGoals > rec.awayGoals) {
                pts2[kh] += 3; losses2[ka]++;
            } else if (rec.homeGoals < rec.awayGoals) {
                pts2[ka] += 3; losses2[kh]++;
            } else {
                pts2[kh]++; pts2[ka]++;
            }
        }
        std::vector<i32> orderInGroup(groupSize);
        for (std::size_t k = 0; k < groupSize; ++k) orderInGroup[k] = static_cast<i32>(k);
        std::sort(orderInGroup.begin(), orderInGroup.end(), [&](i32 a2, i32 b2) {
            if (pts2[a2] != pts2[b2]) return pts2[a2] > pts2[b2];
            if (gdg[a2] != gdg[b2]) return gdg[a2] > gdg[b2];
            if (gf2[a2] != gf2[b2]) return gf2[a2] > gf2[b2];
            if (losses2[a2] != losses2[b2]) return losses2[a2] < losses2[b2];
            return idx[a2] < idx[b2];
        });
        for (std::size_t k = 0; k < groupSize; ++k) {
            const i32 posInGroup = static_cast<i32>(k);
            const LeagueRow& src = sorted[i + orderInGroup[posInGroup]];
            LeagueRow r = src;
            r.position = i + static_cast<i32>(k) + 1;
            out[i + static_cast<i32>(k)] = r;
            visited[i + static_cast<i32>(k)] = true;
        }
        i = j;
    }
    return out;
}

i32 League::positionOf(i32 clubIndex) const {
    const auto t = table();
    for (const auto& r : t) {
        if (r.clubIndex == clubIndex) return r.position;
    }
    return static_cast<i32>(clubs_.size()) + 1;
}

std::vector<i32> League::promotionPlaces() const {
    const TierInfo& t = Pyramid::tier(tierIndex_);
    if (t.promoteCount <= 0) return {};
    const auto t2 = table();
    std::vector<i32> result;
    for (i32 i = 0; i < t.promoteCount && i < static_cast<i32>(t2.size()); ++i) {
        result.push_back(t2[i].clubIndex);
    }
    return result;
}

std::vector<i32> League::relegationPlaces() const {
    const TierInfo& t = Pyramid::tier(tierIndex_);
    if (t.relegateCount <= 0) return {};
    const auto t2 = table();
    std::vector<i32> result;
    const i32 n = static_cast<i32>(t2.size());
    for (i32 i = 0; i < t.relegateCount && i < n; ++i) {
        result.push_back(t2[n - 1 - i].clubIndex);
    }
    return result;
}

League::PlayerFate League::playerFate() const {
    if (playerClubIndex_ < 0) return PlayerFate::Unknown;
    if (!isSeasonComplete()) return PlayerFate::Unknown;
    const auto t2 = table();
    const i32 pos = positionOf(playerClubIndex_);
    if (pos <= 1 && tierIndex_ == Pyramid::kTopTier) return PlayerFate::Champion;
    const auto promoted = promotionPlaces();
    if (std::find(promoted.begin(), promoted.end(), playerClubIndex_) != promoted.end()) {
        return PlayerFate::Promoted;
    }
    const auto relegated = relegationPlaces();
    if (std::find(relegated.begin(), relegated.end(), playerClubIndex_) != relegated.end()) {
        return PlayerFate::Relegated;
    }
    return PlayerFate::Stayed;
}

void League::restore(const std::vector<Fixture>& fixtures, const std::vector<MatchResultRecord>& results) {
    fixtures_ = fixtures;
    results_ = results;
    if (results_.size() != fixtures_.size()) results_.assign(fixtures_.size(), MatchResultRecord{});
}

}  // namespace bkh