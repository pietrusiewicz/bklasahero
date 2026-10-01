// Ekran tabeli ligi — polecenie "table" → lista drużyn z punktami.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
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
import pl.bklasahero.ui.AppScreen
import pl.bklasahero.ui.AppUiState

private data class TableRow(
    val position: Int,
    val name: String,
    val played: Int,
    val goalsFor: Int,
    val goalsAgainst: Int,
    val points: Int,
)

@Composable
fun TableScreen(state: AppUiState, dispatch: (AppIntent) -> Unit) {
    var rows by remember { mutableStateOf<List<TableRow>>(emptyList()) }
    var label by remember { mutableStateOf("") }

    LaunchedEffect(Unit) {
        NativeBridge.command(buildJsonObject { put("cmd", "table") })
            .onSuccess { data ->
                label = data["leagueLabel"]?.jsonPrimitive?.content ?: ""
                val arr = data["table"]?.jsonArray ?: return@onSuccess
                rows = arr.mapNotNull { el ->
                    val o = el as? kotlinx.serialization.json.JsonObject ?: return@mapNotNull null
                    TableRow(
                        position = o["position"]?.jsonPrimitive?.content?.toIntOrNull() ?: 0,
                        name = o["name"]?.jsonPrimitive?.content ?: "",
                        played = o["played"]?.jsonPrimitive?.content?.toIntOrNull() ?: 0,
                        goalsFor = o["goalsFor"]?.jsonPrimitive?.content?.toIntOrNull() ?: 0,
                        goalsAgainst = o["goalsAgainst"]?.jsonPrimitive?.content?.toIntOrNull() ?: 0,
                        points = o["points"]?.jsonPrimitive?.content?.toIntOrNull() ?: 0,
                    )
                }
            }
    }

    LazyColumn(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        item {
            TextButton(onClick = { dispatch(AppIntent.SetScreen(AppScreen.Home)) }) {
                Text(stringResource(R.string.menu_back))
            }
        }
        item { Text(stringResource(R.string.table_title), style = MaterialTheme.typography.headlineSmall) }
        item {
            if (label.isNotEmpty()) {
                Text(label, style = MaterialTheme.typography.titleSmall)
            }
        }
        items(rows) { row ->
            Row(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                Text(
                    text = row.position.toString().padStart(2),
                    style = MaterialTheme.typography.bodyMedium,
                    modifier = Modifier.weight(0.12f),
                )
                Text(
                    text = row.name,
                    style = MaterialTheme.typography.bodyMedium,
                    modifier = Modifier.weight(0.5f),
                )
                Text(
                    text = "${row.goalsFor}:${row.goalsAgainst}",
                    style = MaterialTheme.typography.bodyMedium,
                    modifier = Modifier.weight(0.2f),
                )
                Text(
                    text = row.points.toString(),
                    style = MaterialTheme.typography.bodyMedium,
                    modifier = Modifier.weight(0.18f),
                )
            }
        }
    }
}