// Motyw aplikacji — inspiracja gazetkami okręgowymi: zieleń murawy,
// białe linie, zgaszony odcień wieczornego sparingu.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.staticCompositionLocalOf
import androidx.compose.ui.graphics.Color

private val PitchGreen = Color(0xFF2C8E3F)
private val PitchGreenDark = Color(0xFF14632A)
private val NetWhite = Color(0xFFF7F7F2)
private val CardYellow = Color(0xFFFFC400)
private val RefRed = Color(0xFFD32F2F)

private val LightScheme = lightColorScheme(
    primary = PitchGreenDark,
    onPrimary = NetWhite,
    secondary = CardYellow,
    background = NetWhite,
    surface = NetWhite,
    error = RefRed,
)

private val DarkScheme = darkColorScheme(
    primary = PitchGreen,
    onPrimary = NetWhite,
    secondary = CardYellow,
    background = Color(0xFF101713),
    surface = Color(0xFF1B201C),
    error = RefRed,
)

/**
 * Kolory, których nie ma w palecie Material: tło menu, karty i obramowania.
 * Ekrany używają ich zamiast zaszytych na sztywno hexów, dzięki czemu
 * przełącznik motywu zmienia cały ekran, a nie tylko część widgetów.
 */
@Immutable
data class BkhColors(
    val heroTop: Color,
    val heroBottom: Color,
    val card: Color,
    val cardBorder: Color,
    val onHero: Color,
    val onHeroMuted: Color,
    val accent: Color,
    val danger: Color,
    /** Tło schematycznej mapki w zapowiedzi meczu. */
    val mapBackground: Color,
    /** Siatka i linie pomocnicze na mapce. */
    val mapGrid: Color,
)

private val LightBkhColors = BkhColors(
    heroTop = Color(0xFFDCEBDF),
    heroBottom = Color(0xFFF7F7F2),
    card = Color(0xFFFFFFFF),
    cardBorder = Color(0x2414632A),
    onHero = Color(0xFF10281B),
    onHeroMuted = Color(0xFF4C6B58),
    accent = PitchGreenDark,
    danger = RefRed,
    mapBackground = Color(0xFFE3EDE5),
    mapGrid = Color(0x1F14632A),
)

private val DarkBkhColors = BkhColors(
    heroTop = Color(0xFF12335C),
    heroBottom = Color(0xFF0E2A1E),
    card = Color(0xE6202B3A),
    cardBorder = Color(0x33FFFFFF),
    onHero = Color.White,
    onHeroMuted = Color(0xFFB3C8D8),
    accent = PitchGreen,
    danger = Color(0xFFFFB4AB),
    mapBackground = Color(0xFF16202E),
    mapGrid = Color(0x33FFFFFF),
)

/** Aktualna paleta dodatkowa — patrz [BKlasaHeroTheme]. */
val LocalBkhColors = staticCompositionLocalOf { LightBkhColors }

@Composable
fun BKlasaHeroTheme(darkTheme: Boolean = isSystemInDarkTheme(), content: @Composable () -> Unit) {
    CompositionLocalProvider(
        LocalBkhColors provides if (darkTheme) DarkBkhColors else LightBkhColors,
    ) {
        MaterialTheme(
            colorScheme = if (darkTheme) DarkScheme else LightScheme,
            typography = MaterialTheme.typography,
            content = content,
        )
    }
}
