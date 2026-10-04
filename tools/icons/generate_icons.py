#!/usr/bin/env python3
"""Ikony launcher'a B-Klasa Hero — bez zewnętrznych bibliotek.

Znak: wieniec laurowy wokół piłki (ten sam motyw co zapowiedź meczu w grze),
na głębokiej zieleni ze złotym akcentem. Ten sam rysunek trafia do:
  * bitmap launcher'a (mipmap-*, kwadratowe i okrągłe),
  * adaptacyjnej ikony (wektory ic_launcher_background/foreground),
  * ikony aplikacji na stronie (192×192).

Rysowanie: własny rasteryzator (koło, obrócona elipsa, wielokąt, gradient)
z nadpróbkowaniem 4×, więc krawędzie są gładkie bez PIL-a i bez fontów.

Użycie:
    python3 tools/icons/generate_icons.py                     # do android/.../res
    python3 tools/icons/generate_icons.py --preview plik.png  # arkusz porównawczy
    python3 tools/icons/generate_icons.py --site plik.png     # ikona na stronę
    python3 tools/icons/generate_icons.py --variant 2         # inny wariant kolorystyczny

SPDX-License-Identifier: GPL-3.0-or-later
"""
from __future__ import annotations

import argparse
import math
import os
import struct
import tempfile
import zlib

DESIGN = 100.0  # przestrzeń projektu: 100×100 jednostek
DESIGNS = ("laurel", "shield", "pennant", "net", "roundel", "shot", "pin", "map")

VARIANTS = {
    1: dict(  # głęboka zieleń + złoty laur (domyślny)
        bg_top=(0x0F, 0x3D, 0x1E), bg_bottom=(0x2C, 0x8E, 0x3F),
        glow=(0x3A, 0xA2, 0x4A),
        laurel=(0xF2, 0xC1, 0x4E),
        ball=(0xF7, 0xF7, 0xF2), patch=(0x16, 0x20, 0x2E),
    ),
    2: dict(  # wieczorny mecz: granat + kremowy laur
        bg_top=(0x0B, 0x1F, 0x3A), bg_bottom=(0x12, 0x33, 0x5C),
        glow=(0x2E, 0x5A, 0x8C),
        laurel=(0xF7, 0xF7, 0xF2),
        ball=(0xFF, 0xC4, 0x00), patch=(0x16, 0x20, 0x2E),
    ),
    3: dict(  # kontrast: złoty laur na jaśniejszej murawie
        bg_top=(0x14, 0x63, 0x2A), bg_bottom=(0x3A, 0xA2, 0x4A),
        glow=(0x7F, 0xD2, 0x4E),
        laurel=(0xFF, 0xC4, 0x00),
        ball=(0xF7, 0xF7, 0xF2), patch=(0x14, 0x2B, 0x1C),
    ),
}

# Geometria znaku (w jednostkach projektu).
BALL_CX = BALL_CY = 50.0
BALL_R = 20.0
PENT_R = 7.4
PATCH_R = 6.0
PATCH_D = 16.0
LEAF_R = 33.5
LEAF_LEN = 12.5
LEAF_WID = 5.4
LEAF_COUNT = 6
LEAF_ANGLE_FROM = 36.0
LEAF_ANGLE_TO = 148.0


class Raster:
    """Bufor RGBA z nadpróbkowaniem; kształty podawane w jednostkach projektu."""

    def __init__(self, size: int, ss: int = 4):
        self.size = size
        self.ss = ss
        self.w = size * ss
        self.px = bytearray(self.w * self.w * 4)
        self.scale = self.w / DESIGN

    def _put(self, x: int, y: int, color, cov: float) -> None:
        if cov <= 0.0 or x < 0 or y < 0 or x >= self.w or y >= self.w:
            return
        if cov > 1.0:
            cov = 1.0
        i = (y * self.w + x) * 4
        buf = self.px
        if cov >= 1.0:
            buf[i], buf[i + 1], buf[i + 2], buf[i + 3] = color[0], color[1], color[2], 255
            return
        ia = 1.0 - cov
        a = buf[i + 3] / 255.0
        out_a = cov + a * ia
        if out_a <= 0:
            return
        buf[i] = int((color[0] * cov + buf[i] * a * ia) / out_a)
        buf[i + 1] = int((color[1] * cov + buf[i + 1] * a * ia) / out_a)
        buf[i + 2] = int((color[2] * cov + buf[i + 2] * a * ia) / out_a)
        buf[i + 3] = int(out_a * 255)

    def _bbox(self, x0, y0, x1, y1):
        s = self.scale
        return (max(0, int(x0 * s) - 1), max(0, int(y0 * s) - 1),
                min(self.w - 1, int(x1 * s) + 1), min(self.w - 1, int(y1 * s) + 1))

    def gradient(self, top, bottom) -> None:
        for y in range(self.w):
            t = y / max(1, self.w - 1)
            r = int(top[0] + (bottom[0] - top[0]) * t)
            g = int(top[1] + (bottom[1] - top[1]) * t)
            b = int(top[2] + (bottom[2] - top[2]) * t)
            self.px[y * self.w * 4:(y + 1) * self.w * 4] = bytes((r, g, b, 255)) * self.w

    def radial(self, cx, cy, radius, color, alpha=0.30) -> None:
        x0, y0, x1, y1 = self._bbox(cx - radius, cy - radius, cx + radius, cy + radius)
        s = self.scale
        for y in range(y0, y1 + 1):
            dy = (y + 0.5) / s - cy
            for x in range(x0, x1 + 1):
                dx = (x + 0.5) / s - cx
                d = math.hypot(dx, dy) / radius
                if d < 1.0:
                    self._put(x, y, color, alpha * (1.0 - d) ** 1.4)

    def circle(self, cx, cy, r, color) -> None:
        x0, y0, x1, y1 = self._bbox(cx - r, cy - r, cx + r, cy + r)
        s = self.scale
        rr = r * s
        for y in range(y0, y1 + 1):
            dy = y + 0.5 - cy * s
            for x in range(x0, x1 + 1):
                dx = x + 0.5 - cx * s
                if dx * dx + dy * dy <= rr * rr:
                    self._put(x, y, color, 1.0)

    def ellipse_rot(self, cx, cy, rx, ry, deg, color) -> None:
        rad = math.radians(deg)
        ca, sa = math.cos(rad), math.sin(rad)
        ext = max(rx, ry) + 1
        x0, y0, x1, y1 = self._bbox(cx - ext, cy - ext, cx + ext, cy + ext)
        s = self.scale
        for y in range(y0, y1 + 1):
            dy = (y + 0.5) / s - cy
            for x in range(x0, x1 + 1):
                dx = (x + 0.5) / s - cx
                u = dx * ca + dy * sa
                v = -dx * sa + dy * ca
                if (u / rx) ** 2 + (v / ry) ** 2 <= 1.0:
                    self._put(x, y, color, 1.0)

    def polygon(self, points, color) -> None:
        xs = [p[0] for p in points]
        ys = [p[1] for p in points]
        x0, y0, x1, y1 = self._bbox(min(xs), min(ys), max(xs), max(ys))
        s = self.scale
        pts = [(p[0] * s, p[1] * s) for p in points]
        n = len(pts)
        for y in range(y0, y1 + 1):
            py = y + 0.5
            for x in range(x0, x1 + 1):
                pxx = x + 0.5
                inside = False
                j = n - 1
                for i in range(n):
                    xi, yi = pts[i]
                    xj, yj = pts[j]
                    if (yi > py) != (yj > py):
                        t = (py - yi) / (yj - yi)
                        if pxx < xi + t * (xj - xi):
                            inside = not inside
                    j = i
                if inside:
                    self._put(x, y, color, 1.0)

    def rect(self, x0, y0, x1, y1, color) -> None:
        ax0, ay0, ax1, ay1 = self._bbox(min(x0, x1), min(y0, y1), max(x0, x1), max(y0, y1))
        s = self.scale
        for y in range(ay0, ay1 + 1):
            if not (y0 * s <= y + 0.5 <= y1 * s):
                continue
            for x in range(ax0, ax1 + 1):
                if x0 * s <= x + 0.5 <= x1 * s:
                    self._put(x, y, color, 1.0)

    def ring(self, cx, cy, r, width, color) -> None:
        x0, y0, x1, y1 = self._bbox(cx - r - width, cy - r - width, cx + r + width, cy + r + width)
        s = self.scale
        ro, ri = (r + width / 2.0) * s, max(0.0, (r - width / 2.0)) * s
        for y in range(y0, y1 + 1):
            dy = y + 0.5 - cy * s
            for x in range(x0, x1 + 1):
                dx = x + 0.5 - cx * s
                d2 = dx * dx + dy * dy
                if ri * ri <= d2 <= ro * ro:
                    self._put(x, y, color, 1.0)

    def mask_round_rect(self, corner_ratio=0.18) -> None:
        rr = DESIGN * corner_ratio * self.scale
        w = self.w
        for y in range(w):
            dy = y + 0.5
            cdy = min(max(dy, rr), w - rr)
            for x in range(w):
                dx = x + 0.5
                cdx = min(max(dx, rr), w - rr)
                if (dx - cdx) ** 2 + (dy - cdy) ** 2 > rr * rr:
                    self.px[(y * w + x) * 4 + 3] = 0

    def mask_circle(self) -> None:
        w = self.w
        c = w / 2.0
        for y in range(w):
            dy = y + 0.5 - c
            for x in range(w):
                dx = x + 0.5 - c
                if dx * dx + dy * dy > c * c:
                    self.px[(y * w + x) * 4 + 3] = 0

    def to_png(self, path: str) -> None:
        ss, size, w, src = self.ss, self.size, self.w, self.px
        out = bytearray(size * size * 4)
        n = ss * ss
        for y in range(size):
            for x in range(size):
                r = g = b = a = 0
                for sy in range(ss):
                    base = ((y * ss + sy) * w + x * ss) * 4
                    for sx in range(ss):
                        i = base + sx * 4
                        r += src[i]; g += src[i + 1]; b += src[i + 2]; a += src[i + 3]
                o = (y * size + x) * 4
                out[o] = r // n; out[o + 1] = g // n; out[o + 2] = b // n; out[o + 3] = a // n
        with open(path, "wb") as fh:
            fh.write(_png(size, size, out))


def _chunk(tag: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)


def _png(width: int, height: int, pixels: bytearray) -> bytes:
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        raw.extend(pixels[y * width * 4:(y + 1) * width * 4])
    return (b"\x89PNG\r\n\x1a\n"
            + _chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
            + _chunk(b"IDAT", zlib.compress(bytes(raw), 9))
            + _chunk(b"IEND", b""))


def draw_mark(r: Raster, v: dict, with_background: bool = True) -> None:
    """Wieniec laurowy + piłka. `with_background=False` — sam znak (do adaptive)."""
    if with_background:
        r.gradient(v["bg_top"], v["bg_bottom"])
        r.radial(BALL_CX, BALL_CY, 62.0, v["glow"], 0.35)

    step = (LEAF_ANGLE_TO - LEAF_ANGLE_FROM) / (LEAF_COUNT - 1)
    for side in (-1, 1):
        for i in range(LEAF_COUNT):
            deg = LEAF_ANGLE_FROM + i * step
            rad = math.radians(deg)
            lx = BALL_CX + side * LEAF_R * math.sin(rad)
            ly = BALL_CY - LEAF_R * math.cos(rad)
            r.ellipse_rot(lx, ly, LEAF_LEN / 2.0, LEAF_WID / 2.0, side * deg, v["laurel"])

    r.circle(BALL_CX, BALL_CY, BALL_R, v["ball"])
    pent = [(BALL_CX + PENT_R * math.sin(math.radians(90 + k * 72)),
             BALL_CY - PENT_R * math.cos(math.radians(90 + k * 72))) for k in range(5)]
    r.polygon(pent, v["patch"])
    for k in range(5):
        ang = math.radians(-90 + k * 72)
        pxx = BALL_CX + PATCH_D * math.cos(ang)
        pyy = BALL_CY + PATCH_D * math.sin(ang)
        patch = [(pxx + PATCH_R * math.cos(ang + math.radians(90 + j * 72)),
                  pyy + PATCH_R * math.sin(ang + math.radians(90 + j * 72))) for j in range(5)]
        r.polygon(patch, v["patch"])


def draw_ball(r: Raster, cx: float, cy: float, radius: float, v: dict) -> None:
    """Piłka: kremowa kula + środkowy pięciokąt + łaty przy brzegu."""
    scale = radius / BALL_R
    r.circle(cx, cy, radius, v["ball"])
    pent = [(cx + PENT_R * scale * math.sin(math.radians(90 + k * 72)),
             cy - PENT_R * scale * math.cos(math.radians(90 + k * 72))) for k in range(5)]
    r.polygon(pent, v["patch"])
    for k in range(5):
        ang = math.radians(-90 + k * 72)
        pxx = cx + PATCH_D * scale * math.cos(ang)
        pyy = cy + PATCH_D * scale * math.sin(ang)
        patch = [(pxx + PATCH_R * scale * math.cos(ang + math.radians(90 + j * 72)),
                  pyy + PATCH_R * scale * math.sin(ang + math.radians(90 + j * 72))) for j in range(5)]
        r.polygon(patch, v["patch"])


def draw_design(r: Raster, v: dict, design: str) -> None:
    """Znak aplikacji. `design` wybiera koncepcję (kształt), `v` kolory."""
    r.gradient(v["bg_top"], v["bg_bottom"])
    r.radial(BALL_CX, BALL_CY, 62.0, v["glow"], 0.35)
    gold, cream, patch = v["laurel"], v["ball"], v["patch"]

    if design == "laurel":
        draw_mark(r, v, with_background=False)

    elif design == "shield":
        # tarcza herbowa z piłką i złotym pasem
        outer = [(50, 9), (87, 20), (87, 56), (50, 91), (13, 56), (13, 20)]
        inner = [(50, 17), (80, 26), (80, 54), (50, 82), (20, 54), (20, 26)]
        r.polygon(outer, gold)
        r.polygon(inner, v["bg_bottom"])
        r.radial(50, 44, 40, v["glow"], 0.30)
        draw_ball(r, 50, 42, 15.5, v)
        r.rect(26, 57, 74, 62, gold)
        for k in range(3):
            r.circle(42 + k * 8, 69, 2.4, gold)

    elif design == "pennant":
        # proporzec na maszcie
        r.rect(23, 14, 29, 88, gold)
        r.circle(26, 13, 3.4, gold)
        tri_out = [(31, 19), (89, 37), (31, 55)]
        tri_in = [(36, 25), (79, 37), (36, 49)]
        r.polygon(tri_out, gold)
        r.polygon(tri_in, v["bg_bottom"])
        draw_ball(r, 52, 37, 9.5, v)

    elif design == "net":
        # piłka w siatce bramki
        r.rect(10, 16, 90, 18.6, cream)
        r.rect(10, 16, 12.6, 74, cream)
        r.rect(87.4, 16, 90, 74, cream)
        for i in range(1, 9):
            x = 10 + i * 10.0
            r.rect(x - 0.5, 18, x + 0.5, 74, (cream[0] // 2, cream[1] // 2, cream[2] // 2))
        for i in range(1, 6):
            y = 16 + i * 9.6
            r.rect(12, y - 0.5, 88, y + 0.5, (cream[0] // 2, cream[1] // 2, cream[2] // 2))
        draw_ball(r, 54, 58, 19.0, v)

    elif design == "roundel":
        # okrągła odznaka: złoty pierścień, piłka i kropki
        r.ring(50, 50, 37, 4.5, gold)
        draw_ball(r, 50, 44, 16.5, v)
        for k in range(3):
            r.circle(41 + k * 9, 71, 2.6, gold)

    elif design == "shot":
        # piłka w locie ze smugami
        for y, base, tip in ((34, 6, 34), (48, 8, 42), (62, 6, 34)):
            r.polygon([(base, y - 5.4), (tip, y), (base, y + 5.4)], gold)
        draw_ball(r, 62, 48, 21.0, v)

    elif design == "pin":
        # wskaznik mapy z piłką w środku
        cx, cy, rad = 50.0, 39.0, 18.5
        r.polygon([(cx - rad * 0.70, cy + rad * 0.70), (cx, cy + rad * 2.15),
                   (cx + rad * 0.70, cy + rad * 0.70)], gold)
        r.circle(cx, cy, rad, gold)
        r.polygon([(cx - rad * 0.60, cy + rad * 0.74), (cx, cy + rad * 1.92),
                   (cx + rad * 0.60, cy + rad * 0.74)], v["bg_bottom"])
        r.circle(cx, cy, rad - 3.6, v["bg_bottom"])
        draw_ball(r, cx, cy, rad - 6.4, v)
        # kropki trasy dochodzące do wskaznika
        for i in range(4):
            r.circle(16 + i * 7.5, 76 - i * 3.0, 2.3 - i * 0.15, gold)

    elif design == "map":
        # mapa z trasą: piłka u nas, wskaznik u rywala
        paper = (0xEC, 0xF2, 0xE9)
        grid = (0xC3, 0xD3, 0xC6)
        shade = (0xD3, 0xDF, 0xD6)
        r.rect(12, 26, 88, 78, paper)
        for i in range(1, 7):
            x = 12 + i * (76.0 / 7)
            r.rect(x - 0.4, 26, x + 0.4, 78, grid)
        for i in range(1, 5):
            y = 26 + i * (52.0 / 5)
            r.rect(12, y - 0.4, 88, y + 0.4, grid)
        # zgięcie mapy
        r.polygon([(50, 26), (56, 26), (50, 78), (44, 78)], shade)
        # trasa
        trasa = [(24, 68), (33, 63), (40, 55), (49, 49), (58, 44), (66, 41)]
        for i, (x, y) in enumerate(trasa):
            r.circle(x, y, 2.5 if i % 2 == 0 else 1.8, gold)
        # pin rywala i piłka u nas
        px, py, prad = 70.0, 38.0, 7.5
        r.polygon([(px - prad * 0.70, py + prad * 0.70), (px, py + prad * 2.1),
                   (px + prad * 0.70, py + prad * 0.70)], v["laurel"])
        r.circle(px, py, prad, v["laurel"])
        r.circle(px, py, prad - 2.6, paper)
        draw_ball(r, 24, 68, 9.5, v)

    else:
        raise SystemExit("nieznany projekt: %s" % design)


def icon(size: int, variant: int = 1, round_icon: bool = False, adaptive: bool = False,
         design: str = "laurel") -> Raster:
    v = VARIANTS[variant]
    r = Raster(size)
    if adaptive:
        r.gradient(v["bg_top"], v["bg_bottom"])
        r.radial(BALL_CX, BALL_CY, 62.0, v["glow"], 0.35)
        mark = Raster(size)
        draw_design(mark, v, design)
        # znak bez tła: zostawiamy tylko piksele różniące się od gradientu
        _composite_mark(r, mark, 0.62)
    else:
        draw_design(r, v, design)
        r.mask_circle() if round_icon else r.mask_round_rect()
    return r


def _composite_mark(dst: Raster, src: Raster, scale: float) -> None:
    """Wkleja sam ZNAK (nie tło) — piksele inne niż lokalny gradient."""
    dsz, ssz = dst.w, src.w
    half, off = ssz / 2.0, dsz / 2.0
    for y in range(dsz):
        sy = int((y - off) / scale + half)
        if sy < 0 or sy >= ssz:
            continue
        for x in range(dsz):
            sx = int((x - off) / scale + half)
            if sx < 0 or sx >= ssz:
                continue
            i = (sy * ssz + sx) * 4
            if not src.px[i + 3]:
                continue
            # piksele tła mają kolor z tego samego gradientu — pomijamy je
            dst._put(x, y, (src.px[i], src.px[i + 1], src.px[i + 2]), 1.0)


def _composite_scaled(dst: Raster, src: Raster, scale: float) -> None:
    dsz, ssz = dst.w, src.w
    half, off = ssz / 2.0, dsz / 2.0
    for y in range(dsz):
        sy = int((y - off) / scale + half)
        if sy < 0 or sy >= ssz:
            continue
        for x in range(dsz):
            sx = int((x - off) / scale + half)
            if sx < 0 or sx >= ssz:
                continue
            i = (sy * ssz + sx) * 4
            a = src.px[i + 3]
            if a:
                dst._put(x, y, (src.px[i], src.px[i + 1], src.px[i + 2]), a / 255.0)


def _fmt(v: float) -> str:
    return ("%.2f" % v).rstrip("0").rstrip(".")


def vector_background(variant: int = 1) -> str:
    v = VARIANTS[variant]
    top = "#%02X%02X%02X" % v["bg_top"]
    bottom = "#%02X%02X%02X" % v["bg_bottom"]
    return f"""<?xml version="1.0" encoding="utf-8"?>
<!-- Tło adaptacyjnej ikony: głęboka zieleń w pionowym gradiencie. -->
<vector xmlns:android="http://schemas.android.com/apk/res/android"
    xmlns:aapt="http://schemas.android.com/aapt"
    android:width="108dp"
    android:height="108dp"
    android:viewportWidth="108"
    android:viewportHeight="108">
    <path android:pathData="M0,0 h108 v108 h-108 z">
        <aapt:attr name="android:fillColor">
            <gradient android:type="linear"
                android:startX="54" android:startY="0"
                android:endX="54" android:endY="108"
                android:startColor="{top}" android:endColor="{bottom}" />
        </aapt:attr>
    </path>
</vector>
"""


def vector_foreground(variant: int = 1) -> str:
    v = VARIANTS[variant]
    laurel = "#%02X%02X%02X" % v["laurel"]
    ball = "#%02X%02X%02X" % v["ball"]
    patch = "#%02X%02X%02X" % v["patch"]
    k = 66.0 / DESIGN          # strefa bezpieczna adaptive (66 z 108)
    ox = oy = (108.0 - DESIGN * k) / 2.0

    def P(x: float, y: float):
        return (ox + x * k, oy + y * k)

    parts = ['<?xml version="1.0" encoding="utf-8"?>',
             '<!-- Foreground adaptacyjnej ikony: wieniec laurowy wokół piłki. -->',
             '<vector xmlns:android="http://schemas.android.com/apk/res/android"',
             '    android:width="108dp"',
             '    android:height="108dp"',
             '    android:viewportWidth="108"',
             '    android:viewportHeight="108">']

    step = (LEAF_ANGLE_TO - LEAF_ANGLE_FROM) / (LEAF_COUNT - 1)
    for side in (-1, 1):
        for i in range(LEAF_COUNT):
            deg = LEAF_ANGLE_FROM + i * step
            rad = math.radians(deg)
            lx = BALL_CX + side * LEAF_R * math.sin(rad)
            ly = BALL_CY - LEAF_R * math.cos(rad)
            ax, ay = P(lx - LEAF_LEN / 2.0, ly)
            bx, by = P(lx + LEAF_LEN / 2.0, ly)
            rx, ry = LEAF_LEN / 2.0 * k, LEAF_WID / 2.0 * k
            d = (f"M{_fmt(ax)},{_fmt(ay)} "
                 f"A{_fmt(rx)},{_fmt(ry)},{_fmt(-side * deg)} 1 0 {_fmt(bx)},{_fmt(by)} "
                 f"A{_fmt(rx)},{_fmt(ry)},{_fmt(-side * deg)} 1 0 {_fmt(ax)},{_fmt(ay)} Z")
            parts.append(f'    <path android:fillColor="{laurel}" android:pathData="{d}" />')

    cx, cy = P(BALL_CX, BALL_CY)
    r = BALL_R * k
    parts.append(f'    <path android:fillColor="{ball}" android:pathData="'
                 f'M{_fmt(cx - r)},{_fmt(cy)} a{_fmt(r)},{_fmt(r)} 0 1 0 {_fmt(2 * r)},0 '
                 f'a{_fmt(r)},{_fmt(r)} 0 1 0 {_fmt(-2 * r)},0 Z" />')

    def pentagon(pxx: float, pyy: float, radius: float, start_deg: float) -> str:
        pts = []
        for j in range(5):
            a = math.radians(start_deg + j * 72)
            x, y = P(pxx + radius * math.cos(a), pyy + radius * math.sin(a))
            pts.append((x, y))
        return "M" + " L".join(f"{_fmt(x)},{_fmt(y)}" for x, y in pts) + " Z"

    parts.append(f'    <path android:fillColor="{patch}" android:pathData="'
                 f'{pentagon(BALL_CX, BALL_CY, PENT_R, -90)}" />')
    for kk in range(5):
        ang = -90 + kk * 72
        pxx = BALL_CX + PATCH_D * math.cos(math.radians(ang))
        pyy = BALL_CY + PATCH_D * math.sin(math.radians(ang))
        parts.append(f'    <path android:fillColor="{patch}" android:pathData="'
                     f'{pentagon(pxx, pyy, PATCH_R, ang + 90)}" />')

    parts.append('</vector>')
    return "\n".join(parts) + "\n"


DENSITIES = {"mdpi": 48, "hdpi": 72, "xhdpi": 96, "xxhdpi": 144, "xxxhdpi": 192}


def generate_res(res_dir: str, variant: int = 1, design: str = "laurel") -> None:
    for density, size in DENSITIES.items():
        folder = os.path.join(res_dir, "mipmap-%s" % density)
        os.makedirs(folder, exist_ok=True)
        icon(size, variant, design=design).to_png(os.path.join(folder, "ic_launcher.png"))
        icon(size, variant, round_icon=True, design=design).to_png(
            os.path.join(folder, "ic_launcher_round.png"))
    draw = os.path.join(res_dir, "drawable")
    os.makedirs(draw, exist_ok=True)
    with open(os.path.join(draw, "ic_launcher_background.xml"), "w", encoding="utf-8") as fh:
        fh.write(vector_background(variant))
    with open(os.path.join(draw, "ic_launcher_foreground.xml"), "w", encoding="utf-8") as fh:
        fh.write(vector_foreground(variant))
    print(">>> ikony zapisane w %s (projekt %s, wariant %d)" % (res_dir, design, variant))


def make_preview(path: str) -> None:
    """Arkusz: sześć koncepcji znaku + warianty kolorystyczne i rozmiary."""
    from PIL import Image, ImageDraw, ImageFont  # tylko podgląd (dev)

    cell, pad = 200, 20
    cols = 6
    rows = 3
    head = 74
    sheet = Image.new("RGB", (pad + cols * (cell + pad), head + rows * (cell + pad + 26)),
                      (238, 241, 244))
    d = ImageDraw.Draw(sheet)
    fpath = "/usr/share/fonts/liberation-sans/LiberationSans-Bold.ttf"
    f = ImageFont.truetype(fpath, 14)
    t = ImageFont.truetype(fpath, 21)
    d.text((pad, 10), "B-Klasa Hero — propozycje logo (każda to inny znak)", font=t, fill=(28, 36, 44))
    d.text((pad, 40), "Wariant kolorystyczny i rozmiar wybiera się niezależnie od znaku "
                      "(--design / --variant).", font=f, fill=(96, 110, 122))

    def png(size, variant=1, rnd=False, design="laurel"):
        tmp = tempfile.NamedTemporaryFile(suffix=".png", delete=False)
        tmp.close()
        icon(size, variant, round_icon=rnd, design=design).to_png(tmp.name)
        im = Image.open(tmp.name).convert("RGBA")
        os.unlink(tmp.name)
        return im

    opisy = {
        "laurel": "1. Laur + piłka (obecny)",
        "shield": "2. Tarcza herbowa",
        "pennant": "3. Proporzec",
        "net": "4. Piłka w siatce",
        "roundel": "5. Okrągła odznaka",
        "shot": "6. Piłka w locie",
        "pin": "7. Wskaźnik z piłką",
        "map": "8. Mapa z trasą",
    }

    def put(col, row, im, label):
        x = pad + col * (cell + pad)
        y = head + row * (cell + pad + 26)
        thumb = im.resize((cell, cell), Image.LANCZOS)
        bg = Image.new("RGB", (cell, cell), (255, 255, 255))
        bg.paste(thumb, (0, 0), thumb)
        sheet.paste(bg, (x, y))
        d.rectangle([x, y, x + cell, y + cell], outline=(203, 211, 219))
        d.text((x + cell // 2, y + cell + 5), label, font=f, fill=(70, 84, 96), anchor="ma")

    for i, design in enumerate(DESIGNS[:6]):
        put(i, 0, png(256, 1, design=design), opisy[design])

    # trzeci rząd: realny rozmiar launcher'a (48 px) — tak widać znak na telefonie
    for i, design in enumerate(DESIGNS):
        small = png(48, 1, design=design)
        put(i, 2, small, "48 px — %s" % opisy[design].split(". ", 1)[1])
        # podgląd 1:1 (48 px) na środku komórki, żeby nie oceniać skalowania
        x = pad + i * (cell + pad) + cell // 2 - small.width // 2
        y = head + 2 * (cell + pad + 26) + cell // 2 - small.height // 2
        sheet.paste(small.convert("RGB"), (x, y))

    put(0, 1, png(256, 1, design="pin"), opisy["pin"])
    put(1, 1, png(256, 1, design="map"), opisy["map"])
    put(2, 1, png(192, 1), "192 px (xxxhdpi)")
    put(3, 1, png(48, 1), "48 px (mdpi)")
    put(4, 1, png(256, 1, True), "okrągła")
    put(5, 1, png(256, 2), "wariant 2 — wieczór")
    sheet.save(path)
    print(">>> arkusz zapisany:", path)


def main() -> None:
    here = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--out", default=os.path.join(here, "android", "app", "src", "main", "res"))
    ap.add_argument("--variant", type=int, default=1, choices=sorted(VARIANTS))
    ap.add_argument("--design", default="laurel", choices=DESIGNS)
    ap.add_argument("--site", help="zapisz ikonę 192×192 pod tą ścieżką")
    ap.add_argument("--preview", help="zapisz arkusz porównawczy pod tą ścieżką")
    args = ap.parse_args()

    if args.site:
        icon(192, args.variant, design=args.design).to_png(args.site)
        print(">>> ikona na stronę:", args.site)
    if args.preview:
        make_preview(args.preview)
    if not args.site and not args.preview:
        generate_res(args.out, args.variant, args.design)


if __name__ == "__main__":
    main()
