// Motyw aplikacji — inspiracja gazetkami okręgowymi: zieleń murawy,
// białe linie, zgaszony odcień wieczornego sparingu.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
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

@Composable
fun BKlasaHeroTheme(darkTheme: Boolean = isSystemInDarkTheme(), content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = if (darkTheme) DarkScheme else LightScheme,
        typography = MaterialTheme.typography,
        content = content,
    )
}