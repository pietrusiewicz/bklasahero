#!/usr/bin/env bash
# B-Klasa Hero — bump wersji rdzenia (BKH_VERSION_*) i aplikacji (versionCode/Name).
# Użycie: scripts/bump-version.sh 0.2.0
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

NEW="${1:?Użycie: $0 <MAJOR.MINOR.PATCH>}"
cd "$(dirname "$0")/.."

if ! [[ "$NEW" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "BŁĄD: oczekuję MAJOR.MINOR.PATCH, np. 0.2.0" >&2
    exit 2
fi

MAJOR=$(echo "$NEW" | cut -d. -f1)
MINOR=$(echo "$NEW" | cut -d. -f2)
PATCH=$(echo "$NEW" | cut -d. -f3)

# 1. core/CMakeLists.txt
sed -i \
    -e "s|set(BKH_VERSION_MAJOR [0-9]\\+)|set(BKH_VERSION_MAJOR $MAJOR)|" \
    -e "s|set(BKH_VERSION_MINOR [0-9]\\+)|set(BKH_VERSION_MINOR $MINOR)|" \
    -e "s|set(BKH_VERSION_PATCH [0-9]\\+)|set(BKH_VERSION_PATCH $PATCH)|" \
    core/CMakeLists.txt

# 2. android/app/build.gradle.kts (defaultConfig)
sed -i \
    -e "s|versionName = \".*\"$|versionName = \"$NEW\"|" \
    android/app/build.gradle.kts

# 3. metadata/pl.bklasahero.yml (F-Droid) — bump wersja
if [[ -f metadata/pl.bklasahero.yml ]]; then
    sed -i -E "s|^Version: .*|Version: $NEW|" metadata/pl.bklasahero.yml
fi

# 4. docs/STATUS.md — aktualizacja tabelki
if [[ -f docs/STATUS.md ]]; then
    sed -i -E "s|\| core |[0-9]+\\.[0-9]+\\.[0-9]+ \\|| core |$NEW \\||" docs/STATUS.md || true
fi

echo ">>> zaktualizowano wersję do $NEW"
echo "Pamiętaj o:"
echo "  - zwiększeniu versionCode (przyrostowo) w android/app/build.gradle.kts"
echo "  - wpisaniu zmian w CHANGELOG"
echo "  - tagnięciu: git tag v$NEW"