#!/usr/bin/env bash
# B-Klasa Hero — pobranie zależności rdzenia (GoogleTest, nlohmann/json) do katalogu offline.
# Użycie:
#   scripts/fetch-deps.sh                       # do ./build/_deps (FetchContent)
#   scripts/fetch-deps.sh --offline /opt/deps   # do katalogu z prefabem
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

MODE="online"
DEST="${HOME}/.cache/bkh-deps"
while [[ $# -gt 0 ]]; do
    case "$1" in
        --offline) MODE="offline"; DEST="${2:-/opt/deps}"; shift 2 ;;
        --dest) DEST="$2"; shift 2 ;;
        *) echo "unknown flag $1" >&2; exit 2 ;;
    esac
done

mkdir -p "$DEST"
cd "$DEST"

# --- GoogleTest 1.18.0 ----------------------------------------------
GTEST_VERSION="1.18.0"
GTEST_SHA256="6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5"
GTEST_DIR="googletest-${GTEST_VERSION}"
if [[ ! -d "$GTEST_DIR" ]]; then
    echo ">>> pobieram GoogleTest ${GTEST_VERSION}"
    TARBALL="v${GTEST_VERSION}.tar.gz"
    curl -fsSL -o "$TARBALL" \
        "https://github.com/google/googletest/archive/refs/tags/${TARBALL}"
    echo "${GTEST_SHA256}  ${TARBALL}" | sha256sum -c -
    tar -xzf "$TARBALL"
    rm "$TARBALL"
fi

# --- nlohmann/json 3.12.0 ----------------------------------------------
JSON_VERSION="3.12.0"
JSON_SHA256="4b92eb0c06d10683f7447ce9406cb97cd4b453be18d7279320f7b2f025c10187"
JSON_DIR="json-${JSON_VERSION}"
if [[ ! -d "$JSON_DIR" ]]; then
    echo ">>> pobieram nlohmann/json ${JSON_VERSION}"
    TARBALL="v${JSON_VERSION}.tar.gz"
    curl -fsSL -o "$TARBALL" \
        "https://github.com/nlohmann/json/archive/refs/tags/${TARBALL}"
    echo "${JSON_SHA256}  ${TARBALL}" | sha256sum -c -
    tar -xzf "$TARBALL"
    rm "$TARBALL"
fi

echo ">>> GOTOWE: $DEST"
ls -la "$DEST"