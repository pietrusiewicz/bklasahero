#!/usr/bin/env python3
"""Fetch Polish localities from the OpenStreetMap Overpass API and build a
deterministic, offline-friendly CSV dataset for the bklasahero game.

Output contract (data/places/places_pl.csv):
    osm_id,name,place,population,lat,lon,voivodeship
  - UTF-8, LF line endings, no BOM, minimal CSV quoting.
  - place in {city, town, village}; villages need a parseable population tag
    (>= --min-village-population); city/town rows without population get 0.
  - lat/lon: 5 decimal places; ways/relations use the element center.
  - voivodeship: Polish adjective form, lowercase, with diacritics.
  - Rows sorted by osm_id ascending; osm_id unique across rows.

Data (c) OpenStreetMap contributors, licensed ODbL 1.0. See
data/places/ATTRIBUTION.md.

Standard library only. Safe to re-run: raw Overpass responses are cached
(gzip) under tools/geo/cache/ and reused unless --refresh is given.

Usage:
    python3 tools/geo/fetch_places.py            # build (uses cache if present)
    python3 tools/geo/fetch_places.py --refresh  # force refetch
    python3 tools/geo/fetch_places.py --check    # validate existing CSV
    python3 tools/geo/fetch_places.py --verify-areas  # re-verify area names
"""

import argparse
import csv
import gzip
import io
import json
import re
import sys
import time
import unicodedata
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

# --------------------------------------------------------------------------
# Constants
# --------------------------------------------------------------------------

SCRIPT_DIR = Path(__file__).resolve().parent

# Polite-fetch settings.
USER_AGENT = "bklasahero-geo-prep/0.1 (offline dataset for an F-Droid game)"
HTTP_HEADERS = {"User-Agent": USER_AGENT, "Accept": "application/json"}
SLEEP_BETWEEN_QUERIES = 4.0   # seconds, within the required 3-5 s window
RETRY_WAIT = 20.0             # seconds to wait after an HTTP error/429/504
MAX_ATTEMPTS_PER_ENDPOINT = 3 # initial try + 2 retries on the same endpoint
HTTP_TIMEOUT = 240            # socket timeout; query itself has [timeout:180]

DEFAULT_ENDPOINT = "https://overpass.osm.ch/api/interpreter"
FALLBACK_ENDPOINTS = [
    "https://z.overpass-api.de/api/interpreter",
    "https://overpass-api.de/api/interpreter",
]

# The 16 Polish voivodeships. For each: the CSV value (adjective form without
# "województwo", lowercase, with diacritics), the exact OSM name:pl of the
# admin_level=4 boundary relation, and that relation's id.
#
# name:pl values and relation ids verified against OSM on 2026-09-30 via
# rel["admin_level"="4"]["boundary"="administrative"](48.5,13.5,55.5,24.5).
# Note: OSM stores these names in lowercase ("województwo mazowieckie").
# Ordered by ISO 3166-2:PL code for a stable, deterministic processing order.
VOIVODESHIPS = [
    # value                    name:pl                                  rel id    ISO
    ("dolnośląskie",           "województwo dolnośląskie",              224457),  # PL-02
    ("kujawsko-pomorskie",     "województwo kujawsko-pomorskie",        223407),  # PL-04
    ("lubelskie",              "województwo lubelskie",                 130919),  # PL-06
    ("lubuskie",               "województwo lubuskie",                  130969),  # PL-08
    ("łódzkie",                "województwo łódzkie",                   224458),  # PL-10
    ("małopolskie",            "województwo małopolskie",               224459),  # PL-12
    ("mazowieckie",            "województwo mazowieckie",               130935),  # PL-14
    ("opolskie",               "województwo opolskie",                  224460),  # PL-16
    ("podkarpackie",           "województwo podkarpackie",              130957),  # PL-18
    ("podlaskie",              "województwo podlaskie",                 224461),  # PL-20
    ("pomorskie",              "województwo pomorskie",                 130975),  # PL-22
    ("śląskie",                "województwo śląskie",                   224462),  # PL-24
    ("świętokrzyskie",         "województwo świętokrzyskie",            130914),  # PL-26
    ("warmińsko-mazurskie",    "województwo warmińsko-mazurskie",       223408),  # PL-28
    ("wielkopolskie",          "województwo wielkopolskie",             130971),  # PL-30
    ("zachodniopomorskie",     "województwo zachodniopomorskie",        104401),  # PL-32
]
VOIVODESHIP_VALUES = [v[0] for v in VOIVODESHIPS]
VOIVODESHIP_SET = set(VOIVODESHIP_VALUES)

ALLOWED_PLACES = ("city", "town", "village")
LAT_RANGE = (48.9, 55.0)
LON_RANGE = (14.0, 24.5)

CSV_HEADER = ["osm_id", "name", "place", "population", "lat", "lon", "voivodeship"]

# If a way/relation id ever collides with an id already used by another row,
# this offset is added (repeatedly if needed) to keep osm_id unique. Node ids
# are always kept verbatim ("positive = OSM node id" per the contract).
ID_UNIQUENESS_OFFSET = 10**12

# Threshold escalation ladder when the CSV exceeds --max-bytes: the village
# population minimum is raised (re-filtering from cache, never refetching).
VILLAGE_THRESHOLD_LADDER = (300, 500, 700)


def log(msg: str) -> None:
    print(msg, file=sys.stderr, flush=True)


# --------------------------------------------------------------------------
# Overpass fetching (with cache, retries, mirror fallback)
# --------------------------------------------------------------------------

def build_place_query(name_pl: str) -> str:
    """Overpass QL: all city/town/village elements inside one voivodeship area.

    `out center tags;` keeps responses small while giving ways/relations a
    usable center point.
    """
    return (
        '[out:json][timeout:180];\n'
        f'area["name:pl"="{name_pl}"]["admin_level"="4"]["boundary"="administrative"]->.a;\n'
        '(nwr["place"~"^(city|town|village)$"](area.a););\n'
        'out center tags;\n'
    )


def build_area_verify_query() -> str:
    """Overpass QL: all admin_level=4 boundary relations inside Poland's bbox."""
    return (
        '[out:json][timeout:120];\n'
        'rel["admin_level"="4"]["boundary"="administrative"](48.5,13.5,55.5,24.5);\n'
        'out tags;\n'
    )


def http_post(endpoint: str, query: str) -> bytes:
    """POST an Overpass QL query and return the raw response body bytes."""
    data = urllib.parse.urlencode({"data": query}).encode("utf-8")
    req = urllib.request.Request(endpoint, data=data, headers=HTTP_HEADERS)
    with urllib.request.urlopen(req, timeout=HTTP_TIMEOUT) as resp:
        return resp.read()


def fetch_raw(endpoint: str, query: str, what: str) -> bytes:
    """Fetch `query` trying each endpoint in order; raise RuntimeError if all fail.

    Per endpoint: initial try + up to 2 retries, waiting RETRY_WAIT seconds
    after each failure (HTTP error, 429/504, timeout, malformed JSON, or an
    implausible empty result). Then fall back to the next mirror.
    """
    endpoints = [endpoint] + [e for e in FALLBACK_ENDPOINTS if e != endpoint]
    for ep in endpoints:
        for attempt in range(1, MAX_ATTEMPTS_PER_ENDPOINT + 1):
            t0 = time.time()
            try:
                body = http_post(ep, query)
                # Sanity: must be JSON with a non-empty elements list. Every
                # Polish voivodeship contains hundreds of settlements, so an
                # empty result means a broken/stub mirror response, not data.
                parsed = json.loads(body.decode("utf-8"))
                elements = parsed.get("elements")
                if elements is None:
                    raise ValueError("response has no 'elements' key")
                if len(elements) == 0:
                    raise ValueError(
                        "empty elements list (stub/broken mirror response; "
                        f"osm3s={parsed.get('osm3s', {})})"
                    )
                log(f"  {what}: OK from {ep} "
                    f"({len(elements)} elements, {time.time() - t0:.1f} s, "
                    f"attempt {attempt})")
                return body
            except Exception as exc:  # noqa: BLE001 - any failure is retryable
                log(f"  {what}: attempt {attempt}/{MAX_ATTEMPTS_PER_ENDPOINT} "
                    f"on {ep} failed: {type(exc).__name__}: {str(exc)[:160]}")
                if attempt < MAX_ATTEMPTS_PER_ENDPOINT:
                    log(f"  waiting {RETRY_WAIT:.0f} s before retry ...")
                    time.sleep(RETRY_WAIT)
        log(f"  {what}: endpoint {ep} exhausted, falling back to next mirror")
    raise RuntimeError(f"all endpoints failed for {what}")


def get_voivodeship_payload(voiv: str, name_pl: str, endpoint: str,
                            cache_dir: Path, refresh: bool) -> tuple[bytes, str]:
    """Return (raw JSON bytes, source) for one voivodeship, using cache if possible."""
    # Cache file names use the voivodeship value verbatim (UTF-8 file names).
    cache_path = cache_dir / f"{voiv}.json.gz"
    if cache_path.exists() and not refresh:
        log(f"  {voiv}: cache hit {cache_path.name}")
        return cache_path.read_bytes(), f"cache:{cache_path.name}"
    body = fetch_raw(endpoint, build_place_query(name_pl), voiv)
    # Cache the raw response gzip-compressed (create dir if needed).
    cache_dir.mkdir(parents=True, exist_ok=True)
    cache_path.write_bytes(gzip.compress(body, mtime=0))  # mtime=0: byte-stable cache
    log(f"  {voiv}: cached -> {cache_path.name}")
    return body, f"network:{endpoint}"


# --------------------------------------------------------------------------
# Parsing / filtering / deduplication
# --------------------------------------------------------------------------

def parse_population(raw) -> int | None:
    """Parse the OSM population tag into an int, or None if missing/unparseable."""
    if raw is None:
        return None
    s = str(raw).strip().replace(" ", "").replace("\u00a0", "")
    if not re.fullmatch(r"\d+", s):
        return None
    try:
        return int(s)
    except ValueError:
        return None


def normalize_name(name: str) -> str:
    """Collapse any internal whitespace; keeps diacritics untouched."""
    return " ".join(str(name).split())


def parse_element(elem: dict, voiv: str, stats: dict) -> dict | None:
    """Convert one Overpass element into a candidate row dict, or None to skip.

    Applies: name presence, place whitelist, coordinate presence and Poland
    bounding box, population rules for villages. Threshold-independent steps
    only -- the village population threshold is applied later so escalation
    can re-filter from cache without reparsing.
    """
    tags = elem.get("tags") or {}
    etype = elem.get("type")
    eid = elem.get("id")
    if etype not in ("node", "way", "relation") or not isinstance(eid, int):
        stats["skipped_malformed"] += 1
        return None

    place = tags.get("place")
    if place not in ALLOWED_PLACES:
        stats["skipped_place"] += 1
        return None

    raw_name = tags.get("name") or tags.get("name:pl")
    if raw_name is None or not str(raw_name).strip():
        stats["skipped_no_name"] += 1
        return None
    name = normalize_name(raw_name)
    if name != str(raw_name):
        stats["names_normalized"] += 1

    # Coordinates: nodes carry lat/lon; ways/relations use `center`.
    if etype == "node":
        lat, lon = elem.get("lat"), elem.get("lon")
    else:
        center = elem.get("center") or {}
        lat, lon = center.get("lat"), center.get("lon")
    if not isinstance(lat, (int, float)) or not isinstance(lon, (int, float)):
        stats["skipped_no_coords"] += 1
        return None

    # Validation box for Poland; reject (and count) anything outside.
    if not (LAT_RANGE[0] <= lat <= LAT_RANGE[1] and LON_RANGE[0] <= lon <= LON_RANGE[1]):
        stats["skipped_out_of_bounds"] += 1
        return None

    pop_parsed = parse_population(tags.get("population"))
    has_pop_tag = "population" in tags
    if place == "village" and pop_parsed is None:
        # Villages without a parseable population tag are dropped entirely.
        stats["villages_dropped_no_population"] += 1
        return None
    if place in ("city", "town") and pop_parsed is None:
        pop_parsed = 0
        if has_pop_tag:
            stats["population_unparseable_zeroed"] += 1
        else:
            stats["population_missing_zeroed"] += 1

    return {
        "etype": etype,
        "eid": eid,
        "name": name,
        "place": place,
        "pop": pop_parsed,
        "has_pop_tag": has_pop_tag and pop_parsed is not None,
        "lat": lat,
        "lon": lon,
        "voiv": voiv,
    }


def dedupe(candidates: list[dict], stats: dict) -> list[dict]:
    """Apply the contract's deduplication rules.

    1. Same element (type, id) returned twice (e.g. border-straddling element
       matched by two voivodeship areas): keep the first in canonical order.
    2. Same settlement under key (name, round(lat,3), round(lon,3)): keep one
       row, preferring (a) an element with a parseable population tag,
       (b) a node over way/relation, (c) the lower osm_id.
    """
    # Step 1: element-identity dedup.
    seen_elems: set[tuple[str, int]] = set()
    unique_elems: list[dict] = []
    for cand in candidates:
        key = (cand["etype"], cand["eid"])
        if key in seen_elems:
            stats["dupes_same_element"] += 1
            continue
        seen_elems.add(key)
        unique_elems.append(cand)

    # Step 2: settlement dedup.
    type_rank = {"node": 0, "way": 1, "relation": 2}
    groups: dict[tuple, list[dict]] = {}
    for cand in unique_elems:
        key = (cand["name"], round(cand["lat"], 3), round(cand["lon"], 3))
        groups.setdefault(key, []).append(cand)
    winners: list[dict] = []
    for key in groups:
        group = groups[key]
        if len(group) > 1:
            stats["settlements_deduped"] += len(group) - 1
        # (-has_pop_tag, type_rank, eid) -> population first, then node, then id.
        group.sort(key=lambda c: (not c["has_pop_tag"], type_rank[c["etype"]], c["eid"]))
        winners.append(group[0])
    return winners


def assign_osm_ids(rows: list[dict], stats: dict) -> None:
    """Set final unique osm_id values.

    Nodes keep their id verbatim. Ways/relations keep their id too, unless it
    collides with an id already used by another row -- then ID_UNIQUENESS_OFFSET
    is added (deterministically, processing rows by id) to keep osm_id unique.
    """
    used: set[int] = set()
    # Nodes first: their ids are canonical per the contract.
    for row in rows:
        if row["etype"] == "node":
            row["osm_id"] = row["eid"]
            used.add(row["osm_id"])
    # Then ways/relations in ascending id order.
    others = sorted((r for r in rows if r["etype"] != "node"),
                    key=lambda r: (type_rank_of(r["etype"]), r["eid"]))
    for row in others:
        stats[f"elements_{row['etype']}"] += 1
        cand = row["eid"]
        while cand in used:
            cand += ID_UNIQUENESS_OFFSET
            stats["osm_id_offsets_applied"] += 1
        row["osm_id"] = cand
        used.add(cand)


def type_rank_of(etype: str) -> int:
    return {"node": 0, "way": 1, "relation": 2}[etype]


def apply_village_threshold(rows: list[dict], threshold: int, stats: dict) -> list[dict]:
    """Drop villages below the population threshold (cities/towns always kept)."""
    kept = []
    dropped = 0
    for row in rows:
        if row["place"] == "village" and row["pop"] < threshold:
            dropped += 1
            continue
        kept.append(row)
    stats["villages_dropped_below_threshold"] = dropped
    return kept


# --------------------------------------------------------------------------
# CSV rendering
# --------------------------------------------------------------------------

def render_csv(rows: list[dict]) -> bytes:
    """Render final rows to CSV bytes: header, minimal quoting, LF, no BOM."""
    buf = io.StringIO()
    writer = csv.writer(buf, quoting=csv.QUOTE_MINIMAL, lineterminator="\n")
    writer.writerow(CSV_HEADER)
    for row in sorted(rows, key=lambda r: r["osm_id"]):
        writer.writerow([
            row["osm_id"],
            row["name"],
            row["place"],
            row["pop"],
            f"{row['lat']:.5f}",
            f"{row['lon']:.5f}",
            row["voiv"],
        ])
    return buf.getvalue().encode("utf-8")


# --------------------------------------------------------------------------
# Build pipeline
# --------------------------------------------------------------------------

def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Build/validate the Polish localities CSV from OSM Overpass.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    p.add_argument("--endpoint", default=DEFAULT_ENDPOINT,
                   help="primary Overpass API endpoint (falls back to "
                        + ", ".join(FALLBACK_ENDPOINTS) + " on failure)")
    p.add_argument("--out", default=DEFAULT_OUT_RELATIVE,
                   help="output CSV path; the default (%(default)s) is resolved "
                        "relative to this script, other relative paths resolve "
                        "against the current working directory; absolute paths "
                        "accepted")
    p.add_argument("--cache-dir", default=str(SCRIPT_DIR / "cache"),
                   help="directory for gzip-compressed raw Overpass responses")
    p.add_argument("--min-village-population", type=int, default=300,
                   help="minimum population for place=village rows")
    p.add_argument("--max-bytes", type=int, default=921600,
                   help="maximum CSV size; if exceeded, the village population "
                        "threshold escalates 300->500->700 (refilter from cache)")
    p.add_argument("--refresh", action="store_true",
                   help="ignore the cache and refetch everything")
    p.add_argument("--check", action="store_true",
                   help="validate the CSV at --out against the contract and "
                        "print a report; exit non-zero on any violation")
    p.add_argument("--verify-areas", action="store_true",
                   help="re-verify the 16 voivodeship name:pl values against "
                        "OSM and exit (does not build the CSV)")
    return p.parse_args(argv)


DEFAULT_OUT_RELATIVE = "../../data/places/places_pl.csv"  # relative to the script


def resolve_out_path(out_arg: str) -> Path:
    """Resolve --out: absolute paths as-is; the built-in default (given as a
    script-relative path) anchored to the script directory; any other relative
    path anchored to the current working directory."""
    p = Path(out_arg)
    if p.is_absolute():
        return p.resolve()
    if out_arg == DEFAULT_OUT_RELATIVE:
        return (SCRIPT_DIR / DEFAULT_OUT_RELATIVE).resolve()
    return p.resolve()


def do_verify_areas(endpoint: str) -> int:
    """Re-verify the hardcoded name:pl values against live OSM data."""
    log("Verifying admin_level=4 area names ...")
    body = fetch_raw(endpoint, build_area_verify_query(), "area-verify")
    parsed = json.loads(body.decode("utf-8"))
    found = {}
    for elem in parsed["elements"]:
        tags = elem.get("tags") or {}
        np = tags.get("name:pl", "")
        if np.lower().startswith("województwo"):
            found[np] = elem.get("id")
    ok = True
    for voiv, name_pl, rel_id in VOIVODESHIPS:
        got = found.get(name_pl)
        status = "OK" if got == rel_id else f"MISMATCH (got rel {got})"
        if got != rel_id:
            ok = False
        print(f"{voiv:24s} {name_pl:38s} rel {rel_id}  {status}")
    extra = set(found) - {v[1] for v in VOIVODESHIPS}
    if extra:
        ok = False
        print("Unexpected extra areas:", sorted(extra))
    print("AREA VERIFICATION:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


def do_build(args: argparse.Namespace) -> int:
    out_path = resolve_out_path(args.out)
    cache_dir = Path(args.cache_dir).resolve()
    cache_dir.mkdir(parents=True, exist_ok=True)

    log(f"Endpoint: {args.endpoint}  (fallbacks: {FALLBACK_ENDPOINTS})")
    log(f"Cache dir: {cache_dir}")
    log(f"Output: {out_path}")

    # ---- Stage 1: fetch (or load cached) raw responses, parse elements ----
    sources: dict[str, str] = {}
    candidates: list[dict] = []
    fetch_stats = {
        "skipped_malformed": 0, "skipped_place": 0, "skipped_no_name": 0,
        "skipped_no_coords": 0, "skipped_out_of_bounds": 0,
        "names_normalized": 0, "villages_dropped_no_population": 0,
        "population_missing_zeroed": 0, "population_unparseable_zeroed": 0,
        "dupes_same_element": 0, "settlements_deduped": 0,
        "osm_id_offsets_applied": 0, "elements_way": 0, "elements_relation": 0,
    }
    per_voiv_raw: dict[str, int] = {}
    network_fetches = 0
    for i, (voiv, name_pl, _rel) in enumerate(VOIVODESHIPS):
        body, source = get_voivodeship_payload(
            voiv, name_pl, args.endpoint, cache_dir, args.refresh)
        sources[voiv] = source
        if source.startswith("network"):
            network_fetches += 1
            if i < len(VOIVODESHIPS) - 1:
                time.sleep(SLEEP_BETWEEN_QUERIES)  # be polite between queries
        parsed = json.loads(gzip.decompress(body).decode("utf-8")
                            if source.startswith("cache") else body.decode("utf-8"))
        elements = parsed.get("elements", [])
        per_voiv_raw[voiv] = len(elements)
        for elem in elements:
            row = parse_element(elem, voiv, fetch_stats)
            if row is not None:
                candidates.append(row)

    log(f"Parsed {len(candidates)} candidate rows from "
        f"{sum(per_voiv_raw.values())} raw elements "
        f"({network_fetches} network fetches, "
        f"{len(VOIVODESHIPS) - network_fetches} cache hits)")

    # ---- Stage 2: dedupe, assign ids (threshold-independent) ----
    deduped = dedupe(candidates, fetch_stats)
    assign_osm_ids(deduped, fetch_stats)

    # ---- Stage 3: village threshold, with escalation if CSV too big ----
    thresholds = [args.min_village_population]
    thresholds += [t for t in VILLAGE_THRESHOLD_LADDER
                   if t > args.min_village_population]
    final_rows = None
    used_threshold = None
    csv_bytes = None
    for threshold in thresholds:
        threshold_stats = {}
        rows = apply_village_threshold(deduped, threshold, threshold_stats)
        csv_bytes = render_csv(rows)
        log(f"threshold={threshold}: {len(rows)} rows, {len(csv_bytes)} bytes "
            f"(max {args.max_bytes})")
        if len(csv_bytes) <= args.max_bytes:
            final_rows, used_threshold = rows, threshold
            # Surface the winning iteration's threshold stats in the report.
            fetch_stats.update(threshold_stats)
            break
    if final_rows is None:
        log("ERROR: CSV still exceeds --max-bytes at threshold "
            f"{thresholds[-1]}; aborting without writing.")
        return 2

    # ---- Stage 4: final validation pass before writing ----
    violations = validate_rows(final_rows)
    if violations:
        log("ERROR: internal validation failed:")
        for v in violations[:20]:
            log(f"  {v}")
        return 2

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(csv_bytes)
    log(f"Wrote {out_path} ({len(csv_bytes)} bytes, {len(final_rows)} data rows, "
        f"village threshold {used_threshold})")

    # ---- Report ----
    print("\n=== BUILD REPORT ===")
    print(f"endpoints used: {sorted(set(sources.values()))}")
    print(f"village population threshold: {used_threshold}")
    print(f"rows total: {len(final_rows)}")
    for place in ALLOWED_PLACES:
        n = sum(1 for r in final_rows if r["place"] == place)
        print(f"  {place}: {n}")
    print("rows per voivodeship:")
    for voiv in VOIVODESHIP_VALUES:
        n = sum(1 for r in final_rows if r["voiv"] == voiv)
        flag = "  <-- SUSPICIOUSLY FEW" if n < 50 else ""
        print(f"  {voiv:24s} raw {per_voiv_raw.get(voiv, 0):6d} -> kept {n:6d}{flag}")
    print("parse/dedup stats:")
    for k in sorted(fetch_stats):
        print(f"  {k}: {fetch_stats[k]}")
    return 0


def validate_rows(rows: list[dict]) -> list[str]:
    """Final in-memory validation against the contract. Returns violations."""
    problems = []
    seen_ids: set[int] = set()
    for row in rows:
        if not row["name"]:
            problems.append(f"empty name for osm_id {row['eid']}")
        if row["place"] not in ALLOWED_PLACES:
            problems.append(f"bad place {row['place']!r}")
        if not (LAT_RANGE[0] <= row["lat"] <= LAT_RANGE[1]):
            problems.append(f"lat out of range: {row['lat']}")
        if not (LON_RANGE[0] <= row["lon"] <= LON_RANGE[1]):
            problems.append(f"lon out of range: {row['lon']}")
        if not (isinstance(row["pop"], int) and row["pop"] >= 0):
            problems.append(f"bad population {row['pop']!r}")
        if row["voiv"] not in VOIVODESHIP_SET:
            problems.append(f"bad voivodeship {row['voiv']!r}")
        oid = row.get("osm_id")
        if not isinstance(oid, int) or oid <= 0:
            problems.append(f"bad osm_id {oid!r}")
        elif oid in seen_ids:
            problems.append(f"duplicate osm_id {oid}")
        else:
            seen_ids.add(oid)
    return problems


# --------------------------------------------------------------------------
# --check: validate an existing CSV against the contract
# --------------------------------------------------------------------------

LATLON_RE = re.compile(r"^-?\d+\.\d{5}$")


def do_check(args: argparse.Namespace) -> int:
    out_path = resolve_out_path(args.out)
    print(f"Checking {out_path} ...")
    violations: list[str] = []
    if not out_path.exists():
        print("VIOLATION: file does not exist")
        return 1

    raw = out_path.read_bytes()
    size = len(raw)

    # Encoding / line endings / BOM.
    if raw.startswith(b"\xef\xbb\xbf"):
        violations.append("file has a UTF-8 BOM")
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError as exc:
        print(f"VIOLATION: not valid UTF-8: {exc}")
        return 1
    if b"\r" in raw:
        violations.append("file contains CR bytes (must be LF-only)")
    if text and not text.endswith("\n"):
        violations.append("file does not end with a newline")

    lines = text.split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    if not lines:
        violations.append("file is empty")
        print_report(violations, {}, {}, {}, size, args)
        return 1 if violations else 0

    # Header.
    if lines[0] != ",".join(CSV_HEADER):
        violations.append(f"bad header: {lines[0]!r}")

    rows = []
    reader = csv.reader(io.StringIO(text))
    header = next(reader, None)
    if header != CSV_HEADER:
        violations.append(f"parsed header mismatch: {header!r}")
    for lineno, fields in enumerate(reader, start=2):
        if len(fields) != 7:
            violations.append(f"line {lineno}: expected 7 fields, got {len(fields)}")
            continue
        osm_id_s, name, place, pop_s, lat_s, lon_s, voiv = fields
        row_bad = False
        # osm_id
        try:
            osm_id = int(osm_id_s)
            if osm_id <= 0:
                raise ValueError("non-positive")
        except ValueError:
            violations.append(f"line {lineno}: bad osm_id {osm_id_s!r}")
            row_bad = True
            osm_id = None
        # name
        if not name.strip():
            violations.append(f"line {lineno}: empty name")
            row_bad = True
        if name != normalize_name(name):
            violations.append(f"line {lineno}: name has odd whitespace {name!r}")
            row_bad = True
        try:
            unicodedata.name(name[0])
        except (ValueError, IndexError):
            violations.append(f"line {lineno}: name starts with a control char")
            row_bad = True
        # place
        if place not in ALLOWED_PLACES:
            violations.append(f"line {lineno}: bad place {place!r}")
            row_bad = True
        # population
        try:
            pop = int(pop_s)
            if pop < 0 or str(pop) != pop_s:
                raise ValueError("negative or non-canonical")
        except ValueError:
            violations.append(f"line {lineno}: bad population {pop_s!r}")
            row_bad = True
            pop = None
        if place == "village" and pop is not None:
            if pop == 0:
                violations.append(f"line {lineno}: village with population 0 "
                                  "(villages require a parseable population)")
                row_bad = True
            elif pop < args.min_village_population:
                violations.append(f"line {lineno}: village population {pop} < "
                                  f"--min-village-population "
                                  f"{args.min_village_population}")
                row_bad = True
        # lat/lon
        lat = lon = None
        if not LATLON_RE.match(lat_s):
            violations.append(f"line {lineno}: bad lat format {lat_s!r}")
            row_bad = True
        else:
            lat = float(lat_s)
            if not (LAT_RANGE[0] <= lat <= LAT_RANGE[1]):
                violations.append(f"line {lineno}: lat out of range {lat_s}")
                row_bad = True
        if not LATLON_RE.match(lon_s):
            violations.append(f"line {lineno}: bad lon format {lon_s!r}")
            row_bad = True
        else:
            lon = float(lon_s)
            if not (LON_RANGE[0] <= lon <= LON_RANGE[1]):
                violations.append(f"line {lineno}: lon out of range {lon_s}")
                row_bad = True
        # voivodeship
        if voiv not in VOIVODESHIP_SET:
            violations.append(f"line {lineno}: bad voivodeship {voiv!r}")
            row_bad = True
        # minimal quoting: re-render the parsed fields and compare with the
        # raw line (a canonically minimal re-rendering must round-trip)
        buf = io.StringIO()
        csv.writer(buf, quoting=csv.QUOTE_MINIMAL, lineterminator="").writerow(fields)
        if buf.getvalue() != lines[lineno - 1]:
            violations.append(f"line {lineno}: not minimally quoted: "
                              f"{lines[lineno - 1]!r}")
            row_bad = True
        if not row_bad and osm_id is not None:
            rows.append((osm_id, name, place, pop, lat, lon, voiv))

    # Uniqueness + sort order.
    ids = [r[0] for r in rows]
    if len(ids) != len(set(ids)):
        dupes = sorted({i for i in ids if ids.count(i) > 1})
        violations.append(f"duplicate osm_id values: {dupes[:10]}")
    if ids != sorted(ids):
        violations.append("rows are not sorted by osm_id ascending")

    # Size.
    if size > args.max_bytes:
        violations.append(f"file size {size} exceeds --max-bytes {args.max_bytes}")

    per_place: dict[str, int] = {}
    per_voiv: dict[str, int] = {}
    for _oid, _name, place, _pop, _lat, _lon, voiv in rows:
        per_place[place] = per_place.get(place, 0) + 1
        per_voiv[voiv] = per_voiv.get(voiv, 0) + 1
    sparse = {v: n for v, n in per_voiv.items() if n < 50}

    print_report(violations, per_place, per_voiv, sparse, size, args)
    return 1 if violations else 0


def print_report(violations, per_place, per_voiv, sparse, size, args) -> None:
    print("\n=== CHECK REPORT ===")
    print(f"file size: {size} bytes ({size / 1024:.1f} KB, limit {args.max_bytes})")
    print(f"data rows: {sum(per_place.values())}")
    for place in ALLOWED_PLACES:
        print(f"  {place}: {per_place.get(place, 0)}")
    print("rows per voivodeship:")
    for voiv in VOIVODESHIP_VALUES:
        n = per_voiv.get(voiv, 0)
        flag = "  <-- SUSPICIOUSLY FEW (<50)" if n < 50 else ""
        print(f"  {voiv:24s} {n:6d}{flag}")
    if violations:
        print(f"\nVIOLATIONS ({len(violations)}):")
        for v in violations[:50]:
            print(f"  - {v}")
        if len(violations) > 50:
            print(f"  ... and {len(violations) - 50} more")
        print("RESULT: FAIL")
    else:
        print("\nRESULT: PASS (no contract violations)")


# --------------------------------------------------------------------------

def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    if args.verify_areas:
        return do_verify_areas(args.endpoint)
    if args.check:
        return do_check(args)
    return do_build(args)


if __name__ == "__main__":
    sys.exit(main())
