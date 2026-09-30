# ADR-0002: Flagi kompilacji rdzenia

**Status:** przyjęte
**Data:** 2026

## Decyzja

Wszystkie moduły (`bkh_core`, `bkh_bridge`) budujemy z:

```
-std=c++23 -fno-strict-aliasing -fvisibility=hidden -fvisibility-inlines-hidden
-Wall -Wextra -Wpedantic
-Wshadow -Wnon-virtual-dtor -Wold-style-cast
-Wcast-qual -Wconversion -Wsign-conversion
-Wnull-dereference -Wdouble-promotion
-Wimplicit-fallthrough -Wuseless-cast -Wformat=2
```

W CI: `-Werror`. Lokalnie: opcjonalnie.

## Dlaczego

- **C++23 bez rozszerzeń GNU** — kod musi budować się identycznie na GCC
  (host CI) i clang (NDK).
- **Wyjątki + RTTI włączone** — upraszcza most JNI i obsługę błędów.
  Hot-path nie używa `try`/`catch` — pomiar kosztów: zero alokacji.
- **Bez `-ffast-math`** — determinizm symulacji (te same seed → identyczne
  wyniki na każdym urządzeniu) jest cechą produktu.
- **`-fvisibility=hidden`** — rdzeń eksportuje tylko fasadę, nie typy
  wewnętrzne. Redukuje binary size i powierzchnię ABI.
- **Ścisłe `-Wconversion -Wsign-conversion`** — żeby wymusić jawne rzuty
  na granicach modułów. Kiedyś jeden cichy konwersja wbiła nam NaN do
  symulacji — więcej nie chcemy.
- **`-Wuseless-cast`** — żeby wykryć, że typy się rozeszły (znak, że
  granica modułu się zmieniła).

## Konsekwencje

- CI w trybie Werror wymusza czysty kod.
- Deweloper lokalnie może wyłączyć przez `-DBKH_WERROR=OFF` w cmake.
- Nowy type z `-Wconversion` → fail. Akceptujemy to jako hamulec dla
  pomyłek typów.

## Odrzucone alternatywy

- **Bez `-Werror`** — ryzyko regresji ostrzeżeń.
- **Tylko `-Wall -Wextra`** — zbyt łagodne, brak wykrywania konwersji.
- **`-Werror -Weverything`** (clang) — generuje szum od nagłówków STL.