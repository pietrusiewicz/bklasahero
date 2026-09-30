// Ekran tabeli ligi — renderuje listę drużyn z punktami.
// Dane pobierane poleceniem "table" (zwraca JSON z tablica).
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
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
fun TableScreen(state: AppUiState, dispatch: (AppIntent) -> Unit) {
    LazyColumn(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        item { Text(stringResource(R.string.table_title), style = MaterialTheme.typography.headlineSmall) }
        // Tu idzie renderowanie odpowiedzi cmd="table".
        items(emptyList<String>()) { team ->
            Text(team, style = MaterialTheme.typography.bodyMedium)
        }
    }
}