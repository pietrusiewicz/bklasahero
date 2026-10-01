// Ekran wyniku — odtwarza animację z FrameBuffer, pokazuje duży napis wyniku
// (GOL / OBRONA / PUDŁO), tablicę wyników i przycisk do kolejnego rzutu.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import pl.bklasahero.AppViewModel
import pl.bklasahero.R
import pl.bklasahero.ui.AppUiState
import pl.bklasahero.ui.ScoreboardBar
import pl.bklasahero.ui.render.MatchRenderer

@Composable
fun ResultScreen(state: AppUiState, viewModel: AppViewModel) {
    Box(modifier = Modifier.fillMaxSize()) {
        MatchRenderer(state)
        ScoreboardBar(state.scoreboard)

        // Duży napis wyniku rzutu
        val outcome = outcomeLabel(state.lastOutcomeKey, state.lastKickRole)
        if (outcome.isNotEmpty()) {
            Surface(
                color = Color(0xCC16202E),
                shape = RoundedCornerShape(14.dp),
                modifier = Modifier
                    .align(Alignment.Center)
                    .padding(horizontal = 32.dp),
            ) {
                Text(
                    text = outcome,
                    style = MaterialTheme.typography.headlineMedium,
                    fontWeight = FontWeight.Bold,
                    color = Color.White,
                    textAlign = TextAlign.Center,
                    modifier = Modifier.padding(horizontal = 24.dp, vertical = 12.dp),
                )
            }
        }

        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp)
                .align(Alignment.BottomCenter),
        ) {
            if (state.scoreboard.finished) {
                Text(
                    text = stringResource(
                        if (state.scoreboard.homeScore > state.scoreboard.awayScore) {
                            R.string.result_win
                        } else if (state.scoreboard.homeScore < state.scoreboard.awayScore) {
                            R.string.result_lose
                        } else {
                            R.string.result_draw
                        }
                    ),
                    style = MaterialTheme.typography.titleLarge,
                    fontWeight = FontWeight.Bold,
                    textAlign = TextAlign.Center,
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(bottom = 8.dp),
                )
            }
            Button(
                onClick = { viewModel.continueAfterKick() },
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

@Composable
private fun outcomeLabel(outcomeKey: String?, role: String?): String {
    if (outcomeKey == null) return ""
    val shooting = role != "keeper"
    return when (outcomeKey) {
        "outcome.goal" -> stringResource(if (shooting) R.string.outcome_goal_shooter else R.string.outcome_goal_keeper)
        "outcome.saved" -> stringResource(if (shooting) R.string.outcome_saved_shooter else R.string.outcome_saved_keeper)
        "outcome.woodwork_in" -> stringResource(R.string.outcome_post_in)
        "outcome.woodwork_out" -> stringResource(R.string.outcome_post_out)
        "outcome.wide_left", "outcome.wide_right", "outcome.over_bar", "outcome.stopped" ->
            stringResource(if (shooting) R.string.outcome_miss_shooter else R.string.outcome_miss_keeper)
        else -> ""
    }
}
