// Implementacja katalogu miejscowości (PlaceCatalog) i pomocników językowych.
// Zob. core/include/bkh/place.h dla kontraktu i docs/FDROID.md dla źródła danych.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "bkh/place.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <unordered_map>

namespace bkh {

namespace {

/// 16 województw w kolejności kanonicznej (alfabetycznie po polskiej nazwie).
constexpr const char* kVoivodeships[] = {
    "dolnośląskie",
    "kujawsko-pomorskie",
    "lubelskie",
    "lubuskie",
    "łódzkie",
    "małopolskie",
    "mazowieckie",
    "opolskie",
    "podkarpackie",
    "podlaskie",
    "pomorskie",
    "śląskie",
    "świętokrzyskie",
    "warmińsko-mazurskie",
    "wielkopolskie",
    "zachodniopomorskie",
};

constexpr const char* kVoivodeshipsEn[] = {
    "Lower Silesia",
    "Kuyavia-Pomerania",
    "Lublin",
    "Lubusz",
    "Łódź",
    "Lesser Poland",
    "Masovia",
    "Opole",
    "Subcarpathia",
    "Podlasie",
    "Pomerania",
    "Silesia",
    "Holy Cross",
    "Warmia-Masuria",
    "Greater Poland",
    "West Pomerania",
};

constexpr f64 kEarthRadiusKm = 6371.0088;

/// Normalizacja pojedynczego znaku UTF-8 (polskie litery).
/// Zwraca liczbę bajtów wejściowych (-1, jeśli znak poza obsługą).
int foldUtf8(const char* s, char out[3]) {
    const auto u0 = static_cast<u8>(s[0]);
    if (u0 < 0x80) {
        // ASCII.
        if (u0 >= 'A' && u0 <= 'Z') out[0] = static_cast<char>(u0 - 'A' + 'a');
        else out[0] = static_cast<char>(u0);
        out[1] = '\0';
        return 1;
    }
    // Sekwencje 2-bajtowe (Latin Extended-A/B): U+0100..U+024F.
    if ((u0 & 0xE0) == 0xC0 && (static_cast<u8>(s[1]) & 0xC0) == 0x80) {
        const u32 cp = (static_cast<u32>(u0 & 0x1F) << 6) | (static_cast<u8>(s[1]) & 0x3F);
        switch (cp) {
            case 0x0105: out[0] = 'a'; out[1] = '\0'; return 2;  // ą → a
            case 0x0107: out[0] = 'c'; out[1] = '\0'; return 2;  // ć → c
            case 0x0119: out[0] = 'e'; out[1] = '\0'; return 2;  // ę → e
            case 0x0142: out[0] = 'l'; out[1] = '\0'; return 2;  // ł → l
            case 0x0144: out[0] = 'n'; out[1] = '\0'; return 2;  // ń → n
            case 0x00F3: out[0] = 'o'; out[1] = '\0'; return 2;  // ó → o
            case 0x015B: out[0] = 's'; out[1] = '\0'; return 2;  // ś → s
            case 0x017A: out[0] = 'z'; out[1] = '\0'; return 2;  // ź → z
            case 0x017C: out[0] = 'z'; out[1] = '\0'; return 2;  // ż → z
            case 0x0118: out[0] = 'e'; out[1] = '\0'; return 2;  // Ę → e
            case 0x0104: out[0] = 'a'; out[1] = '\0'; return 2;  // Ą → a
            case 0x0106: out[0] = 'c'; out[1] = '\0'; return 2;  // Ć → c
            case 0x0141: out[0] = 'l'; out[1] = '\0'; return 2;  // Ł → l
            case 0x0143: out[0] = 'n'; out[1] = '\0'; return 2;  // Ń → n
            case 0x00D3: out[0] = 'o'; out[1] = '\0'; return 2;  // Ó → o
            case 0x015A: out[0] = 's'; out[1] = '\0'; return 2;  // Ś → s
            case 0x0179: out[0] = 'z'; out[1] = '\0'; return 2;  // Ź → z
            case 0x017B: out[0] = 'z'; out[1] = '\0'; return 2;  // Ż → z
            default: out[0] = '?'; out[1] = '\0'; return 2;
        }
    }
    // Pomijamy wszystko inne (znaki spoza zakresu).
    if ((u0 & 0xF0) == 0xE0) return 3;  // 3-bajtowy UTF-8
    if ((u0 & 0xF8) == 0xF0) return 4;  // 4-bajtowy UTF-8
    return 1;
}

}  // namespace

const char* placeTypeName(PlaceType type) {
    switch (type) {
        case PlaceType::City: return "city";
        case PlaceType::Town: return "town";
        case PlaceType::Village: return "village";
    }
    return "unknown";
}

std::optional<PlaceType> placeTypeFromString(std::string_view text) {
    if (text == "city") return PlaceType::City;
    if (text == "town") return PlaceType::Town;
    if (text == "village") return PlaceType::Village;
    return std::nullopt;
}

const std::array<std::string_view, 16>& voivodeshipNames() {
    static const std::array<std::string_view, 16> names = [] {
        std::array<std::string_view, 16> a{};
        for (std::size_t i = 0; i < 16; ++i) a[i] = kVoivodeships[i];
        return a;
    }();
    return names;
}

std::optional<u8> voivodeshipId(std::string_view name) {
    for (std::size_t i = 0; i < 16; ++i) {
        if (name == kVoivodeships[i]) return static_cast<u8>(i);
        if (normalizePl(name) == normalizePl(kVoivodeships[i])) return static_cast<u8>(i);
    }
    return std::nullopt;
}

std::string normalizePl(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size();) {
        char buf[3] = {};
        const int n = foldUtf8(text.data() + i, buf);
        if (n <= 0) break;
        out.append(buf);
        i += static_cast<std::size_t>(n);
    }
    return out;
}

const std::array<std::string_view, 16>& voivodeshipNamesEn() {
    static const std::array<std::string_view, 16> names = [] {
        std::array<std::string_view, 16> a{};
        for (std::size_t i = 0; i < 16; ++i) a[i] = kVoivodeshipsEn[i];
        return a;
    }();
    return names;
}

namespace {

/// Minimalny tokenizer CSV: pola rozdzielone przecinkami, cudzysłów `"`,
/// podwójny cudzysłów `""` jako escape. Akceptuje LF i CR.
std::vector<std::string_view> tokenizeRow(std::string_view row) {
    std::vector<std::string_view> fields;
    std::size_t i = 0;
    while (i < row.size()) {
        if (row[i] == '"') {
            // Cytowane pole.
            ++i;
            std::size_t start = i;
            std::string buf;
            while (i < row.size()) {
                if (row[i] == '"' && i + 1 < row.size() && row[i + 1] == '"') {
                    buf.append(row.substr(start, i - start));
                    buf.append("\"");
                    i += 2;
                    start = i;
                } else if (row[i] == '"') {
                    buf.append(row.substr(start, i - start));
                    i++;
                    break;
                } else {
                    ++i;
                }
            }
            fields.push_back(row.substr(0, 0));  // zastąpimy poniżej — trudne ze string_view
            (void)start;
            // Konwersja na string_view wymaga bufora stałego; bezpieczniej: zbuduj string.
            static thread_local std::vector<std::string> bufs;
            bufs.push_back(std::move(buf));
            fields.back() = bufs.back();
        } else {
            std::size_t start = i;
            while (i < row.size() && row[i] != ',') ++i;
            std::string_view f = row.substr(start, i - start);
            // Trim końców.
            while (!f.empty() && (f.back() == '\r' || f.back() == '\n' || f.back() == ' ')) f.remove_suffix(1);
            while (!f.empty() && f.front() == ' ') f.remove_prefix(1);
            fields.push_back(f);
        }
        if (i < row.size() && row[i] == ',') {
            ++i;
        }
    }
    return fields;
}

bool tryParseI64(std::string_view s, i64& out) {
    if (s.empty()) return false;
    std::size_t i = 0;
    bool negative = false;
    if (s[0] == '-') { negative = true; i = 1; }
    if (i == s.size()) return false;
    i64 v = 0;
    for (; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
        v = v * 10 + (s[i] - '0');
        if (v > 100000000000LL) return false;
    }
    out = negative ? -v : v;
    return true;
}

bool tryParseDouble(std::string_view s, f64& out) {
    if (s.empty()) return false;
    // std::from_chars dla double jest w libc++ NDK nadal usunięte (dostępne tylko dla
    // typów całkowitych), a strtod zależałby od locale procesu. Parsujemy ręcznie —
    // kropka dziesiętna, deterministycznie i niezależnie od locale (jak tryParseI64).
    std::size_t i = 0;
    const bool negative = s[0] == '-';
    if (s[0] == '-' || s[0] == '+') {
        ++i;
        if (i == s.size()) return false;
    }

    f64 value = 0.0;
    bool anyDigit = false;
    for (; i < s.size() && s[i] >= '0' && s[i] <= '9'; ++i) {
        value = value * 10.0 + static_cast<f64>(s[i] - '0');
        anyDigit = true;
    }
    if (i < s.size() && s[i] == '.') {
        ++i;
        f64 scale = 0.1;
        for (; i < s.size() && s[i] >= '0' && s[i] <= '9'; ++i) {
            value += static_cast<f64>(s[i] - '0') * scale;
            scale *= 0.1;
            anyDigit = true;
        }
    }
    if (!anyDigit) return false;

    // Wykładnik dziesiętny (np. 1.5e-3) — opcjonalny.
    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
        ++i;
        bool expNegative = false;
        if (i < s.size() && (s[i] == '-' || s[i] == '+')) {
            expNegative = s[i] == '-';
            ++i;
        }
        if (i == s.size()) return false;
        int exponent = 0;
        for (; i < s.size() && s[i] >= '0' && s[i] <= '9'; ++i) {
            exponent = exponent * 10 + (s[i] - '0');
            if (exponent > 308) return false;  // poza zakresem double
        }
        for (int k = 0; k < exponent; ++k) {
            value = expNegative ? value / 10.0 : value * 10.0;
        }
    }

    if (i != s.size()) return false;
    out = negative ? -value : value;
    return true;
}

bool tryParseI32(std::string_view s, i32& out) {
    i64 v;
    if (!tryParseI64(s, v) || v > 2147483647LL || v < -2147483648LL) return false;
    out = static_cast<i32>(v);
    return true;
}

}  // namespace

PlaceCatalog PlaceCatalog::parseCsv(std::string_view csv, PlaceParseReport& report) {
    PlaceCatalog catalog;
    std::unordered_map<i64, std::size_t> idSeen;
    idSeen.reserve(8192);

    std::size_t pos = 0;
    bool firstLine = true;
    bool hasHeader = true;
    while (pos < csv.size()) {
        std::size_t eol = csv.find('\n', pos);
        if (eol == std::string_view::npos) eol = csv.size();
        std::string_view line = csv.substr(pos, eol - pos);
        pos = eol + 1;

        // Pomijamy puste linie i komentarze (linie zaczynające się od `#`).
        bool onlyWs = true;
        for (char c : line) {
            if (c != ' ' && c != '\t' && c != '\r') { onlyWs = false; break; }
        }
        if (onlyWs) continue;
        if (!line.empty() && line.front() == '#') continue;

        const auto fields = tokenizeRow(line);
        if (fields.empty()) continue;
        ++report.rowsSeen;

        if (firstLine) {
            // Wykrycie nagłówka: jeśli pierwsza kolumna to "osm_id" — nagłówek.
            if (fields[0] == "osm_id") {
                hasHeader = true;
                firstLine = false;
                continue;
            }
            hasHeader = false;
            firstLine = false;
        }

        if (fields.size() < 7) {
            ++report.rowsRejected;
            if (report.warnings.size() < 20) {
                report.warnings.push_back("za mało kolumn");
            }
            continue;
        }

        Place p{};
        if (!tryParseI64(fields[0], p.osmId) || p.osmId <= 0) {
            ++report.rowsRejected;
            if (report.warnings.size() < 20) report.warnings.push_back("osm_id nieprawidłowy");
            continue;
        }
        if (idSeen.contains(p.osmId)) {
            ++report.duplicateIds;
            continue;
        }
        p.name = std::string(fields[1]);
        if (p.name.empty()) {
            ++report.rowsRejected;
            if (report.warnings.size() < 20) report.warnings.push_back("pusta nazwa");
            continue;
        }
        const auto type = placeTypeFromString(fields[2]);
        if (!type.has_value()) {
            ++report.rowsRejected;
            if (report.warnings.size() < 20) report.warnings.push_back("nieznany place");
            continue;
        }
        p.type = type.value();
        if (!tryParseI32(fields[3], p.population) || p.population < 0) {
            if (p.type == PlaceType::Village) {
                ++report.rowsRejected;
                if (report.warnings.size() < 20) report.warnings.push_back("wieś bez populacji");
                continue;
            }
            p.population = 0;
        }
        if (!tryParseDouble(fields[4], p.lat) || !tryParseDouble(fields[5], p.lon) ||
            p.lat < 48.9 || p.lat > 55.0 || p.lon < 14.0 || p.lon > 24.5) {
            ++report.rowsRejected;
            if (report.warnings.size() < 20) report.warnings.push_back("współrzędne poza PL");
            continue;
        }
        const auto v = voivodeshipId(fields[6]);
        if (!v.has_value()) {
            ++report.rowsRejected;
            if (report.warnings.size() < 20) report.warnings.push_back("nieznane województwo");
            continue;
        }
        p.voivodeship = v.value();

        idSeen[p.osmId] = catalog.places_.size();
        catalog.places_.push_back(std::move(p));
        ++report.rowsAccepted;
    }
    (void)hasHeader;
    catalog.rebuildIndex();
    return catalog;
}

void PlaceCatalog::add(Place place) {
    places_.push_back(std::move(place));
}

void PlaceCatalog::rebuildIndex() {
    idIndex_.clear();
    nameIndex_.clear();
    idIndex_.reserve(places_.size());
    nameIndex_.reserve(places_.size());
    for (std::size_t i = 0; i < places_.size(); ++i) {
        idIndex_.push_back({places_[i].osmId, i});
        const u64 h = fnv1a(normalizePl(places_[i].name));
        nameIndex_.push_back({h, i});
    }
    std::sort(idIndex_.begin(), idIndex_.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    std::sort(nameIndex_.begin(), nameIndex_.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
}

const Place* PlaceCatalog::findByOsmId(i64 osmId) const {
    auto it = std::lower_bound(idIndex_.begin(), idIndex_.end(), osmId,
                               [](const auto& a, i64 v) { return a.first < v; });
    if (it != idIndex_.end() && it->first == osmId) return &places_[it->second];
    return nullptr;
}

const Place* PlaceCatalog::findByName(std::string_view name) const {
    const std::string target = normalizePl(name);
    const u64 h = fnv1a(target);
    for (const auto& [hh, idx] : nameIndex_) {
        if (hh > h) break;
        if (hh == h && normalizePl(places_[idx].name) == target) return &places_[idx];
    }
    return nullptr;
}

const Place* PlaceCatalog::bestMatch(std::string_view query) const {
    if (query.empty()) return nullptr;
    const std::string norm = normalizePl(query);
    if (norm.empty()) return nullptr;
    const Place* exact = findByName(norm);
    if (exact) return exact;
    // Prefiks, a potem podłańcuch — wybieramy największe miejscowości.
    const Place* best = nullptr;
    i32 bestPop = -1;
    i64 bestId = 0;
    for (const Place& p : places_) {
        const std::string np = normalizePl(p.name);
        if (np.empty()) continue;
        if (np.compare(0, norm.size(), norm) == 0 || np.find(norm) != std::string::npos) {
            if (p.population > bestPop || (p.population == bestPop && p.osmId > bestId)) {
                best = &p;
                bestPop = p.population;
                bestId = p.osmId;
            }
        }
    }
    return best;
}

std::vector<const Place*> PlaceCatalog::search(std::string_view query, std::size_t limit) const {
    std::vector<const Place*> results;
    if (query.empty() || limit == 0) return results;
    const std::string norm = normalizePl(query);
    if (norm.empty()) return results;
    // Kandydaci: prefiks → podłańcuch, wynik sortowany po populacji malejąco, potem nazwa.
    std::vector<const Place*> pref, sub;
    for (const Place& p : places_) {
        const std::string np = normalizePl(p.name);
        if (np.empty()) continue;
        if (np.compare(0, norm.size(), norm) == 0) pref.push_back(&p);
        else if (np.find(norm) != std::string::npos) sub.push_back(&p);
    }
    auto sortByPopName = [](const Place* a, const Place* b) {
        if (a->population != b->population) return a->population > b->population;
        return a->name < b->name;
    };
    std::sort(pref.begin(), pref.end(), sortByPopName);
    std::sort(sub.begin(), sub.end(), sortByPopName);
    results.reserve(std::min(limit, pref.size() + sub.size()));
    for (const Place* p : pref) {
        if (results.size() >= limit) break;
        results.push_back(p);
    }
    for (const Place* p : sub) {
        if (results.size() >= limit) break;
        results.push_back(p);
    }
    return results;
}

f64 PlaceCatalog::distanceKm(f64 lat1, f64 lon1, f64 lat2, f64 lon2) {
    const f64 phi1 = lat1 * kPi / 180.0;
    const f64 phi2 = lat2 * kPi / 180.0;
    const f64 dphi = (lat2 - lat1) * kPi / 180.0;
    const f64 dlam = (lon2 - lon1) * kPi / 180.0;
    const f64 a = std::sin(dphi / 2.0) * std::sin(dphi / 2.0) +
                  std::cos(phi1) * std::cos(phi2) * std::sin(dlam / 2.0) * std::sin(dlam / 2.0);
    const f64 c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return kEarthRadiusKm * c;
}

std::vector<const Place*> PlaceCatalog::nearest(f64 lat, f64 lon, f64 radiusKm,
                                                std::size_t limit, i32 minPopulation,
                                                bool includeVillages) const {
    std::vector<std::pair<f64, const Place*>> pairs;
    pairs.reserve(places_.size() / 4);
    for (const Place& p : places_) {
        if (!includeVillages && p.type == PlaceType::Village) continue;
        if (p.population < minPopulation) continue;
        const f64 d = distanceKm(lat, lon, p.lat, p.lon);
        if (d > radiusKm) continue;
        pairs.push_back({d, &p});
    }
    std::sort(pairs.begin(), pairs.end(),
              [](const auto& a, const auto& b) {
                  if (a.first != b.first) return a.first < b.first;
                  return a.second->osmId < b.second->osmId;
              });
    std::vector<const Place*> results;
    results.reserve(std::min(limit, pairs.size()));
    for (std::size_t i = 0; i < pairs.size() && results.size() < limit; ++i) {
        results.push_back(pairs[i].second);
    }
    return results;
}

std::vector<const Place*> PlaceCatalog::largest(std::size_t limit, i32 minPopulation) const {
    std::vector<const Place*> v;
    v.reserve(places_.size());
    for (const Place& p : places_) {
        if (p.population < minPopulation) continue;
        v.push_back(&p);
    }
    std::sort(v.begin(), v.end(),
              [](const Place* a, const Place* b) {
                  if (a->population != b->population) return a->population > b->population;
                  return a->osmId < b->osmId;
              });
    if (v.size() > limit) v.resize(limit);
    return v;
}

std::vector<const Place*> PlaceCatalog::inVoivodeship(u8 voivodeship, std::size_t limit) const {
    std::vector<const Place*> v;
    for (const Place& p : places_) {
        if (p.voivodeship == voivodeship) v.push_back(&p);
    }
    std::sort(v.begin(), v.end(),
              [](const Place* a, const Place* b) {
                  if (a->population != b->population) return a->population > b->population;
                  return a->osmId < b->osmId;
              });
    if (v.size() > limit) v.resize(limit);
    return v;
}

std::string PlaceCatalog::summary() const {
    std::ostringstream os;
    os << places_.size() << " miejscowości, 16 województw";
    if (!places_.empty()) {
        const Place* biggest = &places_[0];
        for (const Place& p : places_) {
            if (p.population > biggest->population) biggest = &p;
        }
        os << ", największa: " << biggest->name;
    }
    return os.str();
}

u64 PlaceCatalog::checksum() const {
    u64 h = 1469598103934665603ULL;
    for (const Place& p : places_) {
        // Stałoprzecinkowa kumulacja: unikamy bitów double (zależne od platformy).
        h = fnv1a(std::to_string(p.osmId), h);
        h = fnv1a(static_cast<std::string>(placeTypeName(p.type)), h);
        h = fnv1a(std::to_string(p.population), h);
        h = fnv1a(std::to_string(static_cast<i64>(std::round(p.lat * 1e5))), h);
        h = fnv1a(std::to_string(static_cast<i64>(std::round(p.lon * 1e5))), h);
        h = fnv1a(std::to_string(static_cast<i32>(p.voivodeship)), h);
    }
    return h;
}

}  // namespace bkh