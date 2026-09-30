// Ekran terminarza — polecenie "fixtures" → pary meczów wg kolejki.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import kotlinx.serialization.json.buildJsonObject
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonPrimitive
import kotlinx.serialization.json.put
import pl.bklasahero.R
import pl.bklasahero.engine.NativeBridge
import pl.bklasahero.ui.AppIntent
import pl.bklasahero.ui.AppUiState

private data class FixtureRow(
    val round: Int,
    val home: String,
    val away: String,
    val played: Boolean,
    val homeGoals: Int,
    val awayGoals: Int,
)

@Composable
fun FixturesScreen(state: AppUiState, dispatch: (AppIntent) -> Unit) {
    var rows by remember { mutableStateOf<List<FixtureRow>>(emptyList()) }

    LaunchedEffect(Unit) {
        NativeBridge.command(buildJsonObject { put("cmd", "fixtures") })
            .onSuccess { data ->
                val arr = data["fixtures"]?.jsonArray ?: return@onSuccess
                rows = arr.mapNotNull { el ->
                    val o = el as? kotlinx.serialization.json.JsonObject ?: return@mapNotNull null
                    FixtureRow(
                        round = o["round"]?.jsonPrimitive?.content?.toIntOrNull() ?: 0,
                        home = o["home"]?.jsonPrimitive?.content ?: "",
                        away = o["away"]?.jsonPrimitive?.content ?: "",
                        played = o["played"]?.jsonPrimitive?.content?.toBooleanStrictOrNull() ?: false,
                        homeGoals = o["homeGoals"]?.jsonPrimitive?.content?.toIntOrNull() ?: 0,
                        awayGoals = o["awayGoals"]?.jsonPrimitive?.content?.toIntOrNull() ?: 0,
                    )
                }
            }
    }

    LazyColumn(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        item { Text(stringResource(R.string.fixtures_title), style = MaterialTheme.typography.headlineSmall) }
        items(rows) { row ->
            Column(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                Text(
                    text = stringResource(R.string.fixtures_round, row.round),
                    style = MaterialTheme.typography.labelMedium,
                )
                Text(
                    text = if (row.played) {
                        "${row.home} ${row.homeGoals}:${row.awayGoals} ${row.away}"
                    } else {
                        "${row.home} — ${row.away}"
                    },
                    style = MaterialTheme.typography.bodyMedium,
                )
            }
        }
    }
}