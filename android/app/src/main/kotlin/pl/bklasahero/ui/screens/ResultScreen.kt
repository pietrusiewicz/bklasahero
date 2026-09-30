// Ekran wyniku — odtwarza animację z FrameBuffer, pokazuje tekst (gol/obrona/słupek).
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import pl.bklasahero.R
import pl.bklasahero.ui.AppIntent
import pl.bklasahero.ui.AppUiState
import pl.bklasahero.ui.render.MatchRenderer

@Composable
fun ResultScreen(state: AppUiState, dispatch: (AppIntent) -> Unit) {
    Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
        MatchRenderer(state)
        Text(
            stringResource(R.string.result_swipe_to_continue),
            style = MaterialTheme.typography.titleMedium,
            modifier = Modifier.padding(32.dp),
        )
    }
}