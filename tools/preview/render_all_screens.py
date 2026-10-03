#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Renderuje KOMPLET zrzutow B-Klasa Hero na strone aplikacji (9 pozycji).

Zrzuty maja wspolny styl: ciemny motyw, 1080x1920 (360x640 dp @3x), bez ramki
telefonu i paska systemowego. Nazwy plikow odpowiadaja wierszom galerii
(`finder_zrzutekranu` w bazie strony).

Uruchomienie:
    python3 tools/preview/render_all_screens.py /sciezka/docelowa
"""
from __future__ import annotations

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import render_game_mockup as game  # noqa: E402
import render_match_mockup as match  # noqa: E402
import render_menu_mockup as menu  # noqa: E402

HEIGHT = 640.0  # 1080x1920 przy skali 3x
SCALE = 3


def main(out_dir: str) -> None:
    os.makedirs(out_dir, exist_ok=True)
    out = lambda name: os.path.join(out_dir, name)  # noqa: E731

    # Menu glowne — trzy zakladki (komponent menu).
    menu.render_screen(out("bklasahero-1-menu.png"), dark=True, tab=0, height=HEIGHT, scale=SCALE)
    menu.render_screen(out("bklasahero-8-menu-terminarz.png"), dark=True, tab=1,
                       height=HEIGHT, scale=SCALE)
    menu.render_screen(out("bklasahero-9-menu-kariera.png"), dark=True, tab=2,
                       height=HEIGHT, scale=SCALE, scroll=68)

    # Zapowiedz meczu — laurka na mapce okolicy z odlegloscia.
    match.render_screen(out("bklasahero-7-zapowiedz-meczu.png"), dark=True, round_no=8,
                        decisive=True, height=HEIGHT, scale=SCALE)

    # Rozgrywka i samouczek.
    game.render("tutorial", out("bklasahero-2-tutorial.png"), page=0)
    game.render("shoot", out("bklasahero-3-strzal.png"))
    game.render("goal", out("bklasahero-4-gol.png"))
    game.render("save", out("bklasahero-5-obrona.png"))
    game.render("tutorial", out("bklasahero-6-jak-bronic.png"), page=2)

    print("gotowe — 9 zrzutow w", out_dir)


if __name__ == "__main__":
    target = sys.argv[1] if len(sys.argv) > 1 else "docs/screens"
    main(target)
