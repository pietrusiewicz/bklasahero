#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Podglad menu glownego B-Klasa Hero (HomeScreen.kt) jako obrazek.

To NIE jest zrzut ekranu z urzadzenia, tylko mockup narysowany wg tych samych
wartosci, co uklad w Compose (odstepy, promienie, paleta z Theme.kt), zeby
obejrzec efekt bez budowania i instalowania APK.

Uruchomienie:
    python3 tools/preview/render_menu_mockup.py docs/menu-preview.png
"""
from __future__ import annotations

import sys
from PIL import Image, ImageDraw, ImageFont

# --- Skalowanie -------------------------------------------------------------
SS = 4   # nadprobykowanie (wygładzanie krawedzi)
OUT = 2  # skala wynikowa wzgledem "dp"

REG = "/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf"
BOLD = "/usr/share/fonts/liberation-sans/LiberationSans-Bold.ttf"
FA = "/usr/share/fonts/fontawesome/FontAwesome.otf"

I_SOCCER = "\uf1e3"
I_MOON = "\uf186"
I_REFRESH = "\uf01e"
I_HELP = "\uf059"

PAGE_BG = (238, 241, 244, 255)
PAGE_FG = (28, 36, 44, 255)
PAGE_MUTED = (108, 122, 134, 255)
WHITE = (255, 255, 255, 255)

DARK = dict(
    heroTop=(0x12, 0x33, 0x5C, 255),
    heroBottom=(0x0E, 0x2A, 0x1E, 255),
    card=(0x20, 0x2B, 0x3A, 0xE6),
    cardBorder=(255, 255, 255, 51),
    onHero=(255, 255, 255, 255),
    onHeroMuted=(0xB3, 0xC8, 0xD8, 255),
    accent=(0x2C, 0x8E, 0x3F, 255),
    danger=(0xFF, 0xB4, 0xAB, 255),
    primary=(0x2C, 0x8E, 0x3F, 255),
    onPrimary=(0xF7, 0xF7, 0xF2, 255),
    status=(255, 255, 255, 255),
    mapBackground=(0x16, 0x20, 0x2E, 255),
    mapGrid=(255, 255, 255, 51),
)

LIGHT = dict(
    heroTop=(0xDC, 0xEB, 0xDF, 255),
    heroBottom=(0xF7, 0xF7, 0xF2, 255),
    card=(255, 255, 255, 255),
    cardBorder=(0x14, 0x63, 0x2A, 0x24),
    onHero=(0x10, 0x28, 0x1B, 255),
    onHeroMuted=(0x4C, 0x6B, 0x58, 255),
    accent=(0x14, 0x63, 0x2A, 255),
    danger=(0xD3, 0x2F, 0x2F, 255),
    primary=(0x14, 0x63, 0x2A, 255),
    onPrimary=(0xF7, 0xF7, 0xF2, 255),
    status=(0x10, 0x28, 0x1B, 255),
    mapBackground=(0xE3, 0xED, 0xE5, 255),
    mapGrid=(0x14, 0x63, 0x2A, 31),
)

W, H = 360, 800
PAD = 20
CW = W - 2 * PAD
GAP = 12


def set_geometry(out_scale: int = 2, height: float = 800.0) -> None:
    """Ustawia skale wynikowa i wysokosc ekranu (do zrzutow w innym formacie)."""
    global OUT, H
    OUT = out_scale
    H = height


def px(v: float) -> int:
    return int(round(v * SS))


def font(size: float, bold: bool = False, fa: bool = False) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(FA if fa else (BOLD if bold else REG), px(size))


def line_h(size: float) -> float:
    return size * 1.28


class Canvas:
    """Plansza jednego ekranu: tlo + warstwy kart + tekst."""

    def __init__(self, w: float, h: float, top=None, bottom=None):
        self.w, self.h = w, h
        self.img = Image.new("RGBA", (px(w), px(h)), (0, 0, 0, 0))
        if top is not None:
            self.img.alpha_composite(self._gradient(top, bottom))

    def _gradient(self, top, bottom) -> Image.Image:
        strip = Image.new("RGBA", (1, px(self.h)))
        sd = ImageDraw.Draw(strip)
        for y in range(px(self.h)):
            t = y / max(1, px(self.h) - 1)
            sd.point((0, y), tuple(
                int(round(top[i] + (bottom[i] - top[i]) * t)) for i in range(4)
            ))
        return strip.resize((px(self.w), px(self.h)))

    # -- ksztalty ------------------------------------------------------------
    def card(self, x, y, w, h, r, fill, border=None, border_w=1.0):
        layer = Image.new("RGBA", self.img.size, (0, 0, 0, 0))
        ImageDraw.Draw(layer).rounded_rectangle(
            [px(x), px(y), px(x + w), px(y + h)],
            radius=px(r),
            fill=fill,
            outline=border,
            width=px(border_w) if border else 0,
        )
        self.img.alpha_composite(layer)

    def circle(self, cx, cy, r, fill):
        layer = Image.new("RGBA", self.img.size, (0, 0, 0, 0))
        ImageDraw.Draw(layer).ellipse(
            [px(cx - r), px(cy - r), px(cx + r), px(cy + r)], fill=fill
        )
        self.img.alpha_composite(layer)

    def bar(self, x, y, w, h, fill):
        layer = Image.new("RGBA", self.img.size, (0, 0, 0, 0))
        ImageDraw.Draw(layer).rounded_rectangle(
            [px(x), px(y), px(x + w), px(y + h)], radius=px(h / 2), fill=fill
        )
        self.img.alpha_composite(layer)

    def hline(self, x0, x1, y, color):
        layer = Image.new("RGBA", self.img.size, (0, 0, 0, 0))
        ImageDraw.Draw(layer).rectangle([px(x0), px(y), px(x1), px(y + 1)], fill=color)
        self.img.alpha_composite(layer)

    # -- tekst ---------------------------------------------------------------
    def text(self, x, y, s, size, color, bold=False, fa=False, center=None, right=None):
        f = font(size, bold=bold, fa=fa)
        if center is not None:
            self.d.text((px(center), px(y)), s, font=f, fill=color, anchor="mt")
        elif right is not None:
            self.d.text((px(right), px(y)), s, font=f, fill=color, anchor="rt")
        else:
            self.d.text((px(x), px(y)), s, font=f, fill=color, anchor="lt")

    def wrap_limited(self, s, size, max_w, max_lines, bold=False):
        """Zawija tekst do `max_lines` linii; nadmiar ucina wielokropkiem."""
        words = s.split()
        lines, cur = [], ""
        for i, word in enumerate(words):
            probe = (cur + " " + word).strip()
            if self.text_w(probe, size, bold) <= max_w or not cur:
                cur = probe
            else:
                lines.append(cur)
                cur = word
                if len(lines) == max_lines:
                    break
        if cur and len(lines) < max_lines:
            lines.append(cur)
        used = sum(len(l.split()) for l in lines)
        if used < len(words) and lines:
            lines[-1] = self.ellipsize(lines[-1] + " " + words[used], size, max_w, bold)
        return lines

    def ellipsize(self, s, size, max_w, bold=False) -> str:
        """Skraca tekst z wielokropkiem — odpowiednik TextOverflow.Ellipsis."""
        if self.text_w(s, size, bold) <= max_w:
            return s
        cut = s
        while cut and self.text_w(cut + "…", size, bold) > max_w:
            cut = cut[:-1]
        return cut + "…"

    def text_w(self, s, size, bold=False, fa=False) -> float:
        return font(size, bold=bold, fa=fa).getlength(s) / SS

    @property
    def d(self):
        return ImageDraw.Draw(self.img)

    def finish(self, radius=28.0):
        mask = Image.new("L", self.img.size, 0)
        ImageDraw.Draw(mask).rounded_rectangle(
            [0, 0, self.img.size[0] - 1, self.img.size[1] - 1],
            radius=px(radius), fill=255,
        )
        self.img.putalpha(mask)
        return self.img


# --- Dane do podgladu -------------------------------------------------------

TABLE_ROWS = [
    (1, "Orzeł Węgrzynowo", "12:4", 16, False),
    (2, "Sokół Karniewo", "11:5", 13, False),
    (3, "Iskra Szczuki", "10:7", 11, False),
    (4, "Pogoń Maków Mazowiecki", "9:7", 10, True),
    (5, "Grom Krasne", "8:8", 8, False),
    (6, "Znicz Zambski Kościelne", "7:8", 7, False),
    (7, "Start Dobrzankowo", "6:9", 6, False),
    (8, "Bór Gołymin-Ośrodek", "5:10", 4, False),
    (9, "Wisła Leszno", "4:11", 3, False),
    (10, "Świt Pułtusk", "3:12", 2, False),
]

# Miejscowości to prawdziwi sąsiedzi Makowa Mazowieckiego z katalogu OSM
# (8–18 km): liga B klasy składa się z okolicy, a nie z całego kraju.
FIXTURES = [
    (1, [("Orzeł Węgrzynowo", "Sokół Karniewo", 2, 1),
         ("Iskra Szczuki", "Grom Krasne", 1, 1),
         ("Pogoń Maków Mazowiecki", "Świt Pułtusk", 3, 0),
         ("Znicz Zambski Kościelne", "Bór Gołymin-Ośrodek", 0, 2),
         ("Start Dobrzankowo", "Wisła Leszno", 2, 2)]),
    (2, [("Sokół Karniewo", "Iskra Szczuki", 1, 0),
         ("Grom Krasne", "Pogoń Maków Mazowiecki", 2, 2),
         ("Świt Pułtusk", "Znicz Zambski Kościelne", 1, 1),
         ("Bór Gołymin-Ośrodek", "Start Dobrzankowo", 3, 1),
         ("Wisła Leszno", "Orzeł Węgrzynowo", 0, 1)]),
    (3, [("Pogoń Maków Mazowiecki", "Orzeł Węgrzynowo", None, None),
         ("Sokół Karniewo", "Znicz Zambski Kościelne", None, None),
         ("Iskra Szczuki", "Bór Gołymin-Ośrodek", None, None),
         ("Grom Krasne", "Start Dobrzankowo", None, None),
         ("Świt Pułtusk", "Wisła Leszno", None, None)]),
]

PLAYER_MARK = "Pogoń"


# --- Ekran: menu glowne -----------------------------------------------------

def status_bar(c: Canvas, th, chrome: bool = True):
    if not chrome:
        return 24.0  # miejsce na pasek systemowy zostaje, sam pasek rysujemy tylko w mockupie
    c.text(PAD, 6, "20:15", 11, th["status"], bold=True)
    bx = W - PAD - 22
    c.d.rounded_rectangle(
        [px(bx), px(9), px(bx + 22), px(19)], radius=px(3),
        outline=th["status"], width=max(1, px(1.2)),
    )
    c.d.rectangle([px(bx + 22), px(12), px(bx + 24), px(16)], fill=th["status"])
    c.d.rectangle([px(bx + 3), px(12), px(bx + 17), px(16)], fill=th["status"])
    for i in range(3):
        x = bx - 14 - i * 5
        hgt = 4 + i * 2
        c.d.rectangle([px(x), px(19 - hgt), px(x + 3), px(19)], fill=th["status"])
    return 24.0


def round_button(c: Canvas, th, x, y, icon, size=42):
    c.card(x, y, size, size, size / 2, th["card"], th["cardBorder"], 1.0)
    c.text(0, y + size / 2 - 11, icon, 20, th["onHero"], fa=True, center=x + size / 2)


def header(c: Canvas, th, y):
    c.circle(PAD + 24, y + 24, 24, th["accent"])
    c.text(PAD + 13, y + 11, I_SOCCER, 28, WHITE, fa=True)
    c.text(PAD + 62, y - 1, "B-Klasa Hero", 22, th["onHero"], bold=True)
    c.text(PAD + 62, y + 28, "Bohater Niedzielnej Ligi", 12, th["onHeroMuted"])
    right = PAD + CW - 42
    round_button(c, th, right, y + 3, I_MOON)
    round_button(c, th, right - 50, y + 3, I_HELP)
    return y + 48


def career_hero(c: Canvas, th, y):
    h = 14 + max(42, 3 * line_h(12) + 4) + 14
    c.card(PAD, y, CW, h, 18, th["card"], th["cardBorder"])
    cy = y + 14
    c.circle(PAD + 14 + 21, cy + 21, 21, th["accent"])
    c.text(0, cy + 12, "P", 16, WHITE, bold=True, center=PAD + 14 + 21)
    tx = PAD + 14 + 42 + 12
    c.text(tx, cy - 1, "Witaj, Piotrek", 16, th["onHero"], bold=True)
    c.text(tx, cy + line_h(16), "B klasa · mazowieckie · Sezon 2", 12, th["onHeroMuted"])
    c.text(tx, cy + line_h(16) + line_h(12), "Maków Mazowiecki", 12, th["onHeroMuted"])
    return y + h


def big_button(c: Canvas, th, y, label, icon=None, h=58, label_size=22):
    c.card(PAD, y, CW, h, 18, th["primary"])
    tw = c.text_w(label, label_size, bold=True)
    if icon:
        iw = c.text_w(icon, label_size + 4, fa=True)
        sx = PAD + (CW - (iw + 10 + tw)) / 2
        c.text(sx, y + (h - label_size * 1.3) / 2, icon, label_size + 4, th["onPrimary"], fa=True)
        c.text(sx + iw + 10, y + (h - label_size * 1.3) / 2, label, label_size,
               th["onPrimary"], bold=True)
    else:
        c.text(0, y + (h - label_size * 1.3) / 2, label, label_size,
               th["onPrimary"], bold=True, center=PAD + CW / 2)
    return y + h


def tab_row(c: Canvas, th, y, active):
    h = 48
    labels = ("Tabela", "Terminarz", "Kariera")
    seg = CW / 3
    for i, label in enumerate(labels):
        sel = i == active
        c.text(0, y + 15, label, 14,
               th["onHero"] if sel else th["onHeroMuted"],
               bold=sel, center=PAD + seg * i + seg / 2)
    c.bar(PAD + seg * active + 12, y + h - 3, seg - 24, 3, th["accent"])
    return y + h


def page_table(c: Canvas, th, y):
    c.text(PAD + 4, y, "B klasa · mazowieckie", 12, th["onHeroMuted"], bold=True)
    y += line_h(12) + 10
    row_h = 32
    c.card(PAD, y, CW, 34 + row_h * len(TABLE_ROWS), 18, th["card"], th["cardBorder"])
    inner, iw = PAD + 14, CW - 28
    hy = y + 10
    c.text(inner, hy, "#", 11, th["onHeroMuted"])
    c.text(inner + iw * 0.14, hy, "Drużyna", 11, th["onHeroMuted"])
    c.text(0, hy, "Bramki", 11, th["onHeroMuted"], center=inner + iw * 0.70)
    c.text(0, hy, "Pkt", 11, th["onHeroMuted"], right=inner + iw)
    c.hline(PAD, PAD + CW, y + 32, th["cardBorder"])

    for i, (pos, name, goals, pts, is_player) in enumerate(TABLE_ROWS):
        ry = y + 34 + i * row_h
        if is_player:
            a = th["accent"]
            c.card(PAD + 8, ry + 2, CW - 16, row_h - 4, 10, (a[0], a[1], a[2], 41))
        ty = ry + 8
        c.d.text((px(inner), px(ty)), str(pos),
                 font=font(14, bold=is_player),
                 fill=th["onHero"] if is_player else th["onHeroMuted"], anchor="lt")
        c.d.text((px(inner + iw * 0.14), px(ty + 1)),
                 c.ellipsize(name, 12, iw * 0.46 - 8, bold=is_player),
                 font=font(12, bold=is_player), fill=th["onHero"], anchor="lt")
        c.d.text((px(inner + iw * 0.70), px(ty)), goals,
                 font=font(14), fill=th["onHeroMuted"], anchor="mt")
        c.d.text((px(inner + iw), px(ty)), str(pts),
                 font=font(14, bold=True), fill=th["onHero"], anchor="rt")


def page_fixtures(c: Canvas, th, y):
    row_h = 34  # miejsce na dwie linie nazwy (maxLines = 2 w Compose)
    for round_no, matches in FIXTURES:
        h = 12 + line_h(12) + 6 + len(matches) * row_h + 10
        c.card(PAD, y, CW, h, 18, th["card"], th["cardBorder"])
        c.text(PAD + 14, y + 12, "Kolejka %d" % round_no, 12, th["onHeroMuted"], bold=True)
        my = y + 12 + line_h(12) + 6
        # Kolumny jak w FixturesView: nazwa | 48 dp na wynik | nazwa.
        inner, iw = PAD + 14, CW - 28
        score_cx = inner + iw / 2
        name_w = (iw - 48) / 2 - 4
        for home, away, hg, ag in matches:
            is_player = PLAYER_MARK in (home, away)
            col = th["onHero"] if is_player else th["onHeroMuted"]
            for side, text in (("home", home), ("away", away)):
                lines = c.wrap_limited(text, 12, name_w, 2)
                for i, line in enumerate(lines):
                    yy = my + 5 + i * line_h(12)
                    if side == "home":
                        c.d.text((px(inner), px(yy)), line, font=font(12, bold=is_player),
                                 fill=col, anchor="lt")
                    else:
                        c.d.text((px(inner + iw), px(yy)), line, font=font(12, bold=is_player),
                                 fill=col, anchor="rt")
            score = "—" if hg is None else "%d:%d" % (hg, ag)
            c.d.text((px(score_cx), px(my + 9)), score, font=font(14, bold=True),
                     fill=th["accent"] if is_player else th["onHero"], anchor="mt")
            my += row_h
        y += h + 10


def page_career(c: Canvas, th, y):
    c.card(PAD, y, CW, 58, 18, th["card"], th["cardBorder"])
    c.text(PAD + 14, y + 14, "Poziom 3 · 180/400 XP", 14, th["onHero"], bold=True)
    c.bar(PAD + 14, y + 40, CW - 28, 4, th["cardBorder"])
    c.bar(PAD + 14, y + 40, (CW - 28) * 0.45, 4, th["accent"])
    y += 58 + 10

    stats = (("12", "Mecze"), ("8", "Wygrane"), ("34", "Gole"), ("41", "Obrony"))
    tw = (CW - 10) / 2
    for row in range(2):
        for col in range(2):
            value, label = stats[row * 2 + col]
            x = PAD + col * (tw + 10)
            c.card(x, y, tw, 74, 18, th["card"], th["cardBorder"])
            c.text(x + 14, y + 14, value, 22, th["onHero"], bold=True)
            c.text(x + 14, y + 14 + line_h(22) + 4, label, 12, th["onHeroMuted"])
        y += 74 + 10

    c.card(PAD, y, CW, 130, 18, th["card"], th["cardBorder"])
    c.card(PAD + 14, y + 14, CW - 28, 46, 14, (0, 0, 0, 0), th["accent"], 1.4)
    c.text(0, y + 29, "Zapisz", 14, th["accent"], bold=True, center=PAD + CW / 2)
    c.hline(PAD, PAD + CW, y + 74, th["cardBorder"])
    c.text(PAD + 16, y + 88, I_REFRESH, 24, th["danger"], fa=True)
    c.text(PAD + 16 + 38, y + 86, "Nowa kariera", 14, th["danger"], bold=True)
    c.text(PAD + 16 + 38, y + 86 + line_h(14), "Usuwa obecny postęp", 12, th["onHeroMuted"])
    c.text(PAD + CW - 26, y + 88, "›", 20, th["danger"])


def menu_phone(dark: bool, tab: int, chrome: bool = True, scroll: float = 0.0) -> Image.Image:
    th = DARK if dark else LIGHT
    c = Canvas(W, H, th["heroTop"], th["heroBottom"])
    y = status_bar(c, th, chrome) + 12
    y = header(c, th, y) + 14
    y = career_hero(c, th, y) + 6
    y = tab_row(c, th, y, tab)

    # „Graj” przyklejone do dolu ekranu, nad stopka.
    footer_y = H - 34
    button_y = footer_y - 12 - 58
    page_y = y + 12
    page_h = button_y - 10 - page_y
    layer = Canvas(W, page_h)  # przezroczysta warstwa = obszar pagera
    # `scroll` przesuwa treść zakładki w górę — jak przewinięty ekran na telefonie.
    if tab == 0:
        page_table(layer, th, -scroll)
    elif tab == 1:
        page_fixtures(layer, th, -scroll)
    else:
        page_career(layer, th, -scroll)
    c.img.alpha_composite(layer.img, (0, px(page_y)))

    big_button(c, th, button_y, "Graj", icon=I_SOCCER)
    c.text(0, footer_y, "Wersja 0.1.0 · protokół v1", 12, th["onHeroMuted"], center=W / 2)
    return c.finish()


# --- Strona -----------------------------------------------------------------

def main(out_path: str) -> None:
    phones = [
        (menu_phone(dark=True, tab=0), "Ciemny motyw · zakładka Tabela"),
        (menu_phone(dark=False, tab=1), "Jasny motyw · zakładka Terminarz"),
        (menu_phone(dark=False, tab=2), "Jasny motyw · zakładka Kariera"),
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
    d.text((px(margin), px(18)), "B-Klasa Hero — menu główne z przesuwanymi zakładkami",
           font=font(26, bold=True), fill=PAGE_FG)
    d.text((px(margin), px(50)),
           "Przesuń palcem: Tabela ↔ Terminarz ↔ Kariera. Motyw i pomoc siedzą w nagłówku, akcja „Graj” na dole.",
           font=font(14), fill=PAGE_MUTED)
    d.text((px(margin), px(72)),
           "Kluby to prawdziwi sąsiedzi Makowa Mazowieckiego z katalogu OSM (8–18 km). "
           "Mockup układu z Compose — nie zrzut ekranu z urządzenia.",
           font=font(13), fill=PAGE_MUTED)

    final = page.resize(
        (int(round(px(page_w) * OUT / SS)), int(round(px(page_h) * OUT / SS))), Image.LANCZOS
    )
    final.convert("RGB").save(out_path)
    print("zapisano:", out_path, final.size)


def render_screen(out_path: str, dark: bool, tab: int, height: float = 640.0,
                  scale: int = 3, scroll: float = 0.0) -> None:
    """Pojedynczy ekran bez ramki telefonu — format galerii na stronie (np. 1080x1920)."""
    set_geometry(out_scale=scale, height=height)
    img = menu_phone(dark=dark, tab=tab, chrome=False, scroll=scroll).copy()
    # Zrzut na strone to pelny prostokat — bez zaokraglen ramki telefonu.
    img.putalpha(255)
    final = img.resize(
        (int(round(px(W) * scale / SS)), int(round(px(height) * scale / SS))), Image.LANCZOS
    )
    final.convert("RGB").save(out_path)
    print("zapisano:", out_path, final.size)


if __name__ == "__main__":
    if "--screen" in sys.argv:
        # python3 render_menu_mockup.py --screen out.png [--light] [--tab N] [--height DP]
        args = sys.argv[sys.argv.index("--screen") + 1:]
        out = args[0]
        tab = int(args[args.index("--tab") + 1]) if "--tab" in args else 0
        height = float(args[args.index("--height") + 1]) if "--height" in args else 640.0
        scroll = float(args[args.index("--scroll") + 1]) if "--scroll" in args else 0.0
        render_screen(out, dark="--light" not in args, tab=tab, height=height, scroll=scroll)
    else:
        target = sys.argv[1] if len(sys.argv) > 1 else "docs/menu-preview.png"
        main(target)
