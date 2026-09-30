# B-Klasa Hero — publikacja na F-Droid

F-Droid buduje każdą paczkę z metadanych w `metadata/<applicationId>.yml`
oraz kodu źródłowego z Git taga. Poniżej pełna ścieżka od taga do paczki w
repozytorium.

## Jednorazowa konfiguracja (po stronie opiekuna)

1. **Konto na https://f-droid.org** — wniosek przez https://f-droid.org/contact.
2. **Wpis `fdroiddata` w fork'u** — plik `metadata/pl.bklasahero.yml`.
3. **Włączenie robota builda** — robot włącza aplikację po pierwszym PR
   akceptowanym w `fdroiddata`.

## Co trzeba wysłać do repo `fdroiddata`

Plik `metadata/pl.bklasahero.yml` w tym repo to **szablon**.
Oficjalną kopię trzeba wstawić do `fdroiddata` PR-em, razem z:

- `Builds:` — zwykle 1 wpis per ABI (tu mamy `arm64-v8a` + `armeabi-v7a`).
- `Allowed-non-free-uses:` — nie mamy żadnych.
- `Auto-Updater:` — `Maintainer: ...`.

## Procedura wydania

```bash
# 1. Bump wersji.
scripts/bump-version.sh 0.2.0

# 2. Commit + tag.
git add -A
git commit -m "Wydanie 0.2.0"
git tag v0.2.0
git push origin main --tags

# 3. CI buduje release (workflow: .github/workflows/release.yml).
#    → APK trafia do GitHub Releases.

# 4. Fork fdroiddata → kopia metadata/pl.bklasahero.yml →
#    bump Version i CurrentVersion → PR.

# 5. Po akceptacji robota builda, F-Droid publikuje wersję w 24-72 h.
```

## Co obserwuje robot F-Droid

- Czy `versionCode` rośnie monotonicznie — nie resetuj go!
- Czy APK się buduje bez zastrzeżonych bibliotek (`non-free`).
- Czy podpis jest stały (ten sam klucz co poprzednio).
- Czy `Builds:` matchuje tagi.

## Konwencja numerów wersji

`MAJOR.MINOR.PATCH` semver, ale bez `0.x` nie oznacza "niestabilne" — to aplikacja
offline, więc nawet `1.0` nie zmienia kontraktu. Trzymamy się:

- **MAJOR** — bump przy niezgodnej wersji save'a (konieczna migracja).
- **MINOR** — bump przy nowej warstwie funkcjonalnej (np. nowa liga).
- **PATCH** — bump przy poprawkach balansu / tłumaczeń / drobnych bugach.

## Diagnostyka problemów z F-Droid

Najczęstsze błędy:

| Symptom | Rozwiązanie |
|---|---|
| `Can't build app pl.bklasahero` | Sprawdź log robota, zwykle brakuje zależności w `BkhDependencies.cmake` |
| `No update from 0.1.0 (1) to 0.1.0 (1)` | Nie bumpnąłeś `versionCode` |
| `Bad signature` | NIE zmieniaj klucza po pierwszej wersji w repo. Jeśli musisz, koordynuj z F-Droid na forum |
| `Missing binary blob` | Każdy asset musi być pobrany w buildzie (tu: miejsca OSM) |