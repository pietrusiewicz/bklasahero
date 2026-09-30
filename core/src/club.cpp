// Implementacja generatora klubów i tabeli poziomów ligowych.
// Zob. core/include/bkh/club.h dla kontraktu i docs/ADR/0006-fictional-clubs.md.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "bkh/club.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace bkh {
namespace {

constexpr std::string_view kPrefixes[] = {
    "LKS", "GKS", "KS", "MKS", "LZS", "MLKS", "RKS", "OKS", "SKS", "TS", "ZKS", "AKS"
};

constexpr std::string_view kPatrons[] = {
    "Orzeł", "Sokół", "Jastrząb", "Gryf", "Bizon", "Żubr", "Dzik", "Ryś", "Wilk", "Kania",
    "Bór", "Dąb", "Brzoza", "Łan", "Wisła", "Odra", "Noteć", "San", "Nida", "Pilica",
    "Warta", "Raba", "Dunajec", "Gwda", "Obra", "Bzura", "Łyna", "Wkra", "Wieprz",
    "Kamienna", "Czarna", "Biała", "Jezioro", "Staw", "Źródło",
    "Pogoń", "Naprzód", "Start", "Znicz", "Świt", "Jutrzenka", "Gwiazda", "Iskra",
    "Płomień", "Huragan", "Burza", "Wichura", "Orkan", "Tajfun",
    "Ruch", "Unia", "Sparta", "Legion", "Victoria", "Olimpia", "Polonia",
    "Piast", "Korona", "Stal", "Górnik", "Hutnik", "Kolejarz", "Włókniarz",
    "Strzelec", "Zawisza", "Mazur", "Kaszubia", "Podlasie", "Śląsk", "Zagłębie",
    "Kujawiak", "Warmia", "Beskid", "Gorce", "Sudety"
};

constexpr u32 kColorPalette[][3] = {
    {0xFF2E7D32, 0xFFFFFFFF, 0xFF1B5E20},  // zieleń/biel
    {0xFFD32F2F, 0xFFFFFFFF, 0xFF8B0000},  // czerwień/biel
    {0xFF1976D2, 0xFFFFFFFF, 0xFF0D47A1},  // niebieski/biel
    {0xFFFFA000, 0xFF212121, 0xFFEF6C00},  // pomarańcz/czerń
    {0xFF388E3C, 0xFFFFEB3B, 0xFF1B5E20},  // zieleń/żółć
    {0xFF512DA8, 0xFFFFFFFF, 0xFF311B92},  // fiolet/biel
    {0xFF455A64, 0xFFFFC107, 0xFF263238},  // grafit/żółć
    {0xFFEC407A, 0xFFFFFFFF, 0xFFAD1457},  // róż/biel
    {0xFF00838F, 0xFFFFFFFF, 0xFF006064},  // turkus/biel
    {0xFF000000, 0xFFFFFFFF, 0xFFB71C1C},  // czerń/biel
    {0xFFE53935, 0xFF1565C0, 0xFF0D47A1},  // czerwień/niebieski
    {0xFF558B2F, 0xFFFFFFFF, 0xFF33691E},  // oliwkowy/biel
    {0xFF6D4C41, 0xFFFFEB3B, 0xFF4E342E},  // brąz/żółć
    {0xFF7B1FA2, 0xFFFFD600, 0xFF4A148C},  // purpura/żółć
    {0xFF0097A7, 0xFFE91E63, 0xFF006064},  // turkus/róż
    {0xFFFFB300, 0xFF1A237E, 0xFFE65100},  // żółć/granat
    {0xFFC62828, 0xFFE1BEE7, 0xFF8B0000},  // czerwień/fiolet
    {0xFF1565C0, 0xFFFFD54F, 0xFF0D47A1},  // niebieski/żółć
    {0xFF5D4037, 0xFFFAFAFA, 0xFF3E2723},  // brąz/krem
    {0xFF283593, 0xFFFFFFFF, 0xFF1A237E},  // indygo/biel
    {0xFFEF6C00, 0xFF212121, 0xFFE65100},  // pomarańcz/czerń
    {0xFF1B5E20, 0xFFFFCDD2, 0xFF003D00},  // ciemna zieleń/róż
    {0xFFBF360C, 0xFFFFFFFF, 0xFF870000},  // ciemna czerwień/biel
    {0xFF00695C, 0xFFFFEB3B, 0xFF004D40},  // ciemny turkus/żółć
};

/// Sufiksy dla polskich nazw miejscowości (upper-bound substrings).
constexpr std::string_view kSuffixIce[] = {"ice", "uce", "yce", "ace"};
constexpr std::string_view kSuffixIn[]  = {"lin", "min", "win", "ryn", "syn"};
constexpr std::string_view kSuffixOw[]  = {"tow", "kow", "sow", "szczow", "rzow"};
constexpr std::string_view kSuffixEw[]  = {"czew", "szew", "rew"};
constexpr std::string_view kSuffixEc[]  = {"wiec", "łec", "rzec", "ziec"};
constexpr std::string_view kSuffixA[]   = {"ba", "ga", "la", "ma", "na", "ra", "ta", "wa"};
constexpr std::string_view kSuffixO[]   = {"ko", "no", "ło", "wo", "sto"};
constexpr std::string_view kSuffixGen[] = {"gród", "bród", "staw"};

constexpr std::size_t kPrefixesCount   = sizeof(kPrefixes)  / sizeof(std::string_view);
constexpr std::size_t kPatronsCount    = sizeof(kPatrons)   / sizeof(std::string_view);
constexpr std::size_t kPaletteCount    = sizeof(kColorPalette) / sizeof(kColorPalette[0]);

/// Sprawdza, czy `name` kończy się którymś z podanych sufiksów.
bool endsWithAny(const std::string& name, const std::string_view* suffixes, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        const std::string_view s = suffixes[i];
        if (name.size() < s.size()) continue;
        if (name.compare(name.size() - s.size(), s.size(), s) == 0) return true;
    }
    return false;
}

/// Zwraca przyrostek do użycia w formie przymiotnikowej na podstawie końcówki.
std::string adjectivalSuffix(const std::string& name) {
    if (endsWithAny(name, kSuffixIce, sizeof(kSuffixIce)/sizeof(std::string_view))) return "iczanka";
    if (endsWithAny(name, kSuffixIn,  sizeof(kSuffixIn)/sizeof(std::string_view)))  return "inianka";
    if (endsWithAny(name, kSuffixOw,  sizeof(kSuffixOw)/sizeof(std::string_view)))  return "owianka";
    if (endsWithAny(name, kSuffixEw,  sizeof(kSuffixEw)/sizeof(std::string_view)))  return "ewianka";
    if (endsWithAny(name, kSuffixEc,  sizeof(kSuffixEc)/sizeof(std::string_view)))  return "czanka";
    if (endsWithAny(name, kSuffixA,   sizeof(kSuffixA)/sizeof(std::string_view)))   return "anka";
    if (endsWithAny(name, kSuffixO,   sizeof(kSuffixO)/sizeof(std::string_view)))   return "ianka";
    if (endsWithAny(name, kSuffixGen, sizeof(kSuffixGen)/sizeof(std::string_view))) return "ianie";
    // Domyślna reguła: spółgłoska → "ianka".
    return "ianka";
}

constexpr std::string_view kDeniedRealClubs[] = {
    "Legia Warszawa", "Lech Poznań", "Wisła Kraków", "Wisła Płock", "Cracovia",
    "Górnik Zabrze", "Ruch Chorzów", "Pogoń Szczecin", "Jagiellonia Białystok",
    "Śląsk Wrocław", "Raków Częstochowa", "Piast Gliwice", "Korona Kielce",
    "Widzew Łódź", "ŁKS Łódź", "Arka Gdynia", "Lechia Gdańsk",
    "Zagłębie Lubin", "Zagłębie Sosnowiec", "Stal Mielec", "Stal Rzeszów",
    "Radomiak Radom", "Motor Lublin", "Puszcza Niepołomice", "Warta Poznań",
    "Bruk-Bet Termalica", "GKS Katowice", "GKS Tychy", "Miedź Legnica",
    "Odra Opole", "Polonia Warszawa", "Polonia Bytom", "Sandecja Nowy Sącz",
    "Podbeskidzie Bielsko-Biała", "Resovia", "Chojniczanka", "Wigry Suwałki",
    "Olimpia Grudziądz", "Elana Toruń", "Zawisza Bydgoszcz", "Hutnik Kraków",
    "Wisła Puławy", "KKS Kalisz", "Kotwica Kołobrzeg", "Stal Stalowa Wola",
    "Świt Szczecin", "Unia Skierniewice", "Mazur Karczew", "Orzeł Ząbkowice",
    "Orzeł Wierzbica", "Orzeł Banie", "Iskra Kiełczów"
};

}  // namespace

std::string ClubGenerator::adjectivalForm(std::string_view placeName) {
    if (placeName.size() < 4) return std::string{};
    const std::string base(placeName);
    // Dla nazw wielowyrazowych używamy ostatniego członu.
    std::size_t lastSpace = base.find_last_of(' ');
    std::string root = (lastSpace == std::string::npos)
                            ? base
                            : base.substr(lastSpace + 1);
    if (root.size() < 4) root = base;
    // Usuwamy ostatnią literę (zamieniamy ją na nową końcówkę).
    const std::string suffix = adjectivalSuffix(root);
    if (root.size() <= 1) return std::string{};
    return root.substr(0, root.size() - 1) + suffix;
}

bool ClubGenerator::isLikelyRealClubName(std::string_view fullName) {
    // Normalizacja (małe litery, bez ogonków) do porównań.
    // Realizujemy inline, bo normalizePl żyje w place.cpp.
    std::string norm;
    norm.reserve(fullName.size());
    for (std::size_t i = 0; i < fullName.size();) {
        // Polskie znaki: proste mapowanie bajtów UTF-8 (te same co w place.cpp).
        const u8 c0 = static_cast<u8>(fullName[i]);
        if (c0 < 0x80) {
            char ch = static_cast<char>(c0);
            if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
            norm.push_back(ch);
            ++i;
            continue;
        }
        if ((c0 & 0xE0) == 0xC0 && i + 1 < fullName.size()) {
            const u32 cp = (static_cast<u32>(c0 & 0x1F) << 6) |
                           (static_cast<u8>(fullName[i + 1]) & 0x3F);
            char ch = '?';
            switch (cp) {
                case 0x0105: ch = 'a'; break;
                case 0x0107: ch = 'c'; break;
                case 0x0119: ch = 'e'; break;
                case 0x0142: ch = 'l'; break;
                case 0x0144: ch = 'n'; break;
                case 0x00F3: ch = 'o'; break;
                case 0x015B: ch = 's'; break;
                case 0x017A: case 0x017C: ch = 'z'; break;
                default: ch = '?'; break;
            }
            norm.push_back(ch);
            i += 2;
            continue;
        }
        // Pomijamy inne znaki (3/4-bajtowe).
        i += 2;
    }
    // 1. Dokładne dopasowanie.
    for (std::string_view real : kDeniedRealClubs) {
        std::string realNorm;
        realNorm.reserve(real.size());
        for (char ch : real) {
            if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
            realNorm.push_back(ch);
        }
        if (norm == realNorm) return true;
        // 2. Pomijamy przedrostki LKS/GKS/KS/MKS itd. i porównujemy resztę.
        for (std::string_view pre : kPrefixes) {
            std::string preNorm;
            preNorm.reserve(pre.size());
            for (char ch : pre) {
                if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
                preNorm.push_back(ch);
            }
            if (norm.size() > preNorm.size() + 1 && norm.compare(0, preNorm.size(), preNorm) == 0 &&
                norm[preNorm.size()] == ' ') {
                if (norm.substr(preNorm.size() + 1) == realNorm) return true;
            }
        }
    }
    return false;
}

NameProposal ClubGenerator::proposeName(const Place& place, i32 tierIndex, u64 seed, u32 attempt) {
    NameProposal proposal;
    const i32 tier = (tierIndex < 0) ? 0 : (tierIndex > 7 ? 7 : tierIndex);
    u64 s = seed ^ (static_cast<u64>(place.osmId) * 0x9E3779B97F4A7C15ULL) ^
            (static_cast<u64>(tier) * 0xBF58476D1CE4E5B9ULL) ^
            (static_cast<u64>(attempt) * 0x94D049BB133111EBULL);
    auto mix = [&]() {
        s ^= s >> 30; s *= 0xBF58476D1CE4E5B9ULL;
        s ^= s >> 27; s *= 0x94D049BB133111EBULL;
        s ^= s >> 31;
    };
    mix();
    const std::size_t prefixIdx = static_cast<std::size_t>(s % kPrefixesCount); mix();
    const std::size_t patronIdx = static_cast<std::size_t>(s % kPatronsCount);  mix();
    const f64 r = static_cast<f64>(s & 0xFFFF) / 65535.0;  // [0,1]

    const std::string prefix(kPrefixes[prefixIdx]);
    const std::string patron(kPatrons[patronIdx]);
    const std::string town = place.name;

    std::string full;
    if (tier <= 2 && r < 0.65) {
        // Niższe ligi: zwykle pełna formuła prefix + patron + miejscowość.
        full = prefix + " " + patron + " " + town;
    } else if (tier <= 4 && r < 0.55) {
        // Forma przymiotnikowa.
        const std::string adj = adjectivalForm(town);
        if (!adj.empty() && r < 0.30) {
            full = prefix + " " + adj;
        } else {
            full = prefix + " " + patron + " " + town;
        }
    } else if (r < 0.40) {
        // Sam patron + miejscowość (bez przedrostka).
        full = patron + " " + town;
    } else if (r < 0.55) {
        const std::string adj = adjectivalForm(town);
        full = adj.empty() ? (patron + " " + town) : adj;
    } else {
        // Mieszane formy.
        const std::string adj = adjectivalForm(town);
        full = adj.empty() ? (patron + " " + town)
                           : (patron + " " + adj);
    }

    // Skracanie shortName.
    std::string shortName;
    if (!prefix.empty() && (full.compare(0, prefix.size(), prefix) == 0)) {
        // Wytnijmy prefiks z pełnej nazwy — patron to "krótka" forma.
        std::size_t pos = prefix.size() + 1;
        std::size_t end = full.find(' ', pos);
        if (end != std::string::npos) shortName = full.substr(pos, end - pos);
        else shortName = full.substr(pos);
    } else {
        // Pierwsze słowo.
        std::size_t end = full.find(' ');
        shortName = (end == std::string::npos) ? full : full.substr(0, end);
    }

    proposal.fullName = std::move(full);
    proposal.shortName = shortName.empty() ? proposal.fullName : std::move(shortName);
    proposal.prefix = prefix;
    proposal.patron = patron;
    proposal.adjectival = false;  // ustawiane przez wywołującego, jeśli chce
    return proposal;
}

ClubColors ClubGenerator::makeColors(u64 seed, i32 tierIndex) {
    const i32 tier = (tierIndex < 0) ? 0 : (tierIndex > 7 ? 7 : tierIndex);
    u64 s = seed ^ (static_cast<u64>(tier) * 0xD1B54A32D192ED03ULL);
    auto mix = [&]() {
        s ^= s >> 30; s *= 0xBF58476D1CE4E5B9ULL;
        s ^= s >> 27; s *= 0x94D049BB133111EBULL;
        s ^= s >> 31;
    };
    mix();
    const std::size_t idx = static_cast<std::size_t>(s % kPaletteCount); mix();
    ClubColors c{};
    c.primary = kColorPalette[idx][0];
    c.secondary = kColorPalette[idx][1];
    c.accent = kColorPalette[idx][2];
    return c;
}

i32 ClubGenerator::makeFoundedYear(u64 seed, i32 tierIndex) {
    const i32 tier = (tierIndex < 0) ? 0 : (tierIndex > 7 ? 7 : tierIndex);
    u64 s = seed ^ 0xCBF29CE484222325ULL ^ (static_cast<u64>(tier) * 0x9E3779B97F4A7C15ULL);
    auto mix = [&]() {
        s ^= s >> 30; s *= 0xBF58476D1CE4E5B9ULL;
        s ^= s >> 27; s *= 0x94D049BB133111EBULL;
        s ^= s >> 31;
    };
    mix();
    const i32 lo = (tier >= 5) ? 1908 : 1946;
    const i32 hi = (tier >= 5) ? 1960 : 1999;
    const i32 range = hi - lo + 1;
    return lo + static_cast<i32>(s % static_cast<u64>(range));
}

f64 ClubGenerator::makeStrength(i32 tierIndex, u64 seed, bool isPlayerClub) {
    const i32 tier = (tierIndex < 0) ? 0 : (tierIndex > 7 ? 7 : tierIndex);
    const TierInfo& ti = Pyramid::tier(tier);
    u64 s = seed ^ (static_cast<u64>(tier) * 0x6C62272E07BB0142ULL);
    auto mix = [&]() {
        s ^= s >> 30; s *= 0xBF58476D1CE4E5B9ULL;
        s ^= s >> 27; s *= 0x94D049BB133111EBULL;
        s ^= s >> 31;
    };
    mix();
    // Rozrzut ±spread/2, centrowany na strengthBase.
    const f64 offset = (static_cast<f64>(s % 1000ULL) / 1000.0 - 0.5) * ti.strengthSpread;
    f64 base = ti.strengthBase + offset;
    if (isPlayerClub) base = std::max(0.0, base - 2.0);
    return clamp(base, 5.0, 98.0);
}

Club ClubGenerator::makeClub(const Place& place, i32 tierIndex, Random& rng) const {
    const u64 seed = fnv1a("club") ^ (rng.nextU64() * 0xC2B2AE3D27D4EB4FULL);
    NameProposal proposal = proposeName(place, tierIndex, seed);
    // Wymuszenie unikalności: do 6 prób, potem zostawiamy.
    for (u32 attempt = 1; attempt <= 6 && isLikelyRealClubName(proposal.fullName); ++attempt) {
        proposal = proposeName(place, tierIndex, seed, attempt);
    }
    Club c{};
    c.id = 0;  // identyfikator nadawany przez makeLeague (1 = gracz, dalej kolejne)
    c.name = std::move(proposal.fullName);
    c.shortName = std::move(proposal.shortName);
    c.town = place.name;
    c.voivodeship = place.voivodeship;
    c.lat = place.lat;
    c.lon = place.lon;
    c.population = place.population;
    c.placeOsmId = place.osmId;
    c.colors = makeColors(seed, tierIndex);
    c.foundedYear = makeFoundedYear(seed, tierIndex);
    c.strength = makeStrength(tierIndex, seed, false);
    c.isPlayer = false;
    return c;
}

namespace {
// Pomocniczy generator identyfikatorów klubów — prosty licznik w obrębie sezonu.
// Właściwa implementacja nadawania id jest w ClubGenerator::makeLeague.
}

std::vector<const Place*> ClubGenerator::selectRegionalPlaces(const Place& homePlace,
                                                             i32 tierIndex, std::size_t count,
                                                             Random& rng) const {
    const TierInfo& tinfo = Pyramid::tier(tierIndex);
    f64 radius = tinfo.radiusKm;
    std::vector<const Place*> result;
    std::set<i64> seenIds;
    seenIds.insert(homePlace.osmId);
    // Próbujemy coraz większych promieni.
    for (int pass = 0; pass < 4 && result.size() < count; ++pass) {
        auto nearby = catalog_->nearest(homePlace.lat, homePlace.lon, radius,
                                        count * 2 + 4, 0, true);
        // Filtrujemy duplikaty nazw (żeby nie było dwóch LKS Orzeł Bartodzieje).
        std::set<std::string> seenNames;
        seenNames.insert(homePlace.name);
        for (const Place* p : nearby) {
            if (seenIds.contains(p->osmId)) continue;
            if (seenNames.contains(p->name)) continue;
            result.push_back(p);
            seenIds.insert(p->osmId);
            seenNames.insert(p->name);
            if (result.size() >= count) break;
        }
        radius *= 1.8;
    }
    // Gdyby mimo wszystko za mało, dociągamy największe miejscowości w kraju.
    if (result.size() < count) {
        auto largest = catalog_->largest(64, 1000);
        for (const Place* p : largest) {
            if (result.size() >= count) break;
            if (seenIds.contains(p->osmId)) continue;
            result.push_back(p);
            seenIds.insert(p->osmId);
        }
    }
    rng.shuffle(result.begin(), result.end());
    if (result.size() > count) result.resize(count);
    return result;
}

std::vector<const Place*> ClubGenerator::selectNationalPlaces(i32 tierIndex, std::size_t count,
                                                             Random& rng) const {
    auto largest = catalog_->largest(static_cast<std::size_t>(count) * 3, 5000);
    std::vector<const Place*> result;
    std::set<u8> voivSeen;
    for (const Place* p : largest) {
        if (tierIndex >= 6) {
            // Ekstraklasa/I liga: maks. 2 kluby z tego samego województwa.
            if (voivSeen.count(p->voivodeship) >= 2) continue;
        }
        result.push_back(p);
        voivSeen.insert(p->voivodeship);
        if (result.size() >= count) break;
    }
    if (result.size() < count) {
        for (const Place* p : largest) {
            if (result.size() >= count) break;
            result.push_back(p);
        }
    }
    rng.shuffle(result.begin(), result.end());
    if (result.size() > count) result.resize(count);
    return result;
}

std::vector<Club> ClubGenerator::makeLeague(const Place& homePlace, i32 tierIndex,
                                            std::size_t clubCount, Random& rng,
                                            std::size_t& playerClubIndex) const {
    std::vector<Club> clubs;
    clubs.reserve(clubCount);
    playerClubIndex = 0;
    if (Pyramid::tier(tierIndex).regional) {
        // Zostawiamy 1 slot na klub gracza — dobieramy pozostałe N-1.
        auto places = selectRegionalPlaces(homePlace, tierIndex, clubCount - 1, rng);
        // Wstawiamy klub gracza jako pierwszy z grupy, resztę po nim.
        Club player = makeClub(homePlace, tierIndex, rng);
        player.isPlayer = true;
        player.strength = makeStrength(tierIndex,
                                       fnv1a("player") ^ rng.nextU64(), true);
        player.id = 1;
        clubs.push_back(player);
        for (std::size_t i = 0; i < places.size() && clubs.size() < clubCount; ++i) {
            Club c = makeClub(*places[i], tierIndex, rng);
            c.id = static_cast<i32>(clubs.size()) + 1;
            clubs.push_back(std::move(c));
        }
    } else {
        auto places = selectNationalPlaces(tierIndex, clubCount - 1, rng);
        Club player = makeClub(homePlace, tierIndex, rng);
        player.isPlayer = true;
        player.strength = makeStrength(tierIndex,
                                       fnv1a("player") ^ rng.nextU64(), true);
        player.id = 1;
        clubs.push_back(player);
        for (std::size_t i = 0; i < places.size() && clubs.size() < clubCount; ++i) {
            Club c = makeClub(*places[i], tierIndex, rng);
            c.id = static_cast<i32>(clubs.size()) + 1;
            clubs.push_back(std::move(c));
        }
    }
    return clubs;
}

// ---------------------------------------------------------------------------
// Piramida
// ---------------------------------------------------------------------------

namespace {
constexpr TierInfo kTiers[] = {
    {0, "B klasa",     "B Klasa",     "B kl.",  "B Cl.",   10, true,  15.0, 38.0, 8.0, 2, 2},
    {1, "A klasa",     "A Klasa",     "A kl.",  "A Cl.",   10, true,  25.0, 45.0, 8.0, 2, 2},
    {2, "Klasa okręgowa", "District League", "Okręgówka", "District", 10, true, 45.0, 52.0, 8.0, 2, 2},
    {3, "IV liga",     "Fourth League","IV liga","4th Lg", 10, true,  90.0, 58.0, 8.0, 2, 2},
    {4, "III liga",    "Third League", "III liga","3rd Lg", 10, true, 200.0, 64.0, 7.0, 2, 2},
    {5, "II liga",     "Second League","II liga","2nd Lg", 10, false,   0.0, 70.0, 6.0, 2, 2},
    {6, "I liga",      "First League", "I liga","1st Lg",  10, false,   0.0, 76.0, 6.0, 2, 2},
    {7, "Ekstraklasa", "Ekstraklasa",  "Ekl.",  "Ekl.",    10, false,   0.0, 82.0, 5.0, 0, 2}
};
}  // namespace

const TierInfo& Pyramid::tier(i32 tierIndex) {
    return kTiers[clampTier(tierIndex)];
}

i32 Pyramid::clampTier(i32 tierIndex) {
    if (tierIndex < kBottomTier) return kBottomTier;
    if (tierIndex > kTopTier) return kTopTier;
    return tierIndex;
}

std::string_view Pyramid::tierName(i32 tierIndex, Lang lang) {
    const TierInfo& t = tier(tierIndex);
    return lang == Lang::Pl ? t.namePl : t.nameEn;
}

std::string_view Pyramid::tierShortName(i32 tierIndex, Lang lang) {
    const TierInfo& t = tier(tierIndex);
    return lang == Lang::Pl ? t.shortNamePl : t.shortNameEn;
}

std::string Pyramid::leagueLabel(i32 tierIndex, u8 voivodeship, Lang lang) {
    const TierInfo& t = tier(tierIndex);
    if (!t.regional) return std::string(tierName(tierIndex, lang));
    std::string label(tierName(tierIndex, lang));
    label += " · ";
    if (lang == Lang::Pl && voivodeship < voivodeshipNames().size()) {
        label.append(voivodeshipNames()[voivodeship]);
    } else if (voivodeship < voivodeshipNamesEn().size()) {
        label.append(voivodeshipNamesEn()[voivodeship]);
    }
    return label;
}

}  // namespace bkh