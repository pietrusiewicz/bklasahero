// Ekran wyniku — odtwarza animację z FrameBuffer, pokazuje tablicę wyników
// i pozwala przejść do kolejnego rzutu (albo zakończyć mecz).
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import pl.bklasahero.R
import pl.bklasahero.ui.AppIntent
import pl.bklasahero.ui.AppScreen
import pl.bklasahero.ui.AppUiState
import pl.bklasahero.ui.ScoreboardBar
import pl.bklasahero.ui.render.MatchRenderer

@Composable
fun ResultScreen(state: AppUiState, dispatch: (AppIntent) -> Unit) {
    Box(modifier = Modifier.fillMaxSize()) {
        MatchRenderer(state)
        ScoreboardBar(state.scoreboard)
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp)
                .align(Alignment.BottomCenter),
        ) {
            Button(
                onClick = {
                    dispatch(
                        AppIntent.SetScreen(
                            if (state.scoreboard.finished) AppScreen.Home else AppScreen.Shootout
                        )
                    )
                },
                modifier = Modifier.fillMaxWidth(),
            ) {
                Text(
                    stringResource(
                        if (state.scoreboard.finished) R.string.result_finish else R.string.result_next_kick
                    )
                )
            }
        }
    }
}
