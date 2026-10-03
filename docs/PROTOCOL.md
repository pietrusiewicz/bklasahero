# Protokół rdzeń ↔ Android (mostek JNI)

Wersja protokołu: **1** · Wersja schematu save'a: **1** · Status: **obowiązujący kontrakt**

Ten dokument jest **jedynym źródłem prawdy** dla obu stron mostka:
implementacji C++ (`core/src/facade.cpp`) oraz klienta Kotlin
(`android/app/src/main/kotlin/pl/bklasahero/core/CoreClient.kt`).
Zmiana protokołu wymaga bumpnięcia `kProtocolVersion` w `core/include/bkh/facade.h`
i aktualizacji tego pliku.

## Zasady ogólne

1. **Dwa kanały.** Kanał kontrolny to JSON (polecenia rzadkie, czytelne, łatwe w
   debugowaniu). Kanał renderujący to surowy bufor `float[]` (60 fps, bez alokacji
   i bez parsowania po stronie Kotlin).
2. **Bez wyjątków przez JNI.** `Facade::command()` zawsze zwraca poprawny JSON;
   błąd jest sygnalizowany polem `ok:false`.
3. **Rdzeń nie zna platformy.** Nie otwiera plików, nie zna zegara, nie zna Androida.
   Dane (katalog miejscowości, save, znaczniki czasu) dostarcza warstwa Kotlin.
4. **Rdzeń nie zwraca przetłumaczonych zdań** — zwraca stabilne klucze ASCII
   (`messageKey`), które Android mapuje na `strings.xml` (`values/` = EN, `values-pl/` = PL).
5. **Determinizm.** Każde polecenie, które losuje, robi to przez strumień RNG
   wywiedziony z ziarna kariery i jawnych tagów. Ten sam save + te same polecenia
   = ten sam wynik (podstawa testów na hoście).

## Envelope odpowiedzi

```json
{"ok": true,  "data": { ... }}
{"ok": false, "error": {"code": "error.not_found", "message": "szczegóły dla logów"}}
```

Kody błędów: `core/include/bkh/result.h` (namespace `bkh::errc`).

---

## Kanał kontrolny — polecenia

| cmd | Pola żądania | `data` odpowiedzi | Uwagi |
|---|---|---|---|
| `version` | — | `core`, `protocol`, `schema`, `build` | Pierwsze wywołanie po `System.loadLibrary` |
| `catalog` | — | `places`, `checksum`, `voivodeships`, `largest`, `summary` | Wymaga wczytanego katalogu |
| `searchCity` | `query`, `limit?` (domyślnie 12) | `places[]` | Podpowiedzi do pola miasta; dopasowanie bez ogonków |
| `resolveCity` | `name` | `place`, `nearby15`, `nearby30`, `nearest[]` | Potwierdzenie wyboru miasta gracza |
| `newCareer` | `nickname`, `homeOsmId`, `language?`, `seed?`, `preferredClubName?` | `career` | Tworzy ligę startową wokół miasta |
| `career` | — | `career` | Pełny stan do odświeżenia UI |
| `table` | `round?` | `tier`, `rows[]`, `fixtures[]`, `round`, `playerMatch`, `seasonComplete` | Tabela z kryteriami pomocniczymi |
| `attributes` | — | `attributes`, `progression`, `xpToNext` | Ekran rozwoju |
| `upgrade` | `attribute` | `attributes`, `progression` | Klucze: `shot_power`, `shot_accuracy`, `composure`, `curve`, `reflexes`, `reach`, `reading`, `handling` |
| `cosmetics` | — | `items[]`, `equipped{}` | |
| `equip` | `kind`, `id` | `equipped{}` | `kind`: `kit`, `ball`, `gloves`, `net` |
| `achievements` | — | `items[]` | Klucz + warunek spełnienia + postęp |
| `stats` | — | `stats`, `history[]` | |
| `beginMatch` | — | `match` | Ustawia fazę `MatchInProgress`; zwraca `MatchSetup` |
| `matchState` | — | `shootout`, `next`, `role`, `pressure`, `suddenDeath`, `finished` | Stan bieżącego konkursu |
| `shoot` | `aimX`, `aimY`, `effort`, `spin`, `loft` | `kick`, `shootout`, `frame` | Rola gracza: **strzelec**. `frame.floats` = ile floatów pobrać kanałem renderującym |
| `dive` | `side`, `height`, `timingError` | `kick`, `shootout`, `frame` | Rola gracza: **bramkarz**; rozstrzyga rzut CPU |
| `finishMatch` | — | `outcome`, `xp`, `levelUps`, `skillPoints`, `achievements[]`, `season`? | Zapisuje wynik w lidze, przyznaje postęp |
| `nextRound` | — | `round`, `table[]`, `seasonComplete`, `fixtures[]` | Symuluje pozostałe mecze kolejki |
| `seasonSummary` | `nowMs?` | `summary`, `nextTier`, `nextSeason`, `promoted`, `relegated`, `fate` | Kończy sezon i buduje następny |
| `history` | — | `seasons[]` | Lista sezonów z `CareerState::history` |
| `acknowledgements` | — | `items[]` (`name`, `license`, `notice`, `url`) | W tym **atrybucja OpenStreetMap (ODbL)** |

### Obiekt `place`

```json
{"osmId": 531670, "name": "Radom", "type": "city", "population": 209296,
 "lat": 51.40253, "lon": 21.14714, "voivodeship": "mazowieckie", "voivodeshipId": 7}
```

### Obiekt `club`

```json
{"id": 4, "name": "LKS Orzeł Bartodzieje", "shortName": "Orzeł", "town": "Bartodzieje",
 "voivodeship": "mazowieckie", "population": 780, "strength": 41.5,
 "colors": {"primary": 4279645490, "secondary": 4294967295, "accent": 4279176736},
 "foundedYear": 1962, "isPlayer": false}
```

Kolory są liczbami całkowitymi ARGB (`0xAARRGGBB`) — Kotlin robi `Color(value.toULong())`.

### Kluby w `table[]` i `fixtures[]`

Wiersze `table[]` niosą `name`, `shortName` **oraz `town`** — miejscowość, z której
klub pochodzi. Nazwy klubów są generowane z okolicznych miejscowości (dla lig
regionalnych wybiera je `ClubGenerator::selectRegionalPlaces`: promień rośnie
maksymalnie 2,5× względem `TierInfo::radiusKm`, a dopełnienie idzie z tego samego
województwa), więc `town` pokazuje związek ligi z mapą. UI skleja `shortName`
z `town` („Orzeł Węgrzynowo"), pomijając miejscowość, gdy skrót już ją niesie
(forma przymiotnikowa, np. „Węgrzynowianka").

Pary w `fixtures[]` mają `home`/`away` (pełne nazwy), `homeShort`/`awayShort`
oraz `homeTown`/`awayTown`.

### Obiekt `kick` (rezultat rzutu)

```json
{"sequence": 4, "round": 2, "side": "home", "playerRole": "shooter", "suddenDeath": false,
 "scored": true, "outcome": "goal", "outcomeKey": "outcome.goal",
 "woodwork": "none", "keeperTouched": false, "caught": false, "fumbled": false,
 "crossingTimeS": 0.483, "flightTimeS": 0.612, "crossingSpeedMs": 21.4, "maxSpeedMs": 24.8,
 "saveDistanceM": 0.42, "marginXm": 0.31, "marginYm": 0.55,
 "intendedAim": [-1.85, 1.05], "actualAim": [-1.71, 1.12],
 "zone": "low_left", "keeperSide": "right", "keeperHeight": "mid", "keeperTimingErrorS": 0.0}
```

Klucze `outcomeKey`, `woodworkKey`, `zoneKey`, `keeperSideKey` są kluczami ASCII do
`strings.xml` — patrz `messageKey()` w `physics.h` / `shooter.h` / `keeper.h`.

### Obiekt `shootout`

```json
{"homeScore": 3, "awayScore": 2, "homeTaken": 4, "awayTaken": 4,
 "kicksPerSide": 5, "status": "in_progress", "suddenDeath": false,
 "winner": null, "decidedEarly": false, "kickCount": 8,
 "nextKicker": "home", "nextRound": 4, "playerRole": "shooter",
 "mustScore": true, "pressure": 0.86, "canStillWin": {"home": true, "away": true}}
```

### Obiekt `setup` (`MatchSetup`) — zwracany przez `beginMatch`

```json
{"fixture": {"away": 3, "home": 0, "matchIndex": 17, "round": 3},
 "isDecisive": false, "leagueLabel": "B klasa · mazowieckie",
 "distanceKm": 17.9,
 "opponent": "LKS Orzeł Bartodzieje", "opponentShort": "Orzeł", "opponentTown": "Bartodzieje",
 "playerClub": "KS Błysk Radom", "playerClubShort": "Błysk", "playerTown": "Radom",
 "playerShootsFirst": "home", "round": 3, "seasonNumber": 1,
 "map": {"homeLat": 51.4027, "homeLon": 21.1471, "awayLat": 52.1, "awayLon": 20.9,
         "places": [{"isPlayer": true, "lat": 51.4027, "lon": 21.1471,
                     "short": "Błysk", "town": "Radom"}, …]},
 "rules": {"firstKicker": "home", "kicksPerSide": 5, "maxSuddenDeathRounds": 10, "suddenDeath": false}}
```

Gracz zawsze gra stroną **Home** (`playerClub*`), przeciwnik — **Away** (`opponent*`).
`fixture.home`/`fixture.away` to indeksy klubów w `career.league.clubs`.
`playerTown`/`opponentTown` oraz `distanceKm` (odległość ortodromiczna między
tymi miejscowościami) napędzają zapowiedź meczu — patrz `MatchScreen.kt`.
`map.places[]` to wszystkie kluby bieżącej ligi z pozycjami; UI rysuje z tego
schematyczną mapkę okolicy (bez kafelków — gra nie ma uprawnienia INTERNET).

### Obiekt `career`

Zawiera: `state` (nickname, miasto, poziom ligi, numer sezonu, faza), `club` gracza,
`attributes`, `progression`, `stats`, `table[]` (skrót), `nextMatch`, `equipped{}`,
`language`, `seed`, `schemaVersion`.

---

## Kanał renderujący — bufor klatek

`Facade::writeFrame(float* dst, int capacity)` → liczba zapisanych floatów.
Układ (stałe w `frame::` w `facade.h`, mirror w `FrameBuffer.kt`):

| Indeks | Znaczenie |
|---|---|
| `[0]` | wersja układu (`frame::kLayoutVersion` = 1) |
| `[1]` | liczba próbek piłki |
| `[2]` | liczba próbek bramkarza |
| `[3]` | czas trwania odtwarzania [s] |
| `[4]` | czas kontaktu z bramkarzem [s], 0 = brak |
| `[5]` | czas przekroczenia linii bramkowej [s], 0 = brak |
| `[6]` | punkt przecięcia linii bramkowej — x [m] |
| `[7]` | punkt przecięcia linii bramkowej — y [m] |
| `[8 …]` | próbki piłki, po `7` floatów: `t, x, y, z, vx, vy, vz` |
| `[…]` | próbki bramkarza, po `6` floatów: `t, x, y, z, diveProgress, side` |

* Jednostki: metry i sekundy. Układ współrzędnych: `x` w bok (dodatni = prawo
  strzelca), `y` w górę, `z` w głąb (0 = punkt karny, 11 = linia bramkowa).
* `side`: 0 = lewo, 1 = środek, 2 = prawo (`DiveSide`).
* Jeśli `capacity` jest za mały, funkcja zwraca **liczbę ujemną** równą
  `-wymaganaLiczbaFloatów`; Kotlin powiększa bufor i woła ponownie.
* Próbki są gęstości `PhysicsParams::sampleInterval` (domyślnie 1/60 s).
  Renderer interpoluje między próbkami liniowo.

---

## Cykl życia sesji gry

```
loadLibrary → version → loadPlaces(bytes z assets) → catalog
  ├─ brak save'a: searchCity → resolveCity → newCareer → career
  └─ jest save:  loadCareerFromJson(blob z Room) → career

Pętla meczowa (jedna kolejka):
  table → beginMatch → matchState
    → (rola strzelca) shoot        → frame → writeFrame → animacja
    → (rola bramkarza) dive        → frame → writeFrame → animacja
    → matchState (aż do finished)
  → finishMatch → upgrade/cosmetics → nextRound → (powtórz)
  → seasonComplete → seasonSummary → nowy sezon
```

## Wymagania F-Droid wynikające z protokołu

* Brak permisji `android.permission.INTERNET` — protokół nie przewiduje żadnej
  komunikacji sieciowej, a katalog miejscowości jest wczytywany z `assets`.
* Brak identyfikatorów reklamowych, analityki i telemetrii w odpowiedziach.
* Atrybucja OpenStreetMap jest częścią protokołu (`acknowledgements`), więc ekran
  „O aplikacji" zawsze ją pokazuje — wymóg ODbL.
