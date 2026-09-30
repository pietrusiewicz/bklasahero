// Ekran wykonywania rzutu karnego. Tap na bramkę → strzał.
// Wynik trafia do FrameBuffer, animator ponownie go odtwarza.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.pointer.pointerInput
import kotlinx.serialization.json.buildJsonObject
import kotlinx.serialization.json.put
import pl.bklasahero.AppViewModel
import pl.bklasahero.engine.FrameBuffer
import pl.bklasahero.engine.NativeBridge
import pl.bklasahero.ui.AppScreen
import pl.bklasahero.ui.AppUiState
import pl.bklasahero.ui.render.MatchRenderer

@Composable
fun ShootoutScreen(state: AppUiState, viewModel: AppViewModel) {
    LaunchedEffect(state.screen) { FrameBuffer.pull(FloatArray(0)) }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .pointerInput(Unit) {
                detectTapGestures(onTap = { offset ->
                    // Normalizuj do zakresu [-3.66, 3.66] x [0.1, 2.44] (bramka).
                    val x = (offset.x / size.width - 0.5f) * 7.32f
                    val y = (offset.y / size.height) * 2.44f + 0.1f
                    val cmd = buildJsonObject {
                        put("cmd", "shoot")
                        put("aimX", x)
                        put("aimY", y.coerceAtLeast(0.1f))
                        put("effort", 0.7f)
                    }
                    NativeBridge.command(cmd).onSuccess {
                        FrameBuffer.pull(FloatArray(0))
                        viewModel.dispatch(pl.bklasahero.ui.AppIntent.SetScreen(AppScreen.Result))
                    }
                })
            },
    ) {
        MatchRenderer(state)
    }
}