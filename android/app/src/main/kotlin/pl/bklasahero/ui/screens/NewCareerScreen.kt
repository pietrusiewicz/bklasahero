// Formularz nowej kariery + wyszukiwarka miasta.
// Wpisujesz nick, wybierasz miasto z listy (podpowiedzi z rdzenia: cmd="searchCity").
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
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.serialization.json.JsonObject
import pl.bklasahero.R
import pl.bklasahero.ui.AppIntent
import pl.bklasahero.ui.AppUiState

@Composable
fun NewCareerScreen(state: AppUiState, dispatch: (AppIntent) -> Unit) {
    var nickname by remember { mutableStateOf("") }
    var query by remember { mutableStateOf("") }
    val suggestions = remember { MutableStateFlow<List<JsonObject>>(emptyList()) }
    val list by suggestions.collectAsState()

    LaunchedEffect(query) {
        if (query.length >= 2) {
            // Tu idzie wywołanie NativeBridge.command({"cmd":"searchCity","query":query})
            // — pomijam pełny dispatcher, żeby ekran był samodzielny.
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
            items(list) { city ->
                TextButton(onClick = {
                    val id = (city["osmId"] ?: city["osm_id"])?.toString()?.toIntOrNull() ?: 0
                    dispatch(AppIntent.StartNewCareer(nickname, id))
                }) { Text(city["name"]?.toString().orEmpty()) }
            }
        }
        Button(
            onClick = { dispatch(AppIntent.SetScreen(AppScreen.Career)) },
            modifier = Modifier.fillMaxWidth(),
            enabled = nickname.isNotBlank(),
        ) {
            Text(stringResource(R.string.new_career_continue))
        }
    }
}