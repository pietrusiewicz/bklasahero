# B-Klasa Hero

> Penalty shootouts + career in the Polish football pyramid. 100% offline,
> 100% open-source, zero telemetry.

_"B-Klasa Hero"_ (znane też jako _"Sunday League Hero"_) to mobilna gra
piłkarska w całości offline. Zaczynasz od B klasy, strzelasz karne,
zbierasz punkty i — jeśli dopisze ci szczęście w sędziowskich
doliczonych — awansujesz do Ekstraklasy.

W grze:

- **5 rzutów karnych** z fizyką (grawitacja, opór powietrza,
  efekt Magnusa, słupki i poprzeczka).
- **Kariera przez 8 poziomów** polskiej piramidy piłkarskiej.
- **Proceduralne nazwy klubów** (żadnych prawdziwych herbów ani
  ryzyka naruszenia znaków towarowych).
- **~7 800 polskich miejscowości** z OpenStreetMap (ODbL).
- **Zerowe uprawnienia**. Zero sieci. Zero reklam.

## Zrzuty ekranu

_(do dodania przed pierwszym release)_

## Jak zbudować

```bash
# Testy rdzenia C++ (szybko, bez NDK/JDK):
scripts/build-core-tests.sh

# Całość + APK (potrzebujesz JDK 17, Android SDK 34, NDK 27):
docs/BUILD.md
```

## Jak grać

TBD.

## Jak dodać tłumaczenie

Pliki w `android/app/src/main/res/values-<locale>/strings.xml`.
Dodaj nowy `<locale>` i tłumacz — tyle. Zgłoś PR.

## Architektura

Szczegóły w [`docs/ADR/`](docs/ADR):

1. [Granice rdzenia](docs/ADR/0001-core-boundaries.md)
2. [Flagi kompilacji](docs/ADR/0002-build-flags.md)
3. [Format save'a](docs/ADR/0003-save-format.md)
4. [Źródła danych](docs/ADR/0004-data-sources.md)
5. [Wersjonowanie protokołu](docs/ADR/0005-protocol-versioning.md)
6. [Fikcyjne kluby](docs/ADR/0006-fictional-clubs.md)

## Licencja

- **Kod**: GPL-3.0-or-later.
- **Grafiki**: CC0.
- **Dane miejscowości**: OpenStreetMap, ODbL — atrybucja w
  [`data/places/ATTRIBUTION.md`](data/places/ATTRIBUTION.md).

## Kontakt

- Issues: zakładka _Issues_ w repo.
- Prywatność: patrz [`docs/PRIVACY.md`](docs/PRIVACY.md).
- Bezpieczeństwo: e-mail na stronie About w grze (TBD przed 1.0).

## Status

Patrz [`docs/STATUS.md`](docs/STATUS.md).

---

"B-Klasa Hero" nie jest powiązane z żadnym klubem piłkarskim, ligą
ani federacją. Wszystkie nazwy klubów są generowane algorytmicznie,
a wyniki symulowane.