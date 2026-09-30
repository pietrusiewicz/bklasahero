# B-Klasa Hero — ikony

Aplikacja startuje z generyczną ikoną Material 3 (placeholder).
Właściwe grafiki (mipmap-anydpi-v26/ic_launcher.xml z adaptacyjnym
tłem i foreground) są generowane przez `scripts/fastlane-icons.sh`
z materiałów graficznych w `metadata/`. Patrz `docs/FDROID.md`.

Póki co zostawiamy katalog pusty — `assembleRelease` użyje ikony
domyślnej z themes.xml, a F-Droid zgłosi brak jako ostrzeżenie,
nie błąd.

Patrz: `metadata/pl.bklasahero.yml` → `icon_url` (placeholder).