// B-Klasa Hero — konfiguracja korzenia projektu (AGP 8.5+, Kotlin 1.9+).
// Wersje celowo pinowane — żeby build był powtarzalny w F-Droid i CI.
// SPDX-License-Identifier: GPL-3.0-or-later
plugins {
    id("com.android.application") version "8.5.2" apply false
    id("org.jetbrains.kotlin.android") version "1.9.24" apply false
    id("org.jetbrains.kotlin.plugin.compose") version "1.9.24" apply false
    id("com.google.devtools.ksp") version "1.9.24-1.0.20" apply false
}