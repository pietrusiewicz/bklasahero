# Tablica stanu prac

| Obszar | Pliki | Stan |
|---|---|---|
| Kontrakty rdzenia | `core/include/bkh/*.h` | gotowe |
| Rdzeń C++23 (12 modułów) | `core/src/*.cpp` | gotowe |
| 77 testów GoogleTest | `core/tests/*.cpp` | gotowe |
| Mostek JNI (libbkh_bridge.so) | `bridge/src/*.cpp`, `bridge/include/bkh_bridge/*.h` | gotowe |
| Aplikacja Android (Compose) | `android/app/src/main/...` | szkielet |
| CI workflows | `.github/workflows/{host-tests,android-build,release}.yml` | gotowe |
| Skrypty budowy/podpisu | `scripts/*.sh` | gotowe |
| F-Droid + fastlane metadata | `metadata/`, `fastlane/metadata/` | gotowe |
| ADRs | `docs/ADR/0001-0006` | gotowe |
| Build / F-Droid / Privacy docs | `docs/{BUILD,FDROID,PRIVACY}.md` | gotowe |
| README + LICENSE | `README.md`, `LICENSE` | gotowe |

## Wersja

- Rdzeń: **0.1.0** (`BKH_VERSION_*` w `core/CMakeLists.txt`)
- Protokół JSON: **1** (`kProtocolVersion` w `core/include/bkh/facade.h`)
- Schema save'a: **1** (`CareerState::schemaVersion`)

## Licencje zależności

| Zależność | Wersja | Licencja | Rola |
|---|---|---|---|
| nlohmann/json | 3.12.0 | MIT | Serializacja JSON (rdzeń + save) |
| GoogleTest | 1.18.0 | BSD-3-Clause | Testy (tylko build hosta, NIE trafia do APK) |
| OpenStreetMap | — | ODbL | Dane miejscowości |

Obie zależności są pobierane z GitHub + weryfikowane SHA256
(`cmake/BkhDependencies.cmake`). Dla trybu offline jest
`scripts/fetch-deps.sh`.