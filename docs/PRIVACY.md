# B-Klasa Hero — Polityka Prywatności

## Wersja skrócona

Aplikacja **nie zbiera żadnych danych osobowych**. Wszystko działa offline.
Nie ma analityki, reklam, mikropłatności ani konta użytkownika.

## Co aplikacja przechowuje lokalnie

Tylko:

- **Twój save gry** (kariera + postęp) — w prywatnej bazie Room
  `bkh_career.db` na urządzeniu. Możesz go skasować przez
  *Ustawienia → Aplikacje → B-Klasa Hero → Wyczyść dane*.
- **Ustawienia języka** — wybór z `pl`/`en`.
- **Katalog miejscowości** — skopiowany z assets przy pierwszym uruchomieniu,
  zasób statyczny pobrany z OpenStreetMap (patrz
  [`data/places/ATTRIBUTION.md`](../data/places/ATTRIBUTION.md)).

## Czego aplikacja NIE robi

- ❌ Nie wysyła niczego do internetu.
- ❌ Nie loguje do żadnego zdalnego serwisu.
- ❌ Nie używa Google Play Services.
- ❌ Nie czyta IMEI, numeru telefonu, listy kontaktów, lokalizacji GPS.
- ❌ Nie używa mikrofonu ani kamery.

## Uprawnienia systemowe

Aplikacja prosi o **zero uprawnień** (`AndroidManifest.xml` nie zawiera
żadnych `<uses-permission>`).

Jedyny deklarowany `uses-feature` to `android.hardware.touchscreen`
z `required="false"` — żeby F-Droid / Google Play poprawnie zakwalifikował
aplikację dla tabletów bez dotyku.

## Sieć

Aplikacja **nie otwiera połączeń sieciowych** poza pierwszym użyciem
(cache `gradle` przy budowie na CI — nie dotyczy użytkownika końcowego).
Pakiet nie deklaruje `android.permission.INTERNET`.

Jeśli w przyszłości dodamy tryb multiplayer, ta polityka zostanie zaktualizowana
**przed** wydaniem i poprosimy o wyraźną zgodę na połączenie.

## Crash reporting

Aplikacja nie ma zdalnego crash-reportingu. Błędy są widoczne tylko w
logcat, jeśli użytkownik sam je włączy.

## Źródła danych

- **Miejscowości**: OpenStreetMap, ODbL — pełna atrybucja w
  [`data/places/ATTRIBUTION.md`](../data/places/ATTRIBUTION.md).
- **Nazwy klubów**: generowane algorytmicznie, brak prawdziwych nazw.
- **Wyniki sportowe**: symulowane algorytmicznie, brak realnych wyników.

## Kontakt z opiekunem

Zgłoszenia prywatności: sprawdź `README.md` → sekcja "Kontakt".

## Zmiany

Wszelkie zmiany tej polityki będą opisane w `CHANGELOG.md` oraz
wymagały nowego wydania.

---
*Data wejścia w życie: 2026*