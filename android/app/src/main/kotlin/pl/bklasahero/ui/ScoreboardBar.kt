// Pasek wyniku na górze ekranu meczu (rzuty karne).
// Pod nazwami drużyn kropki postępu serii: zielona = gol, czerwona = pudło,
// szara = rzut jeszcze nie wykonany. W nagłej śmierci kropki się wydłużają.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
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

private val DotGoal = Color(0xFF66BB6A)
private val DotMiss = Color(0xFFE5484D)
private val DotEmpty = Color(0xFF42526B)

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
                Column(modifier = Modifier.weight(1f)) {
                    Text(
                        text = scoreboard.home.ifEmpty { "HOME" },
                        style = MaterialTheme.typography.titleMedium,
                        color = Color.White,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis,
                    )
                    KickDots(scoreboard.homeKicks, scoreboard.kicksPerSide)
                }
                Text(
                    text = "${scoreboard.homeScore} : ${scoreboard.awayScore}",
                    style = MaterialTheme.typography.headlineMedium,
                    fontWeight = FontWeight.Bold,
                    color = Color.White,
                    modifier = Modifier.padding(horizontal = 12.dp),
                )
                Column(
                    modifier = Modifier.weight(1f),
                    horizontalAlignment = Alignment.End,
                ) {
                    Text(
                        text = scoreboard.away.ifEmpty { "AWAY" },
                        style = MaterialTheme.typography.titleMedium,
                        color = Color.White,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis,
                        textAlign = androidx.compose.ui.text.style.TextAlign.End,
                    )
                    KickDots(scoreboard.awayKicks, scoreboard.kicksPerSide)
                }
            }
            val taken = scoreboard.homeTaken + scoreboard.awayTaken
            val total = scoreboard.kicksPerSide * 2
            val suddenDeath = scoreboard.homeTaken >= scoreboard.kicksPerSide &&
                scoreboard.awayTaken >= scoreboard.kicksPerSide
            Text(
                text = when {
                    scoreboard.finished -> stringResource(R.string.scoreboard_finished)
                    suddenDeath -> stringResource(R.string.scoreboard_sudden_death)
                    else -> stringResource(R.string.scoreboard_round, (taken + 1).coerceAtMost(total), total)
                },
                style = MaterialTheme.typography.bodySmall,
                color = Color(0xFFB9C6D4),
            )
        }
    }
}

/** Kropki serii: wykonane rzuty (zielona/czerwona) + puste miejsca do 5 (i dalej w nagłej śmierci). */
@Composable
private fun KickDots(kicks: List<Boolean>, kicksPerSide: Int) {
    Row(
        horizontalArrangement = Arrangement.spacedBy(5.dp),
        modifier = Modifier.padding(top = 6.dp),
    ) {
        kicks.forEach { scored ->
            Box(
                modifier = Modifier
                    .size(11.dp)
                    .background(color = if (scored) DotGoal else DotMiss, shape = CircleShape),
            )
        }
        repeat((kicksPerSide - kicks.size).coerceAtLeast(0)) {
            Box(
                modifier = Modifier
                    .size(11.dp)
                    .background(color = DotEmpty, shape = CircleShape),
            )
        }
    }
}
