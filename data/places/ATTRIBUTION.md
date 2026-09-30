# ATTRIBUTION — places_pl.csv

## Copyright and license

- **Data**: © OpenStreetMap contributors.
- **License of the data**: Open Database License (ODbL) 1.0 —
  <https://opendatacommons.org/licenses/odbl/1-0/>.
- OSM's general copyright/attribution policy: <https://www.openstreetmap.org/copyright>.

**THIS DATA FILE (`places_pl.csv`) is licensed under ODbL 1.0.**
The **application source code** of bklasahero is licensed under
**GPL-3.0-or-later**. The two licenses cover separate works: this dataset is a
*database* distributed alongside the app, not part of the program's source code.

This file is a **derived database** under ODbL 1.0: it is a filtered and
transformed subset of OpenStreetMap data (selection, deduplication, field
projection, coordinate rounding, sorting). Distributing it (e.g. inside the
app's assets/APK) triggers ODbL attribution requirements; if this derived
database were itself modified and redistributed publicly, ODbL's share-alike
clause would apply **to the database**, not to the app's GPL-3.0-or-later code.

### Required on-screen attribution in the app

> © OpenStreetMap contributors, ODbL

Display this string wherever the app shows data derived from this file
(e.g. credits/about screen).

## Provenance (so a reviewer can reproduce this exact dataset)

- **Retrieval date**: 2026-09-30 (UTC), between 17:26:28Z and 17:39:41Z
  (per-response OSM base timestamps `timestamp_osm_base` range across the
  16 cached responses).
- **API**: Overpass API (generator `Overpass API 0.7.62.11 87bfad18`).
- **Endpoints used**:
  - Primary configured endpoint `https://overpass.osm.ch/api/interpreter`
    was attempted first but returned **empty stub responses** (HTTP 200,
    zero elements, bogus `timestamp_osm_base: 117393`) on the retrieval date
    and served no data.
  - **All 16 voivodeship responses were therefore fetched from
    `https://z.overpass-api.de/api/interpreter`** (automatic mirror fallback
    built into `tools/geo/fetch_places.py`).
  - Raw responses are cached gzip-compressed under
    `tools/geo/cache/<voivodeship>.json.gz` (not committed; see
    `tools/geo/.gitignore`).
- **HTTP headers sent**: `User-Agent: bklasahero-geo-prep/0.1 (offline dataset
  for an F-Droid game)`, `Accept: application/json`.
- **Query pattern** (one per voivodeship, 16 total; `name:pl` values and
  relation ids of the 16 `admin_level=4` areas verified against OSM on
  2026-09-30 — see `fetch_places.py --verify-areas`):

  ```overpassql
  [out:json][timeout:180];
  area["name:pl"="województwo <name>"]["admin_level"="4"]["boundary"="administrative"]->.a;
  (nwr["place"~"^(city|town|village)$"](area.a););
  out center tags;
  ```

## Exact selection rules applied

1. **Included elements**: OSM nodes/ways/relations tagged `place=city`,
   `place=town`, or `place=village` inside one of the 16 Polish voivodeship
   areas. Excluded: hamlets, suburbs, neighbourhoods, quarters, isolated
   dwellings, localities, and anything else.
2. **Name**: `name` tag, falling back to `name:pl`; rows without a non-empty
   name are dropped (13 elements dropped in this build).
3. **Population**: integer parsed from the `population` tag. Missing or
   unparseable: `city`/`town` rows are kept with population `0`; `village`
   rows are **dropped** (35,082 villages dropped for lack of a parseable
   population tag — OSM population coverage for Polish villages is partial).
4. **Village threshold**: `population >= 300` (5,486 villages dropped below
   threshold). Escalation policy if the CSV exceeded 921,600 bytes: raise to
   500, then 700 (refilter from cache). **Not triggered** — final size is
   491,146 bytes at threshold 300.
5. **Coordinates**: nodes use their `lat`/`lon`; ways/relations use the
   element `center`. Rounded to 5 decimal places. Rows outside
   48.9 ≤ lat ≤ 55.0 or 14.0 ≤ lon ≤ 24.5 are rejected (0 in this build).
6. **Deduplication**: same element returned twice (border overlap) → kept
   once; same settlement under key `(name, round(lat,3), round(lon,3))` →
   one row kept, preferring the element with a parseable population tag,
   then a node over way/relation, then the lower osm_id (1 settlement
   deduplicated in this build).
7. **osm_id**: the OSM element id (nodes keep their id verbatim; a way or
   relation id would be offset by +10¹² only if it collided with another
   row's id — 0 offsets needed here). Unique across rows; rows sorted by
   osm_id ascending.
8. **Voivodeship**: taken from the area query that returned the element,
   written as the lowercase Polish adjective without "województwo"
   (e.g. `mazowieckie`, `warmińsko-mazurskie`).

## Regeneration

```sh
python3 tools/geo/fetch_places.py            # rebuild (reuses cache)
python3 tools/geo/fetch_places.py --refresh  # rebuild with fresh network fetches
python3 tools/geo/fetch_places.py --check    # validate the CSV against the contract
```
