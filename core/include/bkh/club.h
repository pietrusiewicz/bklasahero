// B-Klasa Hero — kluby: model danych i deterministyczny generator nazw.
//
// Nazwy klubów są WYMYŚLONE, ale zgodne z lokalizacją i z polską konwencją
// nazewniczą niższych lig: przedrostek organizacyjny (LKS/GKS/KS/MKS/LZS…) +
// patron (Orzeł, Pogoń, Znicz, Bór…) + nazwa miejscowości, albo forma
// przymiotnikowa od miejscowości („Bartodziejanka"). Wyższe ligi dostają
// „poważniejsze" konstrukcje i większe ośrodki.
//
// Generator jest deterministyczny względem ziarna i identyfikatora miejscowości,
// więc ta sama miejscowość w tym samym ziarnie daje zawsze ten sam klub —
// to warunek odtwarzalności save'a i testowalności na hoście.
//
// Generator unika nazw klubów realnie istniejących (lista wykluczeń w
// club.cpp::isLikelyRealClubName) — to świadoma decyzja prawna, zob.
// docs/ADR/0006-fictional-clubs.md.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "bkh/place.h"
#include "bkh/random.h"
#include "bkh/types.h"

namespace bkh {

/// Kolory klubu w formacie ARGB (0xAARRGGBB) — używane przez renderer Compose.
struct ClubColors {
    u32 primary = 0xFF2E7D32;
    u32 secondary = 0xFFFFFFFF;
    u32 accent = 0xFF1B5E20;

    [[nodiscard]] bool operator==(const ClubColors&) const = default;
};

struct Club {
    i32 id = 0;              // unikalny w ramach save'a
    std::string name;        // pełna nazwa: „LKS Orzeł Bartodzieje"
    std::string shortName;   // do tabeli i komentarzy: „Orzeł"
    std::string town;        // miejscowość klubu
    u8 voivodeship = Place::kUnknownVoivodeship;
    f64 lat = 0.0;
    f64 lon = 0.0;
    i32 population = 0;
    i64 placeOsmId = 0;
    ClubColors colors{};
    f64 strength = 50.0;     // siła gry [0..100] — wpływa na profile strzelca/bramkarza
    i32 foundedYear = 1948;  // smaczek fabularny, deterministyczny
    bool isPlayer = false;

    [[nodiscard]] bool operator==(const Club&) const = default;
};

/// Rodzaj członu nazwy — używany przez generator i przez testy.
enum class NamePartKind : u8 { Prefix = 0, Patron = 1, Adjectival = 2 };

struct NameProposal {
    std::string fullName;    // „LKS Orzeł Bartodzieje"
    std::string shortName;   // „Orzeł"
    std::string prefix;      // „LKS" (może być puste)
    std::string patron;      // „Orzeł" (może być puste, jeśli forma przymiotnikowa)
    bool adjectival = false; // true = nazwa typu „Bartodziejanka"
};

class ClubGenerator {
public:
    /// `catalog` musi przeżyć generator (trzymamy wskaźnik, nie kopię).
    explicit ClubGenerator(const PlaceCatalog& catalog) : catalog_(&catalog) {}

    /// Tworzy klub dla miejscowości na danym poziomie ligowym (0..7).
    [[nodiscard]] Club makeClub(const Place& place, i32 tierIndex, Random& rng) const;

    /// Obsadza ligę: wybiera miejscowości odpowiednie dla poziomu i generuje kluby.
    /// `homePlace` to miasto gracza — w ligach regionalnych (niższe poziomy)
    /// dobieramy miejscowości z okolicy, w ligach ogólnokrajowych z całej Polski.
    /// Klub gracza jest tworzony z `homePlace` i umieszczany pod indeksem
    /// zwróconym w `playerClubIndex`.
    [[nodiscard]] std::vector<Club> makeLeague(const Place& homePlace, i32 tierIndex,
                                              std::size_t clubCount, Random& rng,
                                              std::size_t& playerClubIndex) const;

    /// Wybór miejscowości pod ligę regionalną: najbliższe + wymuszona różnorodność
    /// (nie więcej niż jeden klub z tej samej miejscowości, preferowany mix
    /// miasteczek i wsi dla klimatu B klasy).
    ///
    /// Promień zaczyna się od `TierInfo::radiusKm` i rośnie, gdy w okolicy jest
    /// mało miejscowości — ale nie bardziej niż 2.5× (liga ma zostać lokalna).
    /// Gdy i to nie wystarczy, dopełniamy najbliższymi miejscowościami z TEGO
    /// SAMEGO województwa; dopiero ostateczność sięga po największe miasta
    /// w kraju (bardzo mały katalog, np. w testach).
    [[nodiscard]] std::vector<const Place*> selectRegionalPlaces(const Place& homePlace,
                                                                i32 tierIndex,
                                                                std::size_t count,
                                                                Random& rng) const;

    /// Wybór miejscowości pod ligę ogólnokrajową (największe ośrodki + regiony).
    [[nodiscard]] std::vector<const Place*> selectNationalPlaces(i32 tierIndex, std::size_t count,
                                                                Random& rng) const;

    // --- Nazewnictwo (czyste funkcje — łatwe do testowania) -------------------

    /// Proponuje nazwę klubu dla miejscowości i poziomu ligi.
    /// `attempt` różnicuje losowanie, żeby można było ponowić przy kolizji.
    [[nodiscard]] static NameProposal proposeName(const Place& place, i32 tierIndex, u64 seed,
                                                 u32 attempt = 0);

    /// Tworzy formę przymiotnikową od nazwy miejscowości („Bartodzieje" →
    /// „Bartodziejanka", „Żywiec" → „Żywczanka", „Sochaczew" → „Sochaczewianka").
    /// Zwraca pusty string, gdy nie da się sensownie utworzyć formy.
    [[nodiscard]] static std::string adjectivalForm(std::string_view placeName);

    /// Czy nazwa wygląda na realnie istniejący klub (lista wykluczeń + wzorce).
    [[nodiscard]] static bool isLikelyRealClubName(std::string_view fullName);

    /// Deterministyczne kolory klubu z ziarna i poziomu ligi.
    [[nodiscard]] static ClubColors makeColors(u64 seed, i32 tierIndex);

    /// Deterministyczny rok założenia (1908–1999, starsze w wyższych ligach).
    [[nodiscard]] static i32 makeFoundedYear(u64 seed, i32 tierIndex);

    /// Siła klubu [0..100] dla poziomu ligi z rozrzutem deterministycznym z ziarna.
    [[nodiscard]] static f64 makeStrength(i32 tierIndex, u64 seed, bool isPlayerClub);

    [[nodiscard]] const PlaceCatalog& catalog() const { return *catalog_; }

private:
    const PlaceCatalog* catalog_ = nullptr;
};

/// Nazwy lig (PL/EN) i parametry poziomu — tabela prawdy dla piramidy.
struct TierInfo {
    i32 index = 0;            // 0 = najniższy (B klasa) … 7 = Ekstraklasa
    std::string_view namePl;
    std::string_view nameEn;
    std::string_view shortNamePl;
    std::string_view shortNameEn;
    i32 clubCount = 10;
    bool regional = true;     // true = kluby z okolicy miasta gracza
    f64 radiusKm = 25.0;      // promień doboru miejscowości (ligi regionalne)
    f64 strengthBase = 38.0;  // bazowa siła klubów [0..100]
    f64 strengthSpread = 8.0; // rozrzut siły między klubami
    i32 promoteCount = 2;
    i32 relegateCount = 2;
};

class Pyramid {
public:
    static constexpr i32 kTierCount = 8;
    static constexpr i32 kBottomTier = 0;
    static constexpr i32 kTopTier = kTierCount - 1;

    [[nodiscard]] static const TierInfo& tier(i32 tierIndex);
    [[nodiscard]] static i32 clampTier(i32 tierIndex);
    [[nodiscard]] static std::string_view tierName(i32 tierIndex, Lang lang);
    [[nodiscard]] static std::string_view tierShortName(i32 tierIndex, Lang lang);
    /// Nazwa ligi z nazwą województwa dla poziomów regionalnych,
    /// np. „B klasa · mazowieckie" (PL) / „B klasa · Mazovia" (EN).
    [[nodiscard]] static std::string leagueLabel(i32 tierIndex, u8 voivodeship, Lang lang);
};

/// Nazwy województw po angielsku (do etykiet lig i ekranu „O grze").
[[nodiscard]] const std::array<std::string_view, 16>& voivodeshipNamesEn();

}  // namespace bkh
