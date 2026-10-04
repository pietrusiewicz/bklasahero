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


def icon(size: int, variant: int = 1, round_icon: bool = False, adaptive: bool = False) -> Raster:
    v = VARIANTS[variant]
    r = Raster(size)
    if adaptive:
        r.gradient(v["bg_top"], v["bg_bottom"])
        r.radial(BALL_CX, BALL_CY, 62.0, v["glow"], 0.35)
        mark = Raster(size)
        draw_mark(mark, v, with_background=False)
        _composite_scaled(r, mark, 0.62)
    else:
        draw_mark(r, v, with_background=True)
        r.mask_circle() if round_icon else r.mask_round_rect()
    return r


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


def generate_res(res_dir: str, variant: int = 1) -> None:
    for density, size in DENSITIES.items():
        folder = os.path.join(res_dir, "mipmap-%s" % density)
        os.makedirs(folder, exist_ok=True)
        icon(size, variant).to_png(os.path.join(folder, "ic_launcher.png"))
        icon(size, variant, round_icon=True).to_png(os.path.join(folder, "ic_launcher_round.png"))
    draw = os.path.join(res_dir, "drawable")
    os.makedirs(draw, exist_ok=True)
    with open(os.path.join(draw, "ic_launcher_background.xml"), "w", encoding="utf-8") as fh:
        fh.write(vector_background(variant))
    with open(os.path.join(draw, "ic_launcher_foreground.xml"), "w", encoding="utf-8") as fh:
        fh.write(vector_foreground(variant))
    print(">>> ikony zapisane w %s (wariant %d)" % (res_dir, variant))


def make_preview(path: str) -> None:
    """Arkusz: warianty, wariant okrągły i rozmiary launcher'a."""
    from PIL import Image, ImageDraw, ImageFont  # tylko podgląd (dev)

    cell, pad, cols = 230, 22, 4
    rows = 2
    sheet = Image.new("RGB", (pad + cols * (cell + pad), 44 + rows * (cell + pad + 28)),
                      (238, 241, 244))
    d = ImageDraw.Draw(sheet)
    f = ImageFont.truetype("/usr/share/fonts/liberation-sans/LibertationSans-Bold.ttf"
                           if os.path.exists("/usr/share/fonts/liberation-sans/LibertationSans-Bold.ttf")
                           else "/usr/share/fonts/liberation-sans/LiberationSans-Bold.ttf", 15)
    t = ImageFont.truetype("/usr/share/fonts/liberation-sans/LiberationSans-Bold.ttf", 22)
    d.text((pad, 8), "B-Klasa Hero — nowe logo (warianty i rozmiary)", font=t, fill=(28, 36, 44))

    def png(size, variant, rnd=False, adaptive=False):
        tmp = tempfile.NamedTemporaryFile(suffix=".png", delete=False)
        tmp.close()
        icon(size, variant, round_icon=rnd, adaptive=adaptive).to_png(tmp.name)
        im = Image.open(tmp.name).convert("RGBA")
        os.unlink(tmp.name)
        return im

    grid = [
        [("wariant 1 (domyślny)", png(256, 1)), ("wariant 2 — wieczór", png(256, 2)),
         ("wariant 3 — kontrast", png(256, 3)), ("okrągła", png(256, 1, True))],
        [("192 px (xxxhdpi)", png(192, 1)), ("96 px (xhdpi)", png(96, 1)),
         ("48 px (mdpi)", png(48, 1)), ("48 px okrągła", png(48, 1, True))],
    ]
    for row_i, row in enumerate(grid):
        for col_i, (label, im) in enumerate(row):
            x = pad + col_i * (cell + pad)
            y = 44 + row_i * (cell + pad + 28)
            thumb = im.resize((cell, cell), Image.LANCZOS)
            bg = Image.new("RGB", (cell, cell), (255, 255, 255))
            bg.paste(thumb, (0, 0), thumb)
            sheet.paste(bg, (x, y))
            d.rectangle([x, y, x + cell, y + cell], outline=(203, 211, 219))
            d.text((x + cell // 2, y + cell + 6), label, font=f, fill=(70, 84, 96), anchor="ma")
    sheet.save(path)
    print(">>> arkusz zapisany:", path)


def main() -> None:
    here = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--out", default=os.path.join(here, "android", "app", "src", "main", "res"))
    ap.add_argument("--variant", type=int, default=1, choices=sorted(VARIANTS))
    ap.add_argument("--site", help="zapisz ikonę 192×192 pod tą ścieżką")
    ap.add_argument("--preview", help="zapisz arkusz porównawczy pod tą ścieżką")
    args = ap.parse_args()

    if args.site:
        icon(192, args.variant).to_png(args.site)
        print(">>> ikona na stronę:", args.site)
    if args.preview:
        make_preview(args.preview)
    if not args.site and not args.preview:
        generate_res(args.out, args.variant)


if __name__ == "__main__":
    main()
