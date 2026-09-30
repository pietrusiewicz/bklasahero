# places_pl.csv — Polish localities dataset

Offline dataset of Polish localities (cities, towns, villages) used by the
game to generate local football clubs near the player's home city.
Source: OpenStreetMap via the Overpass API; see [ATTRIBUTION.md](ATTRIBUTION.md)
for licensing (ODbL 1.0), provenance, and the exact selection rules.

## Format

UTF-8, LF line endings, no BOM, minimal CSV quoting (a field is quoted only
if it contains a comma or a double quote; quotes escaped by doubling).
Rows sorted by `osm_id` ascending. Header row:

```
osm_id,name,place,population,lat,lon,voivodeship
```

## Schema

| column        | type    | allowed values / notes |
|---------------|---------|------------------------|
| `osm_id`      | integer | OSM element id, > 0, unique across rows. Positive = OSM node id for node elements; way/relation ids are used verbatim (offset by +10¹² only on collision — none in this build). |
| `name`        | string  | OSM `name` (fallback `name:pl`), non-empty, may contain Polish diacritics (ą ć ę ł ń ó ś ź ż). |
| `place`       | string  | exactly one of `city`, `town`, `village` (lowercase). |
| `population`  | integer | ≥ 0. From the OSM `population` tag; `0` only allowed for `city`/`town` (villages require a parseable population). |
| `lat`         | decimal | 5 decimal places, dot separator, 48.9 ≤ lat ≤ 55.0. Ways/relations use the element center. |
| `lon`         | decimal | 5 decimal places, dot separator, 14.0 ≤ lon ≤ 24.5. |
| `voivodeship` | string  | lowercase Polish adjective, no "województwo", one of the 16: dolnośląskie, kujawsko-pomorskie, lubelskie, lubuskie, łódzkie, małopolskie, mazowieckie, opolskie, podkarpackie, podlaskie, pomorskie, śląskie, świętokrzyskie, warmińsko-mazurskie, wielkopolskie, zachodniopomorskie. |

## Contents (build of 2026-09-30)

- **Total rows: 7,778** (file size 491,146 bytes ≈ 479.6 KB; limit 900 KB)
- By place type: **city 66, town 960, village 6,752**
- Element types: 7,775 nodes, 1 way, 2 relations
- Village population threshold: 300 (escalation to 500/700 not needed)
- All 1,026 city/town rows have a population > 0

Rows per voivodeship:

| voivodeship            | rows |
|------------------------|------|
| dolnośląskie           |  550 |
| kujawsko-pomorskie     |  355 |
| lubelskie              |  458 |
| lubuskie               |  257 |
| łódzkie                |  429 |
| małopolskie            |  592 |
| mazowieckie            |  742 |
| opolskie               |  323 |
| podkarpackie           |  550 |
| podlaskie              |  196 |
| pomorskie              |  669 |
| śląskie                |  604 |
| świętokrzyskie         |  544 |
| warmińsko-mazurskie    |  371 |
| wielkopolskie          |  778 |
| zachodniopomorskie     |  360 |

**Coverage note**: villages appear only where OSM carries a parseable
`population` tag ≥ 300 (per the dataset contract). OSM population coverage
for Polish villages is partial, so village density varies strongly by region
(e.g. podlaskie keeps 196 rows vs. 778 for wielkopolskie). For the game's
"clubs within radius" logic, a small fraction of border towns (Gołdap,
Świnoujście, Krynki, Lipsk) have fewer than 10 neighbouring localities
within 30 km — see the fallback-radius discussion in the build report.

## Regeneration

From the repository root (`bklasahero/`):

```sh
python3 tools/geo/fetch_places.py            # rebuild, reusing tools/geo/cache/
python3 tools/geo/fetch_places.py --refresh  # refetch from Overpass (polite: 4 s between queries, retries + mirror fallback)
python3 tools/geo/fetch_places.py --check    # validate the CSV against the contract (exit != 0 on violation)
python3 tools/geo/fetch_places.py --verify-areas  # re-verify the 16 voivodeship area names against OSM
```

The build is deterministic: identical cache ⇒ byte-identical CSV.
Python ≥ 3.10, standard library only, no network needed when the cache under
`tools/geo/cache/` is populated.
