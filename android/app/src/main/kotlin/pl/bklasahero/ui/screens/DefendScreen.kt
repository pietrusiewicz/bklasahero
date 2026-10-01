// Ekran obrony — rzut karny wykonuje przeciwnik, gracz jest bramkarzem.
// Przesuń palcem w kierunku, w który chcesz zanurkować:
//   lewo/prawo/środek (bok) + góra/dół (wysokość skoku).
// Puszczenie palca = decyzja → rdzeń rozstrzyga rzut.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import pl.bklasahero.AppViewModel
import pl.bklasahero.R
import pl.bklasahero.engine.FrameBuffer
import pl.bklasahero.ui.AppUiState
import pl.bklasahero.ui.ScoreboardBar
import pl.bklasahero.ui.render.MatchRenderer

@Composable
fun DefendScreen(state: AppUiState, viewModel: AppViewModel) {
    LaunchedEffect(state.screen) { FrameBuffer.clear() }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .pointerInput(Unit) {
                awaitEachGesture {
                    val down = awaitFirstDown()
                    var end = down.position
                    while (true) {
                        val event = awaitPointerEvent()
                        val change = event.changes.firstOrNull { it.id == down.id } ?: break
                        if (!change.pressed) break
                        end = change.position
                    }
                    val dx = end.x - down.position.x
                    val dy = end.y - down.position.y
                    val side = when {
                        dx < -size.width * 0.08f -> "Left"
                        dx > size.width * 0.08f -> "Right"
                        else -> "Center"
                    }
                    val height = when {
                        dy < -size.height * 0.08f -> "High"
                        dy > size.height * 0.08f -> "Low"
                        else -> "Mid"
                    }
                    viewModel.dive(side, height)
                }
            },
    ) {
        MatchRenderer(state)
        ScoreboardBar(state.scoreboard)
        Text(
            text = stringResource(R.string.shootout_defend_hint),
            style = MaterialTheme.typography.bodyMedium,
            color = Color.White,
            modifier = Modifier
                .align(Alignment.TopCenter)
                .padding(top = 92.dp, start = 24.dp, end = 24.dp),
        )
    }
}
