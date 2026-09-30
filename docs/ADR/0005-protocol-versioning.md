# ADR-0005: Wersjonowanie protokołu (JSON + save)

**Status:** przyjęte
**Data:** 2026

## Decyzja

Trzy niezależne wersje:

| Co | Numer | Kiedy bump |
|---|---|---|
| Protokół poleceń JSON (`kProtocolVersion`) | 1 | Zmiana nazw kluczy, dodanie nowego pola wymaganego |
| Schema save'a (`CareerState::schemaVersion`) | 1 | Zmiana struktury JSON save'a |
| Wersja rdzenia (`BKH_VERSION_*`) | 0.1.0 | Każdy release |

Przy starcie aplikacji:

```
NativeBridge.protocolVersion →  wyświetl w About
NativeBridge.saveSchemaVersion → sprawdź vs zapisany w save
NativeBridge.coreVersion → wyświetl w About
```

## Dlaczego trzy osobne

- **Protokół poleceń** zmienia się rzadko (dodanie nowego `cmd` to
  rozszerzenie, nie zmiana).
- **Schema save'a** zmienia się przy każdej migracji danych.
- **Wersja rdzenia** zmienia się przy każdym wydaniu (marketing).

## Reguły kompatybilności

| Zmiana | Bump |
|---|---|
| Dodanie nowego `cmd` w protokole | kProtocolVersion (minor) |
| Zmiana nazwy klucza w odpowiedzi | kProtocolVersion (major) |
| Nowe pole opcjonalne w save | saveSchemaVersion (minor) |
| Zmiana typu istniejącego pola w save | saveSchemaVersion (major) |
| Poprawka balansu gry | BKH_VERSION (patch) |
| Nowa warstwa funkcjonalna | BKH_VERSION (minor) |

## Konsekwencje

- Bridge JNI ma 3 osobne funkcje: `nativeProtocolVersion`,
  `nativeSaveSchemaVersion`, `nativeCoreVersion`.
- Aplikacja przy starcie porównuje `saveSchemaVersion` z zapisanym
  w save'ie. Niezgodność = migracja lub odrzucenie.
- Zmiany są opisane w `CHANGELOG.md` per wpływ na każdą z wersji.

## Odrzucone alternatywy

- **Jeden numer** — nieodróżnienie breaking od niebreaking.
- **Semver dla protokołu** — protokół nie ma minor/patch, tylko
  compatible/incompatible.