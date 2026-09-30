#!/usr/bin/env bash
# B-Klasa Hero — szybka kompilacja i testy rdzenia C++ na hoście.
# Nie wymaga NDK ani Gradle — potrzebny tylko cmake/g++.
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

cd "$(dirname "$0")/.."

PRESET="${PRESET:-host-debug}"

echo ">>> konfiguracja ($PRESET)"
cmake --preset "$PRESET"

echo ">>> budowa"
cmake --build --preset "$PRESET" -j"$(nproc 2>/dev/null || echo 2)"

echo ">>> testy"
ctest --preset "$PRESET" --output-on-failure

echo ">>> GOTOWE — wszystko zielone"