# ADR-0003: Format save'a (JSON + Room)

**Status:** przyjęte
**Data:** 2026

## Decyzja

Save gry to **jeden blob JSON** w bazie Room (jeden rekord, jeden slot).

```sql
CREATE TABLE career_save (
    slotId INTEGER PRIMARY KEY,        -- zawsze 0 w wersji 0.x
    json TEXT NOT NULL,
    saved_at_epoch_ms INTEGER NOT NULL
);
```

Cała logika wersjonowania jest w **strukturze JSON** (pole
`schemaVersion`), nie w kolumnach SQL. Migracja = nowy kod, który czyta
JSON `schemaVersion=1`, transformuje i zapisuje `schemaVersion=2`.

## Dlaczego

- **JSON jest czytelny** — gracz może otworzyć save'a w edytorze i
  sprawdzić, czy coś się rozjechało.
- **JSON jest niezależny od Room** — w przyszłości możemy przenieść save
  do pliku (Settings) bez zmiany formatu.
- **Zero SQL-migracji** — dodanie nowej tabeli history sezonów nie
  wymaga ALTER TABLE.
- **Atomowość** — zapis to jeden `UPDATE`, transakcja Room.

## Konsekwencje

- Przy starcie aplikacji:
  1. Wczytaj JSON z DB.
  2. Sprawdź `schemaVersion`.
  3. Jeśli niższy niż aktualny — wykonaj migrację (sekwencja transformacji).
  4. Wgraj do rdzenia przez `NativeBridge.loadCareer`.
- Po każdym meczu:
  1. Pobierz JSON z rdzenia (`NativeBridge.saveCareer`).
  2. Zapisz do bazy.
- Uszkodzony JSON = crash → w 0.x fallback: usuń save i zacznij od nowa.
  W wersji 1.0+ — kopia zapasowa `career_save.json.bak`.

## Odrzucone alternatywy

- **Wiele tabel** (CareerTable, SeasonTable, MatchTable...) — zbyt dużo
  kodu migracji za nic.
- **Protobuf / FlatBuffers** — szybsze, ale nieczytelne i utrudnia debug.
- **Wiele slotów** — dodaje złożoność UI, a 1 slot to ~1 KB.