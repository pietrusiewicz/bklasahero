# ADR-0001: Granice rdzenia (`bkh_core`)

**Status:** przyjęte
**Data:** 2026

## Kontekst

Projekt ma trzy warstwy: rdzeń C++23, mostek JNI, aplikacja Android. Bez
jasnych granic które warstwy mogą co wiedzieć, cała aplikacja zamieni się
w spaghetti.

## Decyzja

**`bkh_core` NIE MOŻE zależeć od żadnej biblioteki poza STL (C++23) oraz
`nlohmann/json`.**

To oznacza:

- ❌ JNI (`jni.h`) — nigdy w `core/include/bkh/*.h`.
- ❌ Android NDK — nigdy w `core/`.
- ❌ `std::filesystem` (nie jest dostępne w NDK r27) — używamy stringów.
- ❌ `std::thread` / atomiki (NIE używamy, by zachować determinizm).
- ❌ Implementacje własnych kontenerów na zewnątrz STL.

Warstwy:

```
Android (Kotlin) → NativeBridge.kt → JNI → bkh_bridge → bkh_core
                                       ↑
                              izolacja typów platformowych
```

## Konsekwencje

- Każdy moduł rdzenia można zbudować na hoście (Linux/Mac/Windows) bez NDK.
- Testy jednostkowe (GoogleTest) żyją w rdzeniu.
- Mostek JNI jest trywialny — tłumaczy tylko String ↔ std::string,
  jfloatArray ↔ float*.
- Za każdym razem gdy rdzeń potrzebuje pliku, **otrzymuje bajty z mostka**
  (patrz `Facade::loadPlaces`). Rdzeń nigdy nie otwiera plików.

## Odrzucone alternatywy

- **Wszystko w Kotlinie** — za wolno na 60-fps fizykę piłki.
- **Rust jako rdzeń** — wprowadza dodatkowy toolchain i problemy z NDK.
- **Java/ART dla logiki gry** — wolniejsze, mniej deterministyczne, GC pauzy.