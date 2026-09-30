// Formularz nowej kariery + wyszukiwarka miasta.
// Wpisujesz nick, wybierasz miasto z listy (podpowiedzi z rdzenia: cmd="searchCity"),
// zatwierdzasz → cmd="newCareer".
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
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
import pl.bklasahero.AppViewModel
import pl.bklasahero.R
import pl.bklasahero.engine.NativeBridge
import pl.bklasahero.ui.AppUiState

private data class CitySuggestion(val osmId: Int, val name: String)

@Composable
fun NewCareerScreen(state: AppUiState, viewModel: AppViewModel) {
    var nickname by remember { mutableStateOf("") }
    var query by remember { mutableStateOf("") }
    var suggestions by remember { mutableStateOf<List<CitySuggestion>>(emptyList()) }

    LaunchedEffect(query) {
        if (query.length < 2) {
            suggestions = emptyList()
            return@LaunchedEffect
        }
        val cmd = buildJsonObject {
            put("cmd", "searchCity")
            put("query", query)
            put("limit", 8)
        }
        NativeBridge.command(cmd)
            .onSuccess { data ->
                val arr = data["items"]?.jsonArray ?: return@onSuccess
                suggestions = arr.mapNotNull { el ->
                    val obj = el as? kotlinx.serialization.json.JsonObject ?: return@mapNotNull null
                    val osm = obj["osmId"]?.jsonPrimitive?.content?.toIntOrNull() ?: return@mapNotNull null
                    val name = obj["name"]?.jsonPrimitive?.content ?: return@mapNotNull null
                    CitySuggestion(osm, name)
                }
            }
    }

    Column(
        modifier = Modifier.fillMaxSize().padding(24.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        Text(stringResource(R.string.new_career_title), style = MaterialTheme.typography.headlineSmall)
        OutlinedTextField(
            value = nickname,
            onValueChange = { nickname = it },
            label = { Text(stringResource(R.string.new_career_nickname)) },
            singleLine = true,
            modifier = Modifier.fillMaxWidth(),
        )
        OutlinedTextField(
            value = query,
            onValueChange = { query = it },
            label = { Text(stringResource(R.string.new_career_search_city)) },
            singleLine = true,
            modifier = Modifier.fillMaxWidth(),
        )
        LazyColumn(modifier = Modifier.fillMaxWidth().weight(1f)) {
            items(suggestions) { city ->
                TextButton(onClick = {
                    viewModel.startNewCareer(nickname.ifBlank { "B-Klasa" }, city.osmId)
                }) { Text(city.name) }
            }
        }
        Button(
            onClick = {
                val first = suggestions.firstOrNull()
                if (first != null && nickname.isNotBlank()) {
                    viewModel.startNewCareer(nickname, first.osmId)
                }
            },
            modifier = Modifier.fillMaxWidth(),
            enabled = nickname.isNotBlank(),
        ) {
            Text(stringResource(R.string.new_career_continue))
        }
    }
}