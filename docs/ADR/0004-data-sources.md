# ADR-0004: Źródło danych geograficznych (OSM + ODbL)

**Status:** przyjęte
**Data:** 2026

## Decyzja

Katalog miejscowości (`data/places/places_pl.csv`) pochodzi z:

- **Geonames** — państwa, województwa (alternatywa).
- **OpenStreetMap** — granice administracyjne, kody pocztowe, populacje.

Wybraliśmy **OSM + ODbL** jako główne źródło, ponieważ:

- Ma **najdokładniejsze populacje** (aktualizowane co tydzień).
- Jest **swobodnie licencjonowany** (ODbL) — pasuje do F-Droid.
- API Overpass pozwala wyciągnąć tylko `place=city|village|town` z `name`.

## Pipeline danych

```
OSM Overpass
    │  (place=city|village|town, country=PL)
    ▼
tools/geo/fetch_places.py
    │  filtruje, normalizuje nazwy (foldUtf8), deduplikuje
    ▼
data/places/places_pl.csv (schemaVersion=1)
    │  zbundlowane w assets/ aplikacji
    ▼
bkh::PlaceCatalog::parseCsv()
    │  w pamięci, jeden obiekt
    ▼
bkh::ClubGenerator::selectRegionalPlaces()
    │  promień 15→25→45→87 km
    ▼
LeagueState.clubs
```

## Konsekwencje

- Plik CSV ma **stały schema**:
  `osm_id,name,place,population,lat,lon,voivodeship`.
- Zmiana schematu = nowa `schemaVersion` w pierwszej linii + migracja.
- Skrypt `tools/geo/fetch_places.py` jest idempotentny — można go
  uruchomić ponownie bez efektów ubocznych.
- Atrybuty: pełna lista atrybucji w
  [`data/places/ATTRIBUTION.md`](../data/places/ATTRIBUTION.md).

## Odrzucone alternatywy

- **GeoNames** — prostsze, ale gorsze pokrycie wsi (B klasa!).
- **Własne dane (curated)** — za duży nakład pracy.
- **Współrzędne z Google Maps API** — proprietary, nie spełnia F-Droid.