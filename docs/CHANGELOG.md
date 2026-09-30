# Dziennik zmian

Wszystkie istotne zmiany projektu są tu rejestrowane. Format inspirowany
[Keep a Changelog](https://keepachangelog.com/pl/1.1.0/), wersjonowanie zgodne
z [SemVer](https://semver.org/lang/pl/).

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
