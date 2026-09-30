# B-Klasa Hero — jak zbudować

Trzy niezależne cele builda:

| Cel | Polecenie | Artefakt | Wymaga |
|---|---|---|---|
| Testy rdzenia C++ (host) | `cmake --preset host-debug && ctest --preset host-debug` | `build/host-debug/core/tests/bkh_core_tests` | cmake, g++/clang |
| APK release | `cd android && ./gradlew :app:assembleRelease` | `android/app/build/outputs/apk/release/app-release.apk` | JDK 17, Android SDK, NDK |
| Podpisany APK | `scripts/sign-apk.sh ...` | jak wyżej | powyższe + keystore |

## Wymagania minimalne

- **CMake ≥ 3.22**
- **Kompilator C++23**: GCC 13+ albo Clang 16+
- **JDK 17** (Android)
- **Android SDK 35** + **NDK 27.2.12479018** (Android)
- **Android Build Tools 35.0.0** (apksigner, zipalign)
- Dysk: 4 GB na cache Gradle + FetchContent.

## Szybki start (host)

```bash
# Testy rdzenia (bez Javy, bez NDK).
scripts/build-core-tests.sh
```

## Budowanie APK (Linux)

```bash
# 1. Jednorazowo: klucz do podpisu release.
scripts/make-keystore.sh release

# 2. Wskaż SDK (NDK wersję pinuje build.gradle.kts: ndkVersion).
export ANDROID_HOME=$HOME/Android/Sdk

# 3. Zbuduj APK release (unsigned — F-Droid podpisuje własnym kluczem).
cd android
./gradlew :app:assembleRelease
```

## Tryb offline (serwer F-Droid)

```bash
# Pobierz zależności do /opt/deps.
scripts/fetch-deps.sh --offline /opt/deps

# Konfiguruj build z lokalnym cache.
cmake --preset host-debug -DBKH_DEPS_DIR=/opt/deps
cd android
# build.gradle.kts mapuje bkh.deps.dir → -DBKH_DEPS_DIR dla CMake.
./gradlew :app:assembleRelease -Pbkh.deps.dir=/opt/deps
```

## Diagnostyka

```bash
# Zależności rdzenia nie chcą się pobrać?
cmake --preset host-debug --trace-source=BkhDependencies

# NDK zgłasza dziwne błędy?
cd android && ./gradlew :app:assembleDebug --info --stacktrace

# Brak ikony w mipmap?
# Wygeneruj je ponownie: python3 tools/icons/generate_icons.py
```

## Wersja rdzenia vs. aplikacji

Rdzeń (`BKH_VERSION_*`) i aplikacja (`versionName`) rosną razem.
Skrypt `scripts/bump-version.sh 0.2.0` aktualizuje oba + metadane F-Droid.

## Struktura katalogów po buildzie

```
build/host-debug/
  core/
    libbkh_core.a                 # biblioteka statyczna rdzenia
    tests/bkh_core_tests          # binarka z testami
  bridge/
    libbkh_bridge.so              # (gdy budujemy bridge poza Androidem)
android/app/build/outputs/apk/
  debug/app-debug.apk
  release/app-release.apk
```