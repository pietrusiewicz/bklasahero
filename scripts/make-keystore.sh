#!/usr/bin/env bash
# B-Klasa Hero — wygenerowanie keystore'u do podpisywania release.
# Ten skrypt jest JEDNORAZOWY — wynik trzymany w 1Password/Hasło na zewnątrz.
# Po wygenerowaniu NIE commitujemy klucza, tylko zapisujemy:
#   1. release.jks → uploadujemy jako sekret CI (base64)
#   2. hasła → sekrety CI (RELEASE_STORE_PASSWORD, RELEASE_KEY_ALIAS, RELEASE_KEY_PASSWORD)
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

ALIAS="${1:-release}"
DN="${2:-CN=B-Klasa Hero,O=B-Klasa Hero,L=Warszawa,C=PL}"
DAYS="${3:-10950}"  # ~30 lat

KEYSTORE="${KEYSTORE:-release.jks}"

if [[ -f "$KEYSTORE" ]]; then
    echo "BŁĄD: $KEYSTORE już istnieje — nie nadpisuję." >&2
    exit 1
fi

if ! command -v keytool >/dev/null 2>&1; then
    echo "BŁĄD: keytool (z JDK) nie znaleziony w PATH." >&2
    exit 1
fi

keytool -genkeypair \
    -v \
    -keystore "$KEYSTORE" \
    -alias "$ALIAS" \
    -keyalg RSA -keysize 4096 \
    -validity "$DAYS" \
    -dname "$DN" \
    -storepass "$(openssl rand -base64 32)" \
    -keypass "$(openssl rand -base64 32)"

echo ""
echo "Wygenerowano: $KEYSTORE"
echo "  alias:  $ALIAS"
echo "  SHA-1:  $(keytool -list -v -keystore "$KEYSTORE" -storepass "$(cat /tmp/storepass 2>/dev/null)" 2>/dev/null | grep 'SHA1:' | head -1 || echo 'wymaga hasła')"
echo ""
echo "Dodaj do CI jako sekrety:"
echo "  RELEASE_KEYSTORE_BASE64: \$(base64 -w0 $KEYSTORE)"
echo "  RELEASE_STORE_PASSWORD:  hasło z powyższego -storepass"
echo "  RELEASE_KEY_ALIAS:       $ALIAS"
echo "  RELEASE_KEY_PASSWORD:    hasło z powyższego -keypass"
echo ""
echo "Backup na 2 różne lokalizacje offline (Hasło 1Password, dysk zaszyfrowany, itp.)."