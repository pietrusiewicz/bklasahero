#!/usr/bin/env bash
# B-Klasa Hero — lokalne podpisanie APK release (np. dla testów na urządzeniu).
# NIE używaj tego w release workflow — tam klucze żyją w sekretach CI.
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

APK="${1:?Użycie: $0 <ścieżka-do-APK> <ścieżka-do-key.jks> [alias]}"
KEYSTORE="${2:?brak klucza}"
ALIAS="${3:-release}"

if [[ ! -f "$APK" ]]; then echo "BŁĄD: $APK nie istnieje" >&2; exit 1; fi
if [[ ! -f "$KEYSTORE" ]]; then echo "BŁĄD: $KEYSTORE nie istnieje" >&2; exit 1; fi

read -rsp "Hasło keystore: " STORE_PASS; echo
read -rsp "Hasło klucza ($ALIAS): " KEY_PASS; echo

# Znajdź zipalign i apksigner z Android SDK
SDK="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"
if [[ -z "$SDK" ]]; then
    echo "BŁĄD: ustaw ANDROID_HOME lub ANDROID_SDK_ROOT" >&2
    exit 1
fi
ZIPALIGN=$(find "$SDK/build-tools" -name zipalign | sort -V | tail -1)
APKSIGNER=$(find "$SDK/build-tools" -name apksigner | sort -V | tail -1)
[[ -x "$ZIPALIGN" ]] || { echo "BŁĄD: zipalign nie znaleziony"; exit 1; }
[[ -x "$APKSIGNER" ]] || { echo "BŁĄD: apksigner nie znaleziony"; exit 1; }

ALIGNED="${APK%.apk}-aligned.apk"
"$ZIPALIGN" -p -f 4 "$APK" "$ALIGNED"
"$APKSIGNER" sign --ks "$KEYSTORE" --ks-key-alias "$ALIAS" \
    --ks-pass "pass:$STORE_PASS" --key-pass "pass:$KEY_PASS" \
    --out "$APK" "$ALIGNED"
rm "$ALIGNED"
"$APKSIGNER" verify --print-certs "$APK"
echo ">>> podpisano $APK"