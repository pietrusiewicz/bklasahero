// Ekran terminarza — kolejka 1..9, pary meczów.
// Dane z polecenia "fixtures".
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import pl.bklasahero.R
import pl.bklasahero.ui.AppIntent
import pl.bklasahero.ui.AppUiState

@Composable
fun FixturesScreen(state: AppUiState, dispatch: (AppIntent) -> Unit) {
    LazyColumn(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        item { Text(stringResource(R.string.fixtures_title), style = MaterialTheme.typography.headlineSmall) }
        // Renderowanie odpowiedzi cmd="fixtures".
    }
}