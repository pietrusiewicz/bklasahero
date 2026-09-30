// Ekran meczu — renderuje boisko (pseudo-3D), obsługuje wejścia (strzał/obrona).
// Tu wywoływane są polecenia "shoot" i "dive" — odpowiedź to FrameBuffer do animacji.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import pl.bklasahero.R
import pl.bklasahero.engine.FrameBuffer
import pl.bklasahero.ui.AppIntent
import pl.bklasahero.ui.AppUiState
import pl.bklasahero.ui.render.MatchRenderer

@Composable
fun MatchScreen(state: AppUiState, dispatch: (AppIntent) -> Unit) {
    // Po wejściu na ekran — zaciągamy klatkę z mostka (jeśli jest).
    LaunchedEffect(state.screen) {
        if (FrameBuffer.floats.isEmpty()) {
            FrameBuffer.pull(FloatArray(0))
        }
    }
    Box(modifier = Modifier.fillMaxSize()) {
        MatchRenderer(state)
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                stringResource(R.string.match_tap_to_shoot),
                style = MaterialTheme.typography.bodyMedium,
            )
            // Overlay akcji (przyciski kierunku/wysokości) dodany przez konkretny
            // widok (shootout/defend) — w wersji 0.x uproszczone: pełnoekranowy tap.
        }
    }
}