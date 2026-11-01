#!/usr/bin/env python3
"""Wyrównuje biblioteki natywne w APK do 4096 B (zipalign bez build-tools).

Na maszynie ARM nie ma działającego `zipalign` z build-tools (x86), a przy
`extractNativeLibs=false` system ładuje `.so` wprost z APK — bez wyrównania
strony ładowanie może się nie udać. Skrypt przepisuje archiwum: wpisy `.so`
zapisuje bez kompresji, dopełniając pole `extra` tak, by dane zaczynały się
na wielokrotności 4096. Inne wpisy przepisuje bez zmian.

Użycie:
    python3 tools/android/align_apk.py wejscie.apk wyjscie.apk [--align 4096]

SPDX-License-Identifier: GPL-3.0-or-later
"""
from __future__ import annotations

import argparse
import struct
import zipfile

ALIGN_STORED = (".so",)  # 4096 B dotyczy bibliotek natywnych (extractNativeLibs=false)


def align(src: str, dst: str, align: int = 4096) -> int:
    """Przepisuje APK, wyrównując wpisy. Zwraca liczbę wyrównanych wpisów."""
    zmienione = 0
    with zipfile.ZipFile(src) as zin, zipfile.ZipFile(dst, "w") as zout:
        for info in zin.infolist():
            data = zin.read(info.filename)
            bez_kompresji = info.filename.endswith(ALIGN_STORED) or info.compress_type == zipfile.ZIP_STORED
            nowy = zipfile.ZipInfo(info.filename, date_time=info.date_time)
            nowy.compress_type = zipfile.ZIP_STORED if bez_kompresji else info.compress_type
            nowy.external_attr = info.external_attr
            nowy.internal_attr = info.internal_attr
            nowy.create_system = info.create_system
            if bez_kompresji:
                # offset danych = pozycja nagłówka + 30 + nazwa + extra
                for _ in range(2):  # druga iteracja po ustabilizowaniu rozmiaru extra
                    naglowek = zout.fp.tell() if hasattr(zout, "fp") else zout.fp.tell()
                    nazwa = len(info.filename.encode("utf-8"))
                    baza = naglowek + 30 + nazwa
                    pad = (align - (baza % align)) % align
                    # pole extra: nagłówek 4 B + dane; używamy identyfikatora 0xD935
                    if pad < 4:
                        pad += align
                    rozmiar_danych = pad - 4
                    extra = struct.pack("<HH", 0xD935, rozmiar_danych) + b"\x00" * rozmiar_danych
                nowy.extra = extra
                zmienione += 1
            zout.writestr(nowy, data)
    return zmienione


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("dst")
    ap.add_argument("--align", type=int, default=4096)
    args = ap.parse_args()
    n = align(args.src, args.dst, args.align)
    print(">>> %s -> %s (wyrównanych wpisów: %d)" % (args.src, args.dst, n))


if __name__ == "__main__":
    main()
