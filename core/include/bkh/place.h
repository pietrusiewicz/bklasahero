// B-Klasa Hero — katalog miejscowości (dane OpenStreetMap, ODbL).
//
// Rdzeń NIE czyta z dysku ani z assets: warstwa Android wczytuje plik
// `assets/places_pl.csv` i przekazuje jego bajty do `PlaceCatalog::parseCsv()`.
// Dzięki temu ta sama klasa działa w testach na hoście (fixture) i na urządzeniu.
//
// Plik danych: data/places/places_pl.csv (wygenerowany przez tools/geo/fetch_places.py)
//   osm_id,name,place,population,lat,lon,voivodeship
// Licencja danych: ODbL 1.0, © OpenStreetMap contributors — zob. data/places/ATTRIBUTION.md
//
// Złożoność: katalog ~5–10 tys. rekordów trzymamy w wektorze; zapytania
// „najbliższe" i „szukaj po nazwie" to przegląd liniowy (kilkadziesiąt
// mikrosekund), więc indeks przestrzenny jest niepotrzebny. Jeśli katalog urośnie
// >50 tys. rekordów, dodaj siatkę (grid index) — patrz komentarz w nearest().
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "bkh/types.h"

namespace bkh {

enum class PlaceType : u8 { City = 0, Town = 1, Village = 2 };

[[nodiscard]] const char* placeTypeName(PlaceType type);
[[nodiscard]] std::optional<PlaceType> placeTypeFromString(std::string_view text);

struct Place {
    i64 osmId = 0;
    std::string name;
    PlaceType type = PlaceType::Village;
    i32 population = 0;
    f64 lat = 0.0;
    f64 lon = 0.0;
    u8 voivodeship = kUnknownVoivodeship;

    static constexpr u8 kUnknownVoivodeship = 255;

    /// Klucz deterministyczny (niezależny od kolejności w pliku).
    [[nodiscard]] u64 stableKey() const { return static_cast<u64>(osmId); }
    /// Miasto z prawami miejskimi (city/town) — używane przy doborze rywali
    /// w wyższych ligach.
    [[nodiscard]] bool isUrban() const { return type != PlaceType::Village; }
};

/// Statystyki parsowania — przydają się w logach i w testach integralności danych.
struct PlaceParseReport {
    std::size_t rowsSeen = 0;
    std::size_t rowsAccepted = 0;
    std::size_t rowsRejected = 0;
    std::size_t duplicateIds = 0;
    std::vector<std::string> warnings;  // pierwsze N problemów (nie blokujące)

    [[nodiscard]] bool ok() const { return rowsAccepted > 0; }
};

/// Nazwy 16 województw w mianowniku (formy przymiotnikowe, jak w pliku danych).
[[nodiscard]] const std::array<std::string_view, 16>& voivodeshipNames();

/// Identyfikator województwa (0..15) po nazwie; `std::nullopt` jeśli nieznane.
[[nodiscard]] std::optional<u8> voivodeshipId(std::string_view name);

/// Polska normalizacja tekstu do porównań: małe litery + znaki bez ogonków/kresek
/// („Łódź" → „lodz"). Używane do wyszukiwania po tym, co wpisał gracz.
[[nodiscard]] std::string normalizePl(std::string_view text);

class PlaceCatalog {
public:
    PlaceCatalog() = default;

    /// Parsuje CSV (z nagłówkiem lub bez). Toleruje CR, cudzysłowy CSV i puste
    /// pola population. Nie rzuca wyjątków — problemy trafiają do `report`.
    static PlaceCatalog parseCsv(std::string_view csv, PlaceParseReport& report);

    [[nodiscard]] std::size_t size() const { return places_.size(); }
    [[nodiscard]] bool empty() const { return places_.empty(); }
    [[nodiscard]] const std::vector<Place>& places() const { return places_; }
    [[nodiscard]] const Place& at(std::size_t index) const { return places_.at(index); }

    /// Dokłada rekord (używane przez testy i przez generator fixture'ów).
    void add(Place place);
    /// Porządkuje indeksy wewnętrzne po ręcznym dodawaniu rekordów.
    void rebuildIndex();

    [[nodiscard]] const Place* findByOsmId(i64 osmId) const;
    /// Dokładne dopasowanie po nazwie (bez względu na wielkość liter i ogonki).
    [[nodiscard]] const Place* findByName(std::string_view name) const;
    /// Najlepsze dopasowanie nazwy wpisanej przez gracza: najpierw dokładne,
    /// potem prefiks, potem podłańcuch; preferuje większe miejscowości.
    [[nodiscard]] const Place* bestMatch(std::string_view query) const;
    /// Lista podpowiedzi do pola wyboru miasta (prefiks + podłańcuch).
    [[nodiscard]] std::vector<const Place*> search(std::string_view query, std::size_t limit = 12) const;

    /// Miejscowości w promieniu `radiusKm` od punktu, posortowane rosnąco po
    /// odległości. `minPopulation` filtruje maleństwa; `includeVillages=false`
    /// ogranicza wynik do miast (city/town).
    [[nodiscard]] std::vector<const Place*> nearest(f64 lat, f64 lon, f64 radiusKm,
                                                   std::size_t limit, i32 minPopulation = 0,
                                                   bool includeVillages = true) const;

    /// Największe miejscowości w kraju (do obsadzania wyższych lig).
    [[nodiscard]] std::vector<const Place*> largest(std::size_t limit, i32 minPopulation = 0) const;

    /// Miejscowości w zadanym województwie, posortowane malejąco po populacji.
    [[nodiscard]] std::vector<const Place*> inVoivodeship(u8 voivodeship, std::size_t limit) const;

    /// Odległość ortodromiczna (haversine) [km].
    [[nodiscard]] static f64 distanceKm(f64 lat1, f64 lon1, f64 lat2, f64 lon2);
    [[nodiscard]] static f64 distanceKm(const Place& a, const Place& b) {
        return distanceKm(a.lat, a.lon, b.lat, b.lon);
    }

    /// Zwięzły opis do logów („12345 miejscowości, 16 województw, największa: Warszawa").
    [[nodiscard]] std::string summary() const;

    /// Suma kontrolna zawartości — pozwala wykryć podmianę pliku danych
    /// i jest zapisywana w save'ie (spójność kariery z danymi).
    [[nodiscard]] u64 checksum() const;

private:
    std::vector<Place> places_;
    std::vector<std::pair<i64, std::size_t>> idIndex_;      // posortowane po osm_id
    std::vector<std::pair<u64, std::size_t>> nameIndex_;    // hash(normalizePl(name)) → index
};

}  // namespace bkh
