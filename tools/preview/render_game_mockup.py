#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Podglady ekranow ROZGRYWKI B-Klasa Hero (ciemny motyw, 1080x1920).

Odtwarzaja to, co rysuje aplikacja: boisko (MatchRenderer), sylwetki zawodnikow
(Figures.kt — proporcje przeniesione 1:1), plansze wynikow oraz ekrany
strzalu, gola, obrony i samouczka. To mockupy ukladu z Compose, nie zrzuty
z urzadzenia.

Uruchomienie:
    python3 tools/preview/render_game_mockup.py docs/game-shots/
"""
from __future__ import annotations

import math
import os
import sys

from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from render_menu_mockup import (  # noqa: E402
    DARK,
    WHITE,
    Canvas,
    font,
    line_h,
    px,
)

# --- Geometria sceny (z MatchRenderer.kt) -----------------------------------
H = 640.0          # wysokosc ekranu w dp (1080x1920 @3x)
W = 360.0
HORIZON_Y = 0.26
GOAL_TOP = 0.30
GOAL_LINE_Y = 0.52
GOAL_LEFT = 0.19
GOAL_RIGHT = 0.81
SHOOTER_Y = 0.90
BOARD_H = 63.0     # pasek wyniku (ScoreboardBar)

SKY_TOP = (0x12, 0x33, 0x5C)
SKY_BOTTOM = (0x7F, 0xA8, 0xD9)
STANDS = (0x24, 0x30, 0x3E)
CROWD_A = (0x2E, 0x3C, 0x4E)
CROWD_B = (0x1B, 0x25, 0x30)
GRASS = (0x2F, 0x8B, 0x3B)
GRASS_LIGHT = (0x3A, 0xA2, 0x4A)
GRASS_DARK = (0x28, 0x7C, 0x34)
LINE = (0xF2, 0xF6, 0xF2)
NET = (0xE8, 0xEC, 0xE8)
POST = (0xFA, 0xFA, 0xFA)
BOARD_BG = (0x16, 0x20, 0x2E, 0xCC)
DOT_EMPTY = (0x42, 0x52, 0x6B, 255)
DOT_GOAL = (0x66, 0xBB, 0x6A, 255)
DOT_MISS = (0xE5, 0x48, 0x4D, 255)

SHOOTER_KIT = dict(
    jersey=(0xD9, 0x34, 0x2B, 255),
    shorts=(0x20, 0x24, 0x2A, 255),
    socks=(0xD9, 0x34, 0x2B, 255),
    boots=(0x16, 0x18, 0x1C, 255),
    skin=(0xE8, 0xB4, 0x8C, 255),
    hair=(0x2B, 0x21, 0x18, 255),
)
KEEPER_KIT = dict(
    jersey=(0xF4, 0xB4, 0x00, 255),
    shorts=(0x20, 0x24, 0x2A, 255),
    socks=(0x20, 0x24, 0x2A, 255),
    boots=(0x16, 0x18, 0x1C, 255),
    skin=(0xE8, 0xB4, 0x8C, 255),
    hair=(0x2B, 0x21, 0x18, 255),
)
GLOVE = (0x3A, 0x3F, 0x45, 255)
SHADE = (0, 0, 0, 0x26)


# --- Pomocnicze rysowanie ---------------------------------------------------

def thick_line(c: Canvas, color, x0, y0, x1, y1, width):
    """Linia z zaokraglonymi koncami (odpowiednik StrokeCap.Round)."""
    c.d.line([px(x0), px(y0), px(x1), px(y1)], fill=color, width=max(1, px(width)))
    r = width / 2
    c.circle(x0, y0, r, color)
    c.circle(x1, y1, r, color)


def rrect(c: Canvas, color, x, y, w, h, r):
    c.card(x, y, w, h, r, color)


def poly(c: Canvas, color, points):
    c.d.polygon([(px(x), px(y)) for x, y in points], fill=color)


def vertical_gradient_image(top, bottom, height_px, width_px):
    strip = Image.new("RGBA", (1, height_px))
    sd = ImageDraw.Draw(strip)
    for y in range(height_px):
        t = y / max(1, height_px - 1)
        sd.point((0, y), tuple(int(round(top[i] + (bottom[i] - top[i]) * t)) for i in range(3)) + (255,))
    return strip.resize((width_px, height_px))


# --- Boisko -----------------------------------------------------------------

def draw_scene(c: Canvas) -> None:
    horizon = H * HORIZON_Y
    sky_top = BOARD_H + 2
    sky_h = int(px(horizon - sky_top))
    c.img.alpha_composite(
        vertical_gradient_image(SKY_TOP, SKY_BOTTOM, sky_h, int(px(W))), (0, px(sky_top))
    )
    # trybuny z tlumem
    c.d.rectangle([0, px(horizon - H * 0.07), px(W), px(horizon)], fill=STANDS)
    cols = 26
    for i in range(cols):
        x = i * (W / cols)
        c.d.rectangle(
            [px(x + 2), px(horizon - H * 0.062), px(x + 11), px(horizon - 3)],
            fill=CROWD_A if i % 2 == 0 else CROWD_B,
        )
    # murawa z pasami
    c.d.rectangle([0, px(horizon), px(W), px(H)], fill=GRASS)
    bands = [(0.000, 0.037, GRASS_DARK), (0.037, 0.160, GRASS_LIGHT), (0.160, 0.176, GRASS_DARK),
             (0.176, 0.300, GRASS_LIGHT), (0.300, 0.318, GRASS_DARK), (0.318, 0.470, GRASS_LIGHT),
             (0.470, 0.489, GRASS_DARK), (0.489, 0.660, GRASS_LIGHT), (0.660, 0.700, GRASS_DARK),
             (0.700, 1.000, GRASS_LIGHT)]
    for a, b, color in bands:
        c.d.rectangle(
            [0, px(horizon + (H - horizon) * a), px(W), px(horizon + (H - horizon) * b)],
            fill=color,
        )
    # linie: pole karne i linia bramkowa
    c.d.line([px(W * 0.07), px(H * 0.72), px(W * 0.07), px(H)], fill=LINE, width=px(2))
    c.d.line([px(W * 0.93), px(H * 0.72), px(W * 0.93), px(H)], fill=LINE, width=px(2))
    c.d.line([px(W * 0.07), px(H * 0.72), px(W * 0.93), px(H * 0.72)], fill=LINE, width=px(2))
    c.d.line([px(0), px(H * GOAL_LINE_Y), px(W), px(H * GOAL_LINE_Y)], fill=LINE, width=px(2.5))


def draw_goal(c: Canvas, with_net: bool = True) -> None:
    left, right = W * GOAL_LEFT, W * GOAL_RIGHT
    top, bottom = H * GOAL_TOP, H * GOAL_LINE_Y
    if with_net:
        c.card(left, top, right - left, bottom - top, 0, (0xFF, 0xFF, 0xFF, 0x1A))
        for i in range(1, 9):
            x = left + (right - left) * i / 9.0
            c.d.line([px(x), px(top), px(x), px(bottom)], fill=(0xE8, 0xEC, 0xE8, 0x88), width=px(1))
        for i in range(1, 5):
            y = top + (bottom - top) * i / 5.0
            c.d.line([px(left), px(y), px(right), px(y)], fill=(0xE8, 0xEC, 0xE8, 0x88), width=px(1))
    for x in (left, right):
        c.d.rectangle([px(x - 1.5), px(top), px(x + 1.5), px(bottom)], fill=POST)
    c.d.rectangle([px(left - 2), px(top - 2), px(right + 2), px(top + 2)], fill=POST)


def draw_ball(c: Canvas, x, y, r=7.0) -> None:
    c.circle(x, y, r, (0xF7, 0xF7, 0xF2, 255))


# --- Sylwetki (port Figures.kt) --------------------------------------------

def draw_player_back(c: Canvas, center_x, feet_y, height, kit) -> None:
    h = height
    cx = center_x
    ankle_y = feet_y - 0.055 * h
    knee_y = feet_y - 0.28 * h
    hip_y = feet_y - 0.47 * h
    waist_y = feet_y - 0.60 * h
    shoulder_y = feet_y - 0.79 * h
    head_r = 0.072 * h
    head_cy = feet_y - 0.895 * h
    leg_x = 0.062 * h
    hip_x = 0.052 * h
    shoulder_half = 0.135 * h
    arm_w = 0.055 * h

    for side in (-1, 1):
        hip = (cx + side * hip_x, hip_y)
        knee = (cx + side * leg_x, knee_y)
        ankle = (cx + side * leg_x, ankle_y)
        thick_line(c, kit["skin"], hip[0], hip[1], knee[0], knee[1], 0.088 * h)
        thick_line(c, kit["skin"], knee[0], knee[1], ankle[0], ankle[1], 0.062 * h)
        thick_line(c, kit["socks"], knee[0], knee[1] + 0.02 * h, ankle[0], ankle[1], 0.068 * h)
        rrect(c, kit["boots"], ankle[0] - 0.058 * h, ankle[1] - 0.010 * h, 0.116 * h, 0.046 * h, 0.020 * h)

    rrect(c, kit["shorts"], cx - 0.105 * h, waist_y - 0.005 * h, 0.210 * h, 0.165 * h, 0.035 * h)
    poly(c, kit["jersey"], [
        (cx - shoulder_half, shoulder_y), (cx + shoulder_half, shoulder_y),
        (cx + 0.108 * h, waist_y), (cx - 0.108 * h, waist_y),
    ])
    poly(c, SHADE, [
        (cx - shoulder_half, shoulder_y), (cx - 0.035 * h, shoulder_y),
        (cx - 0.035 * h, waist_y), (cx - 0.108 * h, waist_y),
    ])
    for s in (-1, 1):
        sx = cx - shoulder_half - 0.030 * h if s < 0 else cx + shoulder_half - 0.015 * h
        rrect(c, kit["jersey"], sx, shoulder_y - 0.005 * h, 0.045 * h, 0.085 * h, 0.018 * h)

    for s in (-1, 1):
        shoulder = (cx + s * (shoulder_half - 0.012 * h), shoulder_y + 0.018 * h)
        elbow = (cx + s * 0.168 * h, waist_y - 0.035 * h)
        hand = (cx + s * 0.170 * h, hip_y - 0.010 * h)
        thick_line(c, kit["jersey"], shoulder[0], shoulder[1], elbow[0], elbow[1], arm_w)
        thick_line(c, kit["skin"], elbow[0], elbow[1], hand[0], hand[1], 0.044 * h)
        c.circle(hand[0], hand[1], 0.026 * h, kit["skin"])

    rrect(c, kit["skin"], cx - 0.026 * h, shoulder_y - 0.055 * h, 0.052 * h, 0.06 * h, 0.018 * h)
    c.circle(cx, head_cy, head_r, kit["skin"])
    c.circle(cx, head_cy - head_r * 0.20, head_r * 0.99, kit["hair"])


def draw_keeper(c: Canvas, feet_x, feet_y, height, kit, direction, progress) -> None:
    """Bramkarz: przy progress=0 stoi, przy 1 lezy w nurkowaniu (obrot 82°)."""
    layer = Canvas(W, H)  # przezroczysta warstwa — obracamy cala sylwetke
    h = height
    cx = feet_x
    ankle_y = feet_y - 0.055 * h
    knee_y = feet_y - 0.28 * h
    hip_y = feet_y - 0.46 * h
    waist_y = feet_y - 0.58 * h
    shoulder_y = feet_y - 0.76 * h
    head_r = 0.072 * h
    head_cy = feet_y - 0.865 * h
    leg_x = 0.055 * h

    for s in (-1, 1):
        hip = (cx + s * 0.045 * h, hip_y)
        knee = (cx + s * leg_x, knee_y)
        ankle = (cx + s * leg_x * 0.95, ankle_y)
        thick_line(layer, kit["skin"], hip[0], hip[1], knee[0], knee[1], 0.085 * h)
        thick_line(layer, kit["skin"], knee[0], knee[1], ankle[0], ankle[1], 0.060 * h)
        thick_line(layer, kit["socks"], knee[0], knee[1] + 0.02 * h, ankle[0], ankle[1], 0.066 * h)
        rrect(layer, kit["boots"], ankle[0] - 0.055 * h, ankle[1] - 0.010 * h, 0.110 * h, 0.044 * h, 0.018 * h)

    rrect(layer, kit["shorts"], cx - 0.100 * h, waist_y - 0.005 * h, 0.200 * h, 0.155 * h, 0.033 * h)
    shoulder_half = 0.130 * h
    poly(layer, kit["jersey"], [
        (cx - shoulder_half, shoulder_y), (cx + shoulder_half, shoulder_y),
        (cx + 0.104 * h, waist_y), (cx - 0.104 * h, waist_y),
    ])
    poly(layer, SHADE, [
        (cx - shoulder_half, shoulder_y), (cx - 0.032 * h, shoulder_y),
        (cx - 0.032 * h, waist_y), (cx - 0.104 * h, waist_y),
    ])
    for s in (-1, 1):
        sx = cx - shoulder_half - 0.028 * h if s < 0 else cx + shoulder_half - 0.014 * h
        rrect(layer, kit["jersey"], sx, shoulder_y - 0.005 * h, 0.043 * h, 0.082 * h, 0.017 * h)

    reach = 1.02 + 0.10 * progress
    for s in (-1, 1):
        shoulder = (cx + s * (shoulder_half - 0.012 * h), shoulder_y + 0.015 * h)
        elbow_up = (cx + s * 0.105 * h, feet_y - 0.92 * h)
        hand = (cx + s * 0.115 * h, feet_y - reach * h)
        thick_line(layer, kit["jersey"], shoulder[0], shoulder[1], elbow_up[0], elbow_up[1], 0.052 * h)
        thick_line(layer, kit["skin"], elbow_up[0], elbow_up[1], hand[0], hand[1], 0.044 * h)
        layer.circle(hand[0], hand[1], 0.032 * h, GLOVE)

    rrect(layer, kit["skin"], cx - 0.026 * h, shoulder_y - 0.055 * h, 0.052 * h, 0.06 * h, 0.018 * h)
    layer.circle(cx, head_cy, head_r, kit["skin"])
    # wlosy tylko z gory (widzimy twarz)
    layer.d.pieslice(
        [px(cx - head_r), px(head_cy - head_r * 1.05), px(cx + head_r), px(head_cy + head_r * 0.95)],
        start=180, end=360, fill=kit["hair"],
    )

    pivot = (px(cx), px(feet_y - 0.45 * h))
    rotated = layer.img.rotate(
        -direction * 82.0 * max(0.0, min(1.0, progress)), center=pivot, resample=Image.BICUBIC
    )
    c.img.alpha_composite(rotated)


# --- Plansza wyniku ---------------------------------------------------------

def draw_board(c: Canvas, home, away, score, taken, dots_home, dots_away, round_text) -> None:
    c.card(0, 0, W, BOARD_H, 0, BOARD_BG)
    c.d.rounded_rectangle(
        [0, px(BOARD_H - 14), px(W), px(BOARD_H + 10)], radius=px(14), fill=BOARD_BG
    )
    c.text(16, 10, home, 16, WHITE, bold=True)
    c.text(0, 10, away, 16, WHITE, bold=True, right=W - 16)
    c.text(0, 6, score, 26, WHITE, bold=True, center=W / 2)
    for i in range(5):
        color = DOT_EMPTY
        if i < len(dots_home):
            color = DOT_GOAL if dots_home[i] else DOT_MISS
        c.circle(20 + i * 12 + 3, 38, 4, color)
        color = DOT_EMPTY
        if i < len(dots_away):
            color = DOT_GOAL if dots_away[i] else DOT_MISS
        c.circle(W - 20 - i * 12 - 3, 38, 4, color)
    c.text(0, 48, round_text, 11, (0xC7, 0xD3, 0xE0, 255), center=W / 2)


def draw_hint(c: Canvas, text, top=92.0, width=W - 48) -> None:
    """Podpowiedz na gorze ekranu (ShootoutScreen/DefendScreen: padding top 92dp)."""
    lines = wrap_text(c, text, 14, width)
    y = top
    for line in lines:
        c.text(0, y, line, 14, WHITE, center=W / 2)
        y += line_h(14)


def wrap_text(c: Canvas, text, size, max_w):
    words, lines, cur = text.split(), [], ""
    for word in words:
        probe = (cur + " " + word).strip()
        if c.text_w(probe, size) <= max_w or not cur:
            cur = probe
        else:
            lines.append(cur)
            cur = word
    if cur:
        lines.append(cur)
    return lines


def draw_button(c: Canvas, label, y=None, height=48.0) -> None:
    if y is None:
        y = H - 16 - height
    c.card(16, y, W - 32, height, 12, DARK["primary"])
    c.text(0, y + (height - 16) / 2 - 2, label, 15, DARK["onPrimary"], bold=True, center=W / 2)


# --- Ekrany -----------------------------------------------------------------

def screen_shoot(c: Canvas) -> None:
    draw_board(c, "Sparta", "Wilk", "0 : 0", 0, [], [], "Rzut 1 z 10")
    draw_scene(c)
    draw_goal(c)
    draw_keeper(c, W / 2, H * GOAL_LINE_Y, 0.155 * H, KEEPER_KIT, direction=1, progress=0.0)
    # celownik w lewym gornym rogu bramki
    aim = (W * 0.29, H * 0.36)
    r = 22.0
    c.d.ellipse([px(aim[0] - r), px(aim[1] - r), px(aim[0] + r), px(aim[1] + r)],
                outline=WHITE, width=px(3))
    c.circle(aim[0], aim[1], 4, WHITE)
    o = r + 8
    c.d.line([px(aim[0] - o), px(aim[1]), px(aim[0] - r + 4), px(aim[1])], fill=WHITE, width=px(3))
    c.d.line([px(aim[0] + r - 4), px(aim[1]), px(aim[0] + o), px(aim[1])], fill=WHITE, width=px(3))
    c.d.line([px(aim[0]), px(aim[1] - o), px(aim[0]), px(aim[1] - r + 4)], fill=WHITE, width=px(3))
    c.d.line([px(aim[0]), px(aim[1] + r - 4), px(aim[0]), px(aim[1] + o)], fill=WHITE, width=px(3))
    draw_player_back(c, W / 2, H * SHOOTER_Y, 0.148 * H, SHOOTER_KIT)
    draw_ball(c, W / 2, H * 0.79, 7)
    # pasek mocy (ShootoutScreen: dol, 32dp marginesu, 10dp wysokosci)
    bar_y = H - 28 - 10
    c.card(32, bar_y, W - 64, 10, 5, (0, 0, 0, 0x66))
    c.card(32, bar_y, (W - 64) * 0.62, 10, 5, (0xFF, 0xC9, 0x3C, 255))
    draw_hint(c, "Przeciągnij palcem po bramce, aby wycelować · przytrzymaj dłużej = mocniejszy strzał")


def screen_result(c: Canvas, outcome: str, keeper_dir: float, ball_pos, trajectory: bool) -> None:
    draw_board(
        c, "Zagłębie", "Bzura",
        "1 : 0" if outcome == "GOL!" else "0 : 0",
        2 if outcome == "GOL!" else 3,
        [True] if outcome == "GOL!" else [False], [], "Rzut 2 z 10" if outcome == "GOL!" else "Rzut 3 z 10",
    )
    draw_scene(c)
    draw_goal(c)
    draw_keeper(c, W * 0.52, H * GOAL_LINE_Y, 0.155 * H, KEEPER_KIT, direction=keeper_dir, progress=1.0)
    if trajectory:
        c.d.line([px(W / 2), px(H * 0.78), px(ball_pos[0]), px(ball_pos[1])],
                 fill=(0xFF, 0xFF, 0xFF, 0x99), width=px(2))
    draw_player_back(c, W / 2, H * SHOOTER_Y, 0.148 * H, SHOOTER_KIT)
    draw_ball(c, W / 2, H * 0.79, 7)
    draw_ball(c, ball_pos[0], ball_pos[1], 9)
    # nakladka wyniku (ResultScreen: Surface na srodku)
    label_w = c.text_w(outcome, 24, bold=True) + 48
    x = (W - label_w) / 2
    y = H * 0.5 - 26
    c.card(x, y, label_w, 52, 14, (0x16, 0x20, 0x2E, 0xCC))
    c.text(0, y + 12, outcome, 24, WHITE, bold=True, center=W / 2)
    draw_button(c, "Następny rzut")


def screen_tutorial(c: Canvas, page: int) -> None:
    # tlo: gradient ekranu samouczka
    c.img.alpha_composite(
        vertical_gradient_image((0x12, 0x33, 0x5C), (0x0E, 0x2A, 0x1E), int(px(H * 0.6)), int(px(W))),
        (0, 0),
    )
    c.img.alpha_composite(
        vertical_gradient_image((0x0E, 0x2A, 0x1E), (0x1C, 0x4B, 0x2A), int(px(H * 0.4)), int(px(W))),
        (0, px(H * 0.6)),
    )
    titles = {0: "Witaj w B-Klasa Hero!", 1: "Jak się strzela", 2: "Jak się broni"}
    bodies = {
        0: "Zagrasz serię rzutów karnych w swojej lidze. Najpierw wybierz miasto, dla którego grasz.",
        1: "Przeciągnij palcem po bramce, żeby wycelować. Przytrzymaj dłużej = mocniejszy strzał, ale mniej celny. Puść palec, żeby strzelić.",
        2: "Gdy strzela przeciwnik, przesuń palcem w stronę, w którą chcesz zanurkować. W górę = wysoki skok, w dół = niski.",
    }
    art_h = 210.0
    body_lines = wrap_text(c, bodies[page], 16, W - 120)
    card_h = 20 + art_h + 12 + line_h(24) + 12 + len(body_lines) * line_h(16) + 20
    card_y = (H - card_h) / 2 - 40
    c.card(20, card_y, W - 40, card_h, 20, (0x20, 0x2B, 0x3A, 0xE6))
    draw_tutorial_art(c, 40, card_y + 20, W - 80, art_h, page)
    ty = card_y + 20 + art_h + 12
    c.text(0, ty, titles[page], 24, WHITE, bold=True, center=W / 2)
    ty += line_h(24) + 12
    for line in body_lines:
        c.text(0, ty, line, 16, (0xCF, 0xE0, 0xF0, 255), center=W / 2)
        ty += line_h(16)
    # kropki krokow
    dots_y = card_y + card_h + 18
    for i in range(3):
        r = 6 if i == page else 4.5
        c.circle(W / 2 + (i - 1) * 20, dots_y + 6, r, WHITE if i == page else (0xFF, 0xFF, 0xFF, 0x66))
    draw_button(c, "Dalej" if page < 2 else "Wybierz swoje miasto", y=dots_y + 30, height=56)
    c.text(0, dots_y + 30 + 56 + 12, "Pomiń", 14, (0xCF, 0xE0, 0xF0, 255), center=W / 2)


def draw_tutorial_art(c: Canvas, x0, y0, w, h, page: int) -> None:
    """Odtworzenie TutorialArt z TutorialScreen.kt."""
    art = Canvas(w, h)  # lokalny uklad wspolrzednych
    art.d.rectangle([0, 0, px(w), px(h)], fill=GRASS)
    art.d.rectangle([0, px(h * 0.62), px(w), px(h)], fill=(0, 0, 0, 0x22))
    gl, gr, gt, gb = w * 0.09, w * 0.91, h * 0.06, h * 0.50
    art.card(gl, gt, gr - gl, gb - gt, 0, (0xFF, 0xFF, 0xFF, 0x1A))
    for i in range(1, 9):
        xx = gl + (gr - gl) * i / 9.0
        art.d.line([px(xx), px(gt), px(xx), px(gb)], fill=(0xFF, 0xFF, 0xFF, 0x55), width=px(1.5))
    for i in range(1, 5):
        yy = gt + (gb - gt) * i / 5.0
        art.d.line([px(gl), px(yy), px(gr), px(yy)], fill=(0xFF, 0xFF, 0xFF, 0x55), width=px(1.5))
    art.d.rectangle([px(gl - 2), px(gt), px(gl + 2), px(gb)], fill=WHITE)
    art.d.rectangle([px(gr - 2), px(gt), px(gr + 2), px(gb)], fill=WHITE)
    art.d.rectangle([px(gl), px(gt - 2), px(gr), px(gt + 2)], fill=WHITE)
    art.d.line([px(0), px(gb), px(w), px(gb)], fill=(0xFF, 0xFF, 0xFF, 0x99), width=px(2))

    if page == 0:
        art.circle(w * 0.5, h * 0.64, h * 0.045, (0xF7, 0xF7, 0xF2, 255))
        draw_player_back(art, w * 0.5, h * 1.0, h * 0.52, SHOOTER_KIT)
    elif page == 1:
        ball = (w * 0.5, h * 0.62)
        aim = (w * 0.29, h * 0.26)
        art.d.line([px(ball[0]), px(ball[1]), px(aim[0]), px(aim[1])],
                   fill=(0xFF, 0xFF, 0xFF, 0x99), width=px(3))
        r = h * 0.11
        art.d.ellipse([px(aim[0] - r), px(aim[1] - r), px(aim[0] + r), px(aim[1] + r)],
                      outline=WHITE, width=px(3))
        art.circle(aim[0], aim[1], 3, WHITE)
        art.d.line([px(aim[0] - r - 14), px(aim[1]), px(aim[0] - r + 2), px(aim[1])], fill=WHITE, width=px(3))
        art.d.line([px(aim[0] + r - 2), px(aim[1]), px(aim[0] + r + 14), px(aim[1])], fill=WHITE, width=px(3))
        art.d.line([px(aim[0]), px(aim[1] - r - 14), px(aim[0]), px(aim[1] - r + 2)], fill=WHITE, width=px(3))
        art.d.line([px(aim[0]), px(aim[1] + r - 2), px(aim[0]), px(aim[1] + r + 14)], fill=WHITE, width=px(3))
        art.circle(ball[0], ball[1], h * 0.042, (0xF7, 0xF7, 0xF2, 255))
        draw_player_back(art, w * 0.5, h * 0.93, h * 0.48, SHOOTER_KIT)
        art.card(w * 0.20, h * 0.95, w * 0.60, h * 0.05, 8, (0, 0, 0, 0x66))
        art.card(w * 0.20, h * 0.95, w * 0.60 * 0.65, h * 0.05, 8, (0xFF, 0xC9, 0x3C, 255))
    else:
        art.circle(w * 0.74, h * 0.66, h * 0.045, (0xF7, 0xF7, 0xF2, 255))
        draw_player_back(art, w * 0.64, h * 1.0, h * 0.50, SHOOTER_KIT)
        draw_keeper(art, w * 0.40, h * 0.60, h * 0.44, KEEPER_KIT, direction=-1, progress=1.0)
        ay = h * 0.87
        thick_line(art, (0xFF, 0xFF, 0xFF, 0xCC), w * 0.70, ay, w * 0.36, ay, 5)
        thick_line(art, (0xFF, 0xFF, 0xFF, 0xCC), w * 0.36, ay, w * 0.44, ay - h * 0.05, 5)
        thick_line(art, (0xFF, 0xFF, 0xFF, 0xCC), w * 0.36, ay, w * 0.44, ay + h * 0.05, 5)

    # wklej ilustracje w karte
    c.img.alpha_composite(art.img, (px(x0), px(y0)))


# --- Zapis ------------------------------------------------------------------

def render(screen: str, out_path: str, page: int = 0, scale: int = 3) -> None:
    """Rysuje ekran w dp (360x640) i skaluje do 1080x1920."""
    c = Canvas(W, H)
    if screen == "shoot":
        screen_shoot(c)
    elif screen == "goal":
        screen_result(c, "GOL!", keeper_dir=-1, ball_pos=(W * 0.25, H * 0.355), trajectory=True)
    elif screen == "save":
        screen_result(c, "WSPANIAŁA OBRONA!", keeper_dir=1, ball_pos=(W * 0.66, H * 0.40),
                      trajectory=True)
    elif screen == "tutorial":
        screen_tutorial(c, page)
    else:
        raise SystemExit("nieznany ekran: %s" % screen)
    img = c.img.copy()
    img.putalpha(255)
    final = img.resize(
        (int(round(px(W) * scale / 4)), int(round(px(H) * scale / 4))), Image.LANCZOS
    )
    final.convert("RGB").save(out_path)
    print("zapisano:", out_path, final.size)


if __name__ == "__main__":
    out_dir = sys.argv[1] if len(sys.argv) > 1 else "docs/game-shots"
    os.makedirs(out_dir, exist_ok=True)
    plan = [
        ("shoot", "bklasahero-3-strzal.png", 0),
        ("goal", "bklasahero-4-gol.png", 0),
        ("save", "bklasahero-5-obrona.png", 0),
        ("tutorial", "bklasahero-2-tutorial.png", 0),
        ("tutorial", "bklasahero-6-jak-bronic.png", 2),
    ]
    for screen, name, page in plan:
        render(screen, os.path.join(out_dir, name), page=page)
