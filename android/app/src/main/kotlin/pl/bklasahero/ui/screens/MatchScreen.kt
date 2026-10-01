// Ekran meczu — renderuje boisko (pseudo-3D) i startuje konkurs rzutów karnych.
// W wersji 0.1.0 mecz = seria 5 rzutów + 5 obron (patrz ShootoutScreen).
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import pl.bklasahero.AppViewModel
import pl.bklasahero.R
import pl.bklasahero.engine.FrameBuffer
import pl.bklasahero.ui.AppUiState
import pl.bklasahero.ui.ScoreboardBar
import pl.bklasahero.ui.render.MatchRenderer

@Composable
fun MatchScreen(state: AppUiState, viewModel: AppViewModel) {
    // Po wejściu na ekran — zaciągamy klatkę z mostka (jeśli jest).
    LaunchedEffect(state.screen) {
        if (FrameBuffer.floats.isEmpty()) {
            FrameBuffer.pull(FloatArray(0))
        }
    }
    Box(modifier = Modifier.fillMaxSize()) {
        MatchRenderer(state)
        ScoreboardBar(state.scoreboard)
        Column(
            modifier = Modifier.fillMaxWidth().padding(16.dp).align(Alignment.BottomCenter),
        ) {
            Text(
                stringResource(R.string.match_tap_to_shoot),
                style = MaterialTheme.typography.bodyMedium,
            )
            Button(
                onClick = { viewModel.beginMatch() },
                modifier = Modifier.fillMaxWidth().padding(top = 8.dp),
            ) {
                Text(stringResource(R.string.match_start_shootout))
            }
        }
    }
}
