# Dziennik zmian

Wszystkie istotne zmiany projektu są tu rejestrowane. Format inspirowany
[Keep a Changelog](https://keepachangelog.com/pl/1.1.0/), wersjonowanie zgodne
z [SemVer](https://semver.org/lang/pl/).

## [Niewydane]

### Zmienione

- **Menu główne przeprojektowane** (`ui/screens/HomeScreen.kt`): nagłówek z
  herbem, awatar i skrót kariery (nick, liga, sezon, miasto), duży przycisk
  **„Graj” przyklejony do dołu ekranu** oraz **przesuwane zakładki
  Tabela ↔ Terminarz ↔ Kariera** (`HorizontalPager` + `TabRow`). Emoji
  zastąpione ikonami Material. Wybrana zakładka żyje w stanie (`homeTab`), więc
  powrót do menu wraca na to samo miejsce, a przesunięcie palcem i tapnięcie w
  zakładkę działają tak samo. Podgląd układu:
  [`docs/menu-preview.png`](menu-preview.png).
- **Motyw i pomoc w nagłówku**: przełącznik trybu jasny/ciemny to teraz okrągły
  przycisk obok tytułu (zamiast wiersza w karcie opcji), a obok niego skrót do
  samouczka. Karta „Opcje” zniknęła — „Nowa kariera” przeniosła się do
  zakładki Kariera, razem z przyciskiem „Zapisz”.
- **Ujednolicona treść ligi** (`ui/components/LeagueViews.kt`): tabela i
  terminarz to wspólne widoki używane zarówno przez zakładki menu, jak i przez
  samodzielne ekrany (`TableScreen`, `FixturesScreen`). Tabela podświetla klub
  gracza i pokazuje nagłówek kolumn; terminarz grupuje mecze w kolejki i
  wyróżnia parę gracza, a wyniki pokazuje w układzie nazwa — wynik — nazwa.
- **Bez rozpoczętej kariery** zakładki są niedostępne: menu pokazuje kartę
  startową, skrót do samouczka i podpis wyjaśniający, że tabela i terminarz
  odblokują się po starcie kariery.
- **Kolory motywu** (`ui/theme/Theme.kt`): dodana paleta `BkhColors`
  (tło menu, karty, obramowania, kolory pomocnicze) udostępniana przez
  `LocalBkhColors`. Ekran główny nie używa już zaszytych na sztywno hexów,
  więc przełącznik motywu zmienia cały ekran.
- **Nawigacja wstecz** (`ui/State.kt`, `ui/AppNavGraph.kt`): stan trzyma stos
  ekranów (`navStack`) i nową intencję `GoBack`; systemowy przycisk „wstecz”
  cofa do poprzedniego ekranu zamiast zamykać aplikację. Przyciski
  „← Wróć” w tabeli i terminarzu korzystają z `GoBack`.
- **Przerwanie meczu** wymaga potwierdzenia — wyjście z ekranu meczu (również
  systemowym „wstecz”) pyta, czy przerwać, i informuje, że wynik nie zostanie
  zapisany.
- **Komunikaty** (`Snackbar`): `AppUiState.toast` jest wreszcie pokazywany
  (m.in. „Kariera zapisana”), a techniczne klucze są tłumaczone na teksty dla
  gracza.

### Dodane

- **Nazwy klubów widocznie związane z mapą**: wiersze `table[]` niosą teraz
  `town`, a pary `fixtures[]` — `homeTown`/`awayTown` (`core/src/facade.cpp`,
  opis w [`docs/PROTOCOL.md`](PROTOCOL.md)). UI skleja skrót z miejscowością
  („Orzeł Węgrzynowo”) nową funkcją `clubLabel` (`ui/components/ClubLabel.kt`),
  pomijając miejscowość, gdy skrót już ją niesie („Węgrzynowianka”).
  Testy JVM: `ClubLabelTest`.
- **Komplet zrzutów na stronę** (`tools/preview/render_all_screens.py`):
  jeden generator renderuje wszystkie 9 pozycji galerii (trzy zakładki menu,
  zapowiedź meczu, samouczek ×2, strzał, gol, obrona) w ciemnym motywie,
  1080×1920. Ekrany rozgrywki odwzorowują `MatchRenderer` i `Figures.kt`
  (proporcje sylwetek przeniesione 1:1), a nie są zrzutami z urządzenia.
  Arkusz poglądowy: [`docs/gallery-preview.png`](gallery-preview.png).
- **Podgląd menu** (`tools/preview/render_menu_mockup.py`): generator mockupu
  układu z `HomeScreen.kt` (trzy zakładki, oba motywy). Kluby w podglądzie to
  prawdziwi sąsiedzi Makowa Mazowieckiego z katalogu OSM — patrz
  [`docs/menu-preview.png`](menu-preview.png).
- **Zapowiedź meczu** (`MatchScreen.kt`): przed pierwszym rzutem widać nazwy
  klubów, a pod nimi **schematyczną mapkę okolicy** — punkty wszystkich klubów
  ligi, przerywaną trasę „my → rywal”, etykiety obu miejscowości, **odległość**
  w plakietce („18 km”) i laurkowy wieniec jako pieczęć w rogu mapki. Pod mapką
  kolejka, liga i znacznik „Mecz o awans” w końcówce sezonu. Mapka jest rysowana
  wektorowo (`Canvas` + rzutowanie z poprawką `cos(szerokość)`, północ u góry) —
  gra jest w 100% offline, więc żadnych kafelków mapy. Dane dostarcza rdzeń:
  `MatchSetup` niesie `distanceKm` i `leagueClubs`, a JSON `setup` —
  `playerTown`, `opponentTown`, `distanceKm` oraz `map.places[]`
  (`core/src/career.cpp`, `core/src/json_io.cpp`, opis w
  [`docs/PROTOCOL.md`](PROTOCOL.md)). Test rdzenia:
  `CareerTest.MatchSetupCarriesTownsAndDistance`. Podgląd:
  [`docs/match-preview.png`](match-preview.png)
  (`tools/preview/render_match_mockup.py` — ligę i odległości liczy wprost
  z `data/places/places_pl.csv`).

### Naprawione

- **Ligi regionalne przestały ściągać rywali z drugiego końca Polski**
  (`core/src/club.cpp`, `ClubGenerator::selectRegionalPlaces`): promień doboru
  miejscowości rośnie teraz maksymalnie 2,5× względem `TierInfo::radiusKm`
  (B klasa: do ~37 km zamiast ~87 km), a gdy w okolicy brakuje miejscowości,
  ligę dopełniają najbliższe miejscowości z **tego samego województwa** —
  dopiero ostateczność sięga po największe miasta w kraju. Nazwy miejscowości
  przy okazji nie powtarzają się już w jednej lidze (zbiór nazw był resetowany
  w każdej iteracji). Nowe testy: `ClubTest.RegionalLeagueComesFromNearbyPlaces`,
  `ClubTest.SparseRegionStaysInVoivodeshipInsteadOfWholeCountry`,
  `ClubTest.LeagueHasNoDuplicateTowns`.

- **Podsumowanie kariery po restarcie**: menu główne dociąga `cmd="career"`,
  gdy rdzeń ma zapisaną karierę, ale ekran nie zna jeszcze jej danych — wcześniej
  menu pokazywało puste imię i poziom 0.
- **Nazwa klubu w karcie kariery**: ekran czytał nieistniejące pole
  `league.clubs[...]`, więc nazwa nigdy się nie pojawiała; teraz używa
  `leagueLabel` i `homeCity` z odpowiedzi `cmd="career"`.

## [0.1.0] — 2025 (pierwsze wydanie)

### Dodane

- **Rdzeń C++23** (`core/`): fizyka piłki (opór + efekt Magnusa), kolizje
  ze słupkami/poprzeczką, bramkarz z profilami per-liga, strzelec, konkurs
  rzutów karnych 5+5 z nagłą śmiercią, deterministyczny RNG (xoshiro256**).
- **Kariera**: 8-poziomowa piramida (B klasa → Ekstraklasa), 10 klubów,
  9 kolejek, awans top-2 / spadek bottom-2, symulacja pozostałych meczów kolejki.
- **Katalog miejscowości**: ~7 800 lokalizacji OSM + generowanie wymyślonych
  nazw klubów wokół miasta gracza.
- **Mostek JNI** (`bridge/`): 10 eksportowanych symboli `Java_*`, kanał
  sterowania JSON + kanał renderujący (bufor float).
- **Aplikacja Android** (Kotlin/Jetpack Compose, Material 3): pseudo-3D
  renderer boiska na Canvas, Room do save'ów, zero uprawnień INTERNET.
- **77 testów GoogleTest** pokrywających rdzeń i balans symulacji.
- **CI** (GitHub Actions): testy hosta, build APK, publikacja release.
- **F-Droid** metadata + fastlane PL/EN + ikony launcher'a (CC0).

### Zmienione

- Brak (pierwsze wydanie).

### Naprawione

- Brak (pierwsze wydanie).
