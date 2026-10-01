# B-Klasa Hero — publikacja na F-Droid

F-Droid buduje każdą paczkę z kodu źródłowego (git tag/commit) + metadanych
`metadata/pl.bklasahero.yml`. Poniżej pełna ścieżka.

## Wymagania wstępne

1. Publiczne repo: https://github.com/pietrusiewicz/bklasahero (klon `https://`, bez auth).
2. Tag dla każdego wydania: `v0.1.0`, `v0.2.0`, …
3. Licencja FOSS (GPL-3.0-or-later) + brak zależności non-free — spełnione.

## Jak działa build na serwerze F-Droid

- `Repo:` → klon repo; `commit:` (pełny hash taga) → checkout; `subdir: android` → `gradle assembleRelease`.
- Release jest **niepodpisany** — F-Droid podpisuje własnym kluczem.
- Zależność rdzenia `nlohmann/json` (MIT) pobiera CMake `FetchContent` z weryfikacją SHA256 (build serwer ma sieć, jak przy zależnościach Maven).
- `versionCode` musi rosnąć monotonicznie — nie resetuj go.

## Procedura wydania

```bash
# 1. Bump wersji (versionName + versionCode).
scripts/bump-version.sh 0.2.0 2

# 2. Commit + tag.
git add -A
git commit -m "Wydanie 0.2.0"
git tag v0.2.0
git push origin main --tags

# 3. Pełny hash taga wpisz do metadata jako commit:.
git rev-parse v0.2.0   # → <hash>

# 4. W metadata/pl.bklasahero.yml dopisz wpis w Builds: z commit: <hash>.
#    (CurrentVersion / CurrentVersionCode już zaktualizował bump-version.sh)

# 5. Fork fdroiddata → skopiuj metadata/pl.bklasahero.yml → MR
#    (label "New App" przy pierwszym zgłoszeniu, potem "Update").
```

## Co obserwuje robot F-Droid

- Czy `versionCode` rośnie monotonicznie.
- Czy APK buduje się bez bibliotek non-free.
- Czy `commit:` to pełny, niezmienny hash (nie tag / nie nazwa brancha).
- Czy `gradle:` jest ustawione — bez tego build uznany za „manualny" i nie powstanie.

## Testowanie metadata przed MR (opcjonalne)

Zainstaluj `fdroidserver` i uruchom w kontenerze buildserver:

```bash
fdroid readmeta
fdroid lint pl.bklasahero
fdroid build pl.bklasahero
```

## Diagnostyka

| Symptom | Rozwiązanie |
|---|---|
| `Can't build app pl.bklasahero` | Sprawdź log robota; zwykle zły `output:` albo brak zależności |
| `No update from X to X` | Nie bumpnąłeś `versionCode` |
| `Bad signature` | Nie zmieniaj klucza F-Droid po pierwszej wersji |
| FetchContent nie pobiera | Sprawdź SHA256 w `cmake/BkhDependencies.cmake` / sieć buildserwera |
