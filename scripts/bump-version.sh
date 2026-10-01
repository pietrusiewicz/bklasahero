#!/usr/bin/env bash
# B-Klasa Hero — bump wersji rdzenia i aplikacji (versionName + versionCode).
#
# Użycie:
#   scripts/bump-version.sh 0.2.0       # versionCode = poprzedni + 1
#   scripts/bump-version.sh 0.2.0 2     # jawnie podany versionCode
#
# Aktualizuje:
#   1. CMakeLists.txt (korzeń) — project(bkh VERSION x.y.z)
#   2. android/app/build.gradle.kts — domyślne versionName / versionCode
#   3. metadata/pl.bklasahero.yml — CurrentVersion / CurrentVersionCode
#   4. docs/STATUS.md — wersja rdzenia
#
# NIE dodaje wpisu w Builds: (potrzebny pełny hash taga — zrób to po tagnięciu).
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

NEW="${1:?Użycie: $0 <MAJOR.MINOR.PATCH> [versionCode]}"
cd "$(dirname "$0")/.."

if ! [[ "$NEW" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "BŁĄD: oczekuję MAJOR.MINOR.PATCH, np. 0.2.0" >&2
    exit 2
fi

META=metadata/pl.bklasahero.yml
OLDCODE=$(sed -nE 's|^CurrentVersionCode:[[:space:]]*([0-9]+).*|\1|p' "$META" 2>/dev/null | head -1)
NEWCODE="${2:-$(( ${OLDCODE:-0} + 1 ))}"

if ! [[ "$NEWCODE" =~ ^[0-9]+$ ]]; then
    echo "BŁĄD: versionCode musi być liczbą całkowitą, dostałem '$2'" >&2
    exit 2
fi
if [[ -n "$OLDCODE" ]] && (( NEWCODE <= OLDCODE )); then
    echo "BŁĄD: versionCode musi rosnąć monotonicznie ($OLDCODE → $NEWCODE)" >&2
    exit 2
fi

# 1. CMakeLists.txt (korzeń) — VERSION x.y.z w project(bkh ...)
sed -i -E "s|VERSION [0-9]+\.[0-9]+\.[0-9]+|VERSION $NEW|" CMakeLists.txt

# 2. android/app/build.gradle.kts — domyślne versionName / versionCode
sed -i -E \
    -e "s|(\?): \"[0-9]+\.[0-9]+\.[0-9]+\"|\1: \"$NEW\"|" \
    -e "s|(\?): [0-9]+$|\1: $NEWCODE|" \
    android/app/build.gradle.kts

# 3. metadata/pl.bklasahero.yml — CurrentVersion / CurrentVersionCode
if [[ -f "$META" ]]; then
    sed -i -E \
        -e "s|^CurrentVersion: .*|CurrentVersion: $NEW|" \
        -e "s|^CurrentVersionCode: .*|CurrentVersionCode: $NEWCODE|" \
        "$META"
fi

# 4. docs/STATUS.md — wersja rdzenia
if [[ -f docs/STATUS.md ]]; then
    sed -i -E "s|- Rdzeń: \*\*[0-9]+\.[0-9]+\.[0-9]+\*\*|- Rdzeń: **$NEW**|" docs/STATUS.md || true
fi

echo ">>> zaktualizowano wersję do $NEW (versionCode $OLDCODE → $NEWCODE)"
echo "Pamiętaj o:"
echo "  - wpisie zmian w docs/CHANGELOG.md"
echo "  - tagnięciu i dodaniu wpisu w Builds: w $META (commit = pełny hash taga):"
echo "      git tag v$NEW && git rev-parse v$NEW"
