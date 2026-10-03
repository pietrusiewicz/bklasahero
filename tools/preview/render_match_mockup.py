#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Podglad ekranu PRZEDMECZOWEGO B-Klasa Hero (MatchScreen.kt).

Rysuje zapowiedz meczu: laurkowy wieniec, nazwy klubow, miejscowosci
i odleglosc miedzy nimi. To mockup ukladu z Compose, nie zrzut z urzadzenia.

Uruchomienie:
    python3 tools/preview/render_match_mockup.py docs/match-preview.png
"""
from __future__ import annotations

import os
import sys

from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from render_menu_mockup import (  # noqa: E402
    CW,
    DARK,
    H,
    I_SOCCER,
    LIGHT,
    PAD,
    PAGE_BG,
    PAGE_FG,
    PAGE_MUTED,
    W,
    WHITE,
    Canvas,
    font,
    line_h,
    px,
)

# Kolory boiska — te same, co w ui/render/MatchRenderer.kt.
SKY_TOP = (0x12, 0x33, 0x5C)
SKY_BOTTOM = (0x7F, 0xA8, 0xD9)
STANDS = (0x24, 0x30, 0x3E)
CROWD_A = (0x2E, 0x3C, 0x4E)
CROWD_B = (0x1B, 0x25, 0x30)
GRASS_BASE = (0x2F, 0x8B, 0x3B)
GRASS_LIGHT = (0x3A, 0xA2, 0x4A)
GRASS_DARK = (0x28, 0x7C, 0x34)
LINE_WHITE = (0xF2, 0xF6, 0xF2)
POST_WHITE = (0xFA, 0xFA, 0xFA)
KEEPER = (0xF4, 0xB4, 0x00)
BALL = (0xF7, 0xF7, 0xF2, 255)

def blend(fg, bg, a):
    """Kolor fg o przezroczystosci a polozony na bg."""
    return tuple(int(round(bg[i] + (fg[i] - bg[i]) * a)) for i in range(3)) + (255,)


HORIZON = 0.294 * H  # linia murawy
GOAL_TOP = 470.0
GOAL_BOTTOM = 560.0
GOAL_LEFT = 118.0
GOAL_RIGHT = 242.0


def pitch(c: Canvas) -> None:
    """Uproszczony stadion: niebo, trybuny, murawa z pasami, pole karne, bramka."""
    d = c.d
    sky_bottom = H * 0.1625
    d.rectangle([0, 0, px(W), px(sky_bottom)], fill=SKY_TOP)
    d.rectangle([0, px(sky_bottom), px(W), px(HORIZON)], fill=STANDS)
    for i in range(26):
        x = i * (W / 26.0)
        d.rectangle(
            [px(x + 2), px(sky_bottom + 20), px(x + 11), px(HORIZON - 8)],
            fill=CROWD_A if i % 2 == 0 else CROWD_B,
        )
    d.rectangle([0, px(HORIZON), px(W), px(H)], fill=GRASS_BASE)
    stripe = (H - HORIZON) / 8.0
    for i in range(8):
        if i % 2 == 0:
            d.rectangle(
                [0, px(HORIZON + i * stripe), px(W), px(HORIZON + (i + 1) * stripe)],
                fill=GRASS_LIGHT,
            )
    d.line([px(24), px(HORIZON + 10), px(24), px(H)], fill=LINE_WHITE, width=px(2))
    d.line([px(W - 24), px(HORIZON + 10), px(W - 24), px(H)], fill=LINE_WHITE, width=px(2))
    area_line = H * 0.8075
    box_top = H * 0.7325
    d.line([px(24), px(area_line), px(W - 24), px(area_line)], fill=LINE_WHITE, width=px(2))
    d.line([px(56), px(box_top), px(56), px(H)], fill=LINE_WHITE, width=px(2))
    d.line([px(W - 56), px(box_top), px(W - 56), px(H)], fill=LINE_WHITE, width=px(2))
    d.line([px(56), px(box_top), px(W - 56), px(box_top)], fill=LINE_WHITE, width=px(2))


def scoreboard_bar(c: Canvas, home: str, away: str, dots: int = 5) -> None:
    """Pasek wyniku na gorze ekranu (odpowiednik ScoreboardBar)."""
    h = 74
    c.card(0, 0, W, h, 0, (0x16, 0x20, 0x2E, 0xCC))
    c.d.rounded_rectangle(
        [0, px(h - 14), px(W), px(h + 10)], radius=px(14), fill=(0x16, 0x20, 0x2E, 0xCC)
    )
    c.text(PAD, 12, home, 16, WHITE, bold=True)
    c.text(0, 12, away, 16, WHITE, bold=True, right=W - PAD)
    for i in range(dots):
        x = PAD + i * 12
        c.bar(x, 36, 8, 8, (0x42, 0x52, 0x6B, 255))
        x2 = W - PAD - 8 - i * 12
        c.bar(x2, 36, 8, 8, (0x42, 0x52, 0x6B, 255))
    c.text(0, 8, "0 : 0", 28, WHITE, bold=True, center=W / 2)
    c.text(0, 52, "Rzut 1 z 10", 11, (0xC7, 0xD3, 0xE0, 255), center=W / 2)


def laurel_crest(c: Canvas, th) -> None:
    """Wieniec laurowy + piłka — ozdoba zapowiedzi (odpowiednik LaurelCrest)."""
    size = 92.0
    x0 = PAD + (CW - size) / 2
    y0 = 0.0  # rysowane w lokalnym ukladzie karty
    cx, cy = x0 + size / 2, y0 + size / 2
    radius = size * 0.40
    leaf_len = size * 0.17
    leaf_wid = size * 0.075
    leaves = 7
    import math

    for side in (-1, 1):
        for i in range(leaves):
            deg = 30.0 + i * (130.0 / (leaves - 1))
            rad = math.radians(deg)
            lx = cx + side * radius * math.sin(rad)
            ly = cy - radius * math.cos(rad)
            _leaf(c, lx, ly, leaf_len, leaf_wid, side * deg, th["accent"])
    c.circle(cx, cy, size * 0.25, th["heroBottom"])
    c.text(0, cy - 15, I_SOCCER, 30, th["accent"], fa=True, center=cx)


def _leaf(c: Canvas, cx: float, cy: float, length: float, width: float, deg: float, color) -> None:
    """Listek: owal obrocony o `deg` (PIL kreci sie odwrotnie niz Compose)."""
    pad = 3.0
    layer = Image.new("RGBA", (px(length + 2 * pad), px(width + 2 * pad)), (0, 0, 0, 0))
    ImageDraw.Draw(layer).ellipse(
        [px(pad), px(pad), px(pad + length), px(pad + width)], fill=color
    )
    rotated = layer.rotate(-deg, resample=Image.BICUBIC, expand=True)
    c.img.alpha_composite(
        rotated, (px(cx) - rotated.width // 2, px(cy) - rotated.height // 2)
    )



# --- Liga z prawdziwego katalogu (do mapki) --------------------------------

def load_league(home_name: str = "Maków Mazowiecki", count: int = 9):
    """Zwraca (miasto gracza, [(odleglosc_km, place)]) z data/places/places_pl.csv."""
    import csv
    import math

    path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "..", "data", "places", "places_pl.csv"
    )
    rows = list(csv.DictReader(open(path, encoding="utf-8")))
    for r in rows:
        r["lat"] = float(r["lat"])
        r["lon"] = float(r["lon"])
    home = next(r for r in rows if r["name"] == home_name)

    def hav(a, b):
        R = 6371.0
        p1, p2 = math.radians(a["lat"]), math.radians(b["lat"])
        dp = p2 - p1
        dl = math.radians(b["lon"] - a["lon"])
        h = math.sin(dp / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(dl / 2) ** 2
        return 2 * R * math.asin(math.sqrt(h))

    near = sorted(
        (p for p in rows if p["osm_id"] != home["osm_id"]), key=lambda p: hav(home, p)
    )[:count]
    return home, [(hav(home, p), p) for p in near], hav


def project(places, w: float, h: float, pad_x: float = 0.70, pad_y: float = 0.60):
    """Rzutuje (lat, lon) na prostokat mapki — jak projectPoints w MatchScreen.kt."""
    import math

    lat_mid = sum(p["lat"] for p in places) / len(places)
    km_lat, km_lon = 110.574, 111.320 * math.cos(math.radians(lat_mid))
    xs = [p["lon"] * km_lon for p in places]
    ys = [p["lat"] * km_lat for p in places]
    min_x, max_x = min(xs), max(xs)
    min_y, max_y = min(ys), max(ys)
    span_x = max(max_x - min_x, 0.5)
    span_y = max(max_y - min_y, 0.5)
    scale = min(w * pad_x / span_x, h * pad_y / span_y)
    off_x = (w - span_x * scale) / 2
    off_y = (h - span_y * scale) / 2
    return [
        (off_x + (xs[i] - min_x) * scale, off_y + (max_y - ys[i]) * scale)
        for i in range(len(places))
    ]



def _dashed_line(c: Canvas, x0: float, y0: float, x1: float, y1: float, color) -> None:
    """Przerywana trasa gracz -> rywal (jak PathEffect.dashPathEffect)."""
    import math

    total = math.hypot(x1 - x0, y1 - y0)
    if total <= 0:
        return
    dash, gap = 12.0, 10.0
    pos = 0.0
    while pos < total:
        end = min(pos + dash, total)
        xa = x0 + (x1 - x0) * pos / total
        ya = y0 + (y1 - y0) * pos / total
        xb = x0 + (x1 - x0) * end / total
        yb = y0 + (y1 - y0) * end / total
        c.d.line([px(xa), px(ya), px(xb), px(yb)], fill=color, width=px(3))
        pos += dash + gap


def _map_label(c: Canvas, th, x: float, y: float, text: str) -> None:
    w = c.text_w(text, 11) + 12
    c.card(x, y, w, 18, 6, th["card"])
    c.text(x + 6, y + 2, text, 11, th["onHero"])


def _laurel(c: Canvas, th, cx: float, cy: float, size: float, plate) -> None:
    """Pieczec z laurkiem — ta sama geometria co LaurelCrest w Compose."""
    import math

    for side in (-1, 1):
        for i in range(7):
            deg = 30.0 + i * (130.0 / 6.0)
            rad = math.radians(deg)
            lx = cx + side * (size * 0.40) * math.sin(rad)
            ly = cy - (size * 0.40) * math.cos(rad)
            _leaf(c, lx, ly, size * 0.17, size * 0.075, side * deg, th["accent"])
    c.circle(cx, cy, size * 0.25, plate)
    c.text(0, cy - 8, I_SOCCER, 17, th["accent"], fa=True, center=cx)


def match_phone(dark: bool, round_no: int, decisive: bool) -> Image.Image:
    th = DARK if dark else LIGHT
    c = Canvas(W, H)  # przezroczysta plansza: tlo rysuje boisko
    pitch(c)
    scoreboard_bar(c, "Pogoń", "Świt")

    # karta zapowiedzi przyklejona do dolu (jak w MatchScreen)
    card_x, card_w = PAD, CW
    card_h = 262
    card_y = H - 16 - 54 - 12 - 22 - 12 - card_h
    c.card(card_x, card_y, card_w, card_h, 20, th["card"], th["cardBorder"])

    inner_x = card_x + 14
    inner_w = card_w - 28
    y = card_y + 14

    # nazwy klubow
    c.text(inner_x, y + 2, "Pogoń", 16, th["onHero"], bold=True)
    c.text(0, y + 2, "Świt", 16, th["onHero"], bold=True, right=inner_x + inner_w)
    c.text(0, y + 2, "–", 16, th["onHeroMuted"], center=card_x + card_w / 2)
    y += line_h(16) + 10

    # mapka: siatka, punkty ligi, trasa, etykiety, pieczec i odleglosc
    map_h = 158.0
    c.card(inner_x, y, inner_w, map_h, 16, th["mapBackground"])
    home, league, hav = load_league()
    places = [home] + [p for _, p in league]
    # Rywal z zapowiedzi: Świt Pułtusk (najdalszy rywal w tej lidze).
    opp_index = next(
        (i for i, (_, p) in enumerate(league) if p["name"] == "Pułtusk"), len(league) - 1
    )
    opponent = league[opp_index][1]
    distance = hav(home, opponent)
    pts = project(places, inner_w, map_h)
    # siatka
    step = min(inner_w, map_h) / 4.0
    gx = step
    while gx < inner_w:
        c.hline_v = None
        c.card(inner_x + gx, y, 0.6, map_h, 0, th["mapGrid"])
        gx += step
    gy = step
    while gy < map_h:
        c.card(inner_x, y + gy, inner_w, 0.6, 0, th["mapGrid"])
        gy += step
    px_home, py_home = pts[0]
    px_away, py_away = pts[1 + opp_index]
    _dashed_line(c, inner_x + px_home, y + py_home, inner_x + px_away, y + py_away, th["accent"])
    for i, (px_, py_) in enumerate(pts):
        if i == 0:
            c.circle(inner_x + px_, y + py_, 8, th["accent"])
            c.circle(inner_x + px_, y + py_, 3.5, th["mapBackground"])
        elif i == 1 + opp_index:
            c.circle(inner_x + px_, y + py_, 8, th["danger"])
            c.circle(inner_x + px_, y + py_, 3.5, th["mapBackground"])
        else:
            c.circle(inner_x + px_, y + py_, 3.5, blend(th["onHeroMuted"], th["mapBackground"], 0.5))
    _map_label(c, th, inner_x + px_home + 12, y + py_home - 26, home["name"])
    _map_label(c, th, inner_x + px_away + 12, y + py_away + 10, opponent["name"])
    _laurel(c, th, inner_x + 6 + 26, y + 6 + 26, 52.0, th["mapBackground"])
    # plakietka z odlegloscia
    dist = ("%.1f" % distance).replace(".", ",") + " km" if distance < 10 else "%d km" % round(distance)
    dw = c.text_w(dist, 12, bold=True) + 20
    c.card(inner_x + inner_w - 8 - dw, y + 8, dw, 26, 13, th["card"], th["cardBorder"])
    c.text(0, y + 13, dist, 12, th["accent"], bold=True, center=inner_x + inner_w - 8 - dw / 2)
    y += map_h + 10

    c.text(0, y, "Kolejka %d · B klasa · mazowieckie" % round_no, 11, th["onHeroMuted"],
           center=card_x + card_w / 2)
    if decisive:
        y += line_h(11) + 4
        c.text(0, y, "Mecz o awans", 11, th["accent"], bold=True, center=card_x + card_w / 2)

    # podpowiedz + przycisk startu
    hint_y = card_y + card_h + 10
    c.text(0, hint_y, "Przygotuj się na serię rzutów karnych", 12, (0xE6, 0xEE, 0xE6, 255),
           center=W / 2)
    btn_y = H - 16 - 54
    c.card(PAD, btn_y, CW, 54, 16, th["primary"])
    c.text(0, btn_y + 15, "Rozpocznij rzuty karne", 16, th["onPrimary"], bold=True, center=W / 2)
    return c.finish(radius=0)


def render_screen(out_path: str, dark: bool, round_no: int, decisive: bool,
                  height: float = 640.0, scale: int = 3) -> None:
    """Pojedynczy ekran bez ramki telefonu — format galerii na stronie."""
    import render_menu_mockup as kit

    kit.set_geometry(out_scale=scale, height=height)
    globals()["H"] = height
    globals()["HORIZON"] = 0.294 * height
    globals()["GOAL_TOP"] = height * 0.5875
    globals()["GOAL_BOTTOM"] = height * 0.7
    img = match_phone(dark=dark, round_no=round_no, decisive=decisive).copy()
    img.putalpha(255)
    final = img.resize(
        (int(round(kit.px(kit.W) * scale / 4)), int(round(kit.px(height) * scale / 4))),
        Image.LANCZOS,
    )
    final.convert("RGB").save(out_path)
    print("zapisano:", out_path, final.size)


def main(out_path: str) -> None:
    phones = [
        (match_phone(dark=True, round_no=3, decisive=False), "Ciemny motyw · 3. kolejka"),
        (match_phone(dark=False, round_no=8, decisive=True), "Jasny motyw · końcówka sezonu"),
    ]
    margin, gap = 44.0, 48.0
    title_h, caption_h = 104.0, 54.0
    page_w = margin * 2 + W * len(phones) + gap * (len(phones) - 1)
    page_h = title_h + H + caption_h
    page = Image.new("RGBA", (px(page_w), px(page_h)), PAGE_BG)

    for i, (img, caption) in enumerate(phones):
        x = margin + i * (W + gap)
        page.alpha_composite(img, (px(x), px(title_h)))
        d = ImageDraw.Draw(page)
        d.rounded_rectangle(
            [px(x - 1), px(title_h - 1), px(x + W), px(title_h + H)],
            radius=px(29), outline=(203, 211, 219, 255), width=px(1.5),
        )
        d.text((px(x + W / 2), px(title_h + H + 18)), caption,
               font=font(15, bold=True), fill=PAGE_FG, anchor="mt")

    d = ImageDraw.Draw(page)
    d.text((px(margin), px(18)), "B-Klasa Hero — zapowiedź meczu przed pierwszym rzutem",
           font=font(26, bold=True), fill=PAGE_FG)
    d.text((px(margin), px(50)),
           "Laurka, kluby oraz miejscowości z odległością między nimi (z rdzenia: playerTown, "
           "opponentTown, distanceKm).",
           font=font(14), fill=PAGE_MUTED)
    d.text((px(margin), px(72)),
           "Przykład: Pogoń Maków Mazowiecki kontra Świt Pułtusk — 17,9 km. "
           "Mockup układu z Compose, nie zrzut ekranu z urządzenia.",
           font=font(13), fill=PAGE_MUTED)

    final = page.resize((px(page_w) // 2, px(page_h) // 2), Image.LANCZOS)
    final.convert("RGB").save(out_path)
    print("zapisano:", out_path, final.size)


if __name__ == "__main__":
    if "--screen" in sys.argv:
        args = sys.argv[sys.argv.index("--screen") + 1:]
        out = args[0]
        height = float(args[args.index("--height") + 1]) if "--height" in args else 640.0
        render_screen(
            out,
            dark="--light" not in args,
            round_no=int(args[args.index("--round") + 1]) if "--round" in args else 3,
            decisive="--decisive" in args,
            height=height,
        )
    else:
        target = sys.argv[1] if len(sys.argv) > 1 else "docs/match-preview.png"
        main(target)
