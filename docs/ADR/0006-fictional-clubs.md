# ADR-0006: Fikcyjne kluby (brak prawdziwych nazw)

**Status:** przyjęte
**Data:** 2026

## Decyzja

**Wszystkie kluby w grze mają nazwy generowane algorytmicznie.**
Nie mapujemy nazw generowanych na istniejące kluby. Nie używamy
prawdziwych herbów.

Przykłady generowanych nazw:

```
GKS Wólka Witosa
LKS Mała Nieszawka
KS Dębowa Polana
```

(Schemat: `prefix + patron + town`, np. `GKS` + `Wólka` + `Witosa`.)

## Dlaczego

- **Brak ryzyka naruszenia znaków towarowych** istniejących klubów.
- **Brak ryzyka zniesławienia** — nazwy generowane są neutralne.
- **Determinizm** — z tego samego seeda (miasto + seed ligi) zawsze
  powstaje ten sam zestaw klubów.
- **Lokalność** — kluby z B klasy często nie mają rozpoznawalnych
  herbów, więc fikcyjna marka jest tu naturalna.

## Czego unikamy

- ❌ Prawdziwych nazw (np. "GKS Bełchatów" — istniejący klub).
- ❌ Prawdziwych herbów (znaki towarowe).
- ❌ Wskazywania patronów związanych z polityką lub religią
  (generujemy z neutralnej listy).
- ❌ Mapowania miasta 1:1 na jeden klub (każde miasto → kilka klubów
  z różnymi patronami).

## Implementacja

```cpp
// core/src/club.cpp
Club ClubGenerator::proposeName(Random& rng) {
    static const std::array prefixes = {"LKS", "GKS", "KS", "WKS", "MLKS"};
    static const std::array patrons   = {"Witosa", "Kościuszki", "Sienkiewicza", ...};
    ...
}
```

Lista `patrons` jest wewnętrzna i nie wycieka na zewnątrz modułu.

## Odrzucone alternatywy

- **Generowanie skrótów typu "DWW"** (pierwsze litery miasta) — brzmi
  sztucznie, psuje immersję.
- **Curated listy fikcyjnych nazw** — za dużo pracy, brak różnorodności.
- **Prawdziwe nazwy + licencja** — nawet za darmo wymaga audytu
  prawnego.