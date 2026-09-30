#!/usr/bin/env python3
"""Generuje ikony launcher'a B-Klasa Hero (PNG) bez zewnętrznych bibliotek.

Ikona: zielona murawa + biała piłka + żółty akcent (w stylu okręgowych
tablic wyników). Czysty standard biblioteki — działa na każdym hoście CI.

Użycie:
    python3 tools/icons/generate_icons.py [katalog_wyjściowy]

Domyślnie: android/app/src/main/res/

SPDX-License-Identifier: GPL-3.0-or-later
"""
import os
import struct
import sys
import zlib


def _chunk(tag: bytes, data: bytes) -> bytes:
    return (
        struct.pack(">I", len(data))
        + tag
        + data
        + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    )


def _png(width: int, height: int, pixels: bytearray) -> bytes:
    """Zapisuje RGBA (4 bajty/piksel) do PNG."""
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # filter: None
        raw.extend(pixels[y * width * 4 : (y + 1) * width * 4])
    return (
        b"\x89PNG\r\n\x1a\n"
        + _chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
        + _chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + _chunk(b"IEND", b"")
    )


def _make_icon(size: int, round_corner: bool) -> bytearray:
    """Rysuje: tło murawy + białą piłkę + żółty pas u dołu."""
    px = bytearray(size * size * 4)
    cx = size / 2.0
    cy = size / 2.0
    ball_r = size * 0.30
    stripe_y = size * 0.82

    GREEN = (44, 142, 63)     # murawa
    GREEN_DARK = (20, 99, 42) # cień
    WHITE = (247, 247, 242)
    YELLOW = (255, 196, 0)
    corner_r = size * 0.18 if round_corner else size * 0.02

    def inside_round_rect(x: float, y: float) -> bool:
        # Zaokrąglone rogi (promień corner_r).
        r = corner_r
        if x < r and y < r:
            return (x - r) ** 2 + (y - r) ** 2 <= r * r
        if x > size - r and y < r:
            return (x - (size - r)) ** 2 + (y - r) ** 2 <= r * r
        if x < r and y > size - r:
            return (x - r) ** 2 + (y - (size - r)) ** 2 <= r * r
        if x > size - r and y > size - r:
            return (x - (size - r)) ** 2 + (y - (size - r)) ** 2 <= r * r
        return True

    for y in range(size):
        for x in range(size):
            i = (y * size + x) * 4
            fx, fy = x + 0.5, y + 0.5
            if not inside_round_rect(fx, fy):
                px[i : i + 4] = (0, 0, 0, 0)
                continue
            # Murawa w pionowym gradiencie.
            t = fy / size
            r = int(GREEN[0] + (GREEN_DARK[0] - GREEN[0]) * t)
            g = int(GREEN[1] + (GREEN_DARK[1] - GREEN[1]) * t)
            b = int(GREEN[2] + (GREEN_DARK[2] - GREEN[2]) * t)
            # Żółty pas u dołu.
            if fy > stripe_y:
                r, g, b = YELLOW
            px[i : i + 4] = (r, g, b, 255)
            # Piłka (białe koło z lekkim cieniem).
            dx, dy = fx - cx, fy - cy
            d2 = dx * dx + dy * dy
            if d2 <= ball_r * ball_r:
                shade = 1.0 - (d2 / (ball_r * ball_r)) * 0.25
                r = int(WHITE[0] * shade)
                g = int(WHITE[1] * shade)
                b = int(WHITE[2] * shade)
                px[i : i + 4] = (r, g, b, 255)
    return px


DENSITIES = {
    "mdpi": 48,
    "hdpi": 72,
    "xhdpi": 96,
    "xxhdpi": 144,
    "xxxhdpi": 192,
}


def main() -> int:
    base = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(__file__), "..", "..", "android", "app", "src", "main", "res"
    )
    base = os.path.abspath(base)
    for density, size in DENSITIES.items():
        d = os.path.join(base, f"mipmap-{density}")
        os.makedirs(d, exist_ok=True)
        for name, rounded in (("ic_launcher", False), ("ic_launcher_round", True)):
            path = os.path.join(d, f"{name}.png")
            with open(path, "wb") as f:
                f.write(_png(size, size, _make_icon(size, rounded)))
            print(f"  {path}  ({size}x{size})")
    print("OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
