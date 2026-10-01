// Pasek wyniku na górze ekranu meczu (rzuty karne).
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import pl.bklasahero.R

@Composable
fun ScoreboardBar(scoreboard: Scoreboard) {
    Surface(
        color = Color(0xCC16202E),
        shape = RoundedCornerShape(bottomStart = 14.dp, bottomEnd = 14.dp),
        shadowElevation = 4.dp,
    ) {
        Column(
            modifier = Modifier.fillMaxWidth().padding(horizontal = 16.dp, vertical = 10.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(
                    text = scoreboard.home.ifEmpty { "HOME" },
                    style = MaterialTheme.typography.titleMedium,
                    color = Color.White,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                    modifier = Modifier.weight(1f),
                )
                Text(
                    text = "${scoreboard.homeScore} : ${scoreboard.awayScore}",
                    style = MaterialTheme.typography.headlineMedium,
                    fontWeight = FontWeight.Bold,
                    color = Color.White,
                    modifier = Modifier.padding(horizontal = 12.dp),
                )
                Text(
                    text = scoreboard.away.ifEmpty { "AWAY" },
                    style = MaterialTheme.typography.titleMedium,
                    color = Color.White,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                    textAlign = androidx.compose.ui.text.style.TextAlign.End,
                    modifier = Modifier.weight(1f),
                )
            }
            val taken = scoreboard.homeTaken + scoreboard.awayTaken
            val total = scoreboard.kicksPerSide * 2
            Text(
                text = if (scoreboard.finished) {
                    stringResource(R.string.scoreboard_finished)
                } else {
                    stringResource(R.string.scoreboard_round, (taken + 1).coerceAtMost(total), total)
                },
                style = MaterialTheme.typography.bodySmall,
                color = Color(0xFFB9C6D4),
            )
        }
    }
}
