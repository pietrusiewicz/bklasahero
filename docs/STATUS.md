# Tablica stanu prac

| Obszar | Pliki | Stan | Właściciel |
|---|---|---|---|
| Kontrakty rdzenia (nagłówki) | `core/include/bkh/*.h` | gotowe | agent główny |
| Protokół JNI | `docs/PROTOCOL.md` | gotowe | agent główny |
| System budowy CMake | `CMakeLists.txt`, `cmake/`, `core/CMakeLists.txt`, `bridge/CMakeLists.txt` | gotowe | agent główny |
| Dane geo (OSM → CSV) | `tools/geo/`, `data/places/` | w toku | subagent A |
| Katalog miejscowości + kluby | `core/src/place.cpp`, `core/src/club.cpp` | stub → w toku | subagent D |
| Liga + kariera + JSON | `core/src/league.cpp`, `career.cpp`, `json_io.cpp` | stub → w toku | subagent G |
| Fizyka, bramkarz, strzelec, konkurs, fasada | `core/src/{random,physics,keeper,shooter,shot,shootout,facade}.cpp` | stub → w toku | agent główny |
| Mostek JNI | `bridge/src/*.cpp` | oczekuje | agent główny |
| Aplikacja Android (Compose) | `android/` | oczekuje | subagent E |
| CI + skrypty | `.github/workflows/`, `scripts/`, `docs/BUILD.md` | oczekuje | subagent F1 |
| F-Droid + fastlane + dokumentacja | `metadata/`, `fastlane/`, `docs/`, `README.md` | oczekuje | subagent F2 |

**Legenda:** stub = plik istnieje, żeby CMake się konfigurował; treść docelowa nadpisze stub.
