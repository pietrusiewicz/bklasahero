// Ekran tabeli ligi — samodzielny widok (z przyciskiem „wróć”) na wypadek
// wejścia z ekranu kariery. Treść renderuje wspólny LeagueTableView, ten sam,
// którego używa zakładka „Tabela” w menu głównym.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import pl.bklasahero.R
import pl.bklasahero.ui.AppIntent
import pl.bklasahero.ui.components.LeagueTableView

@Composable
fun TableScreen(dispatch: (AppIntent) -> Unit) {
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        TextButton(onClick = { dispatch(AppIntent.GoBack) }) {
            Text(stringResource(R.string.menu_back))
        }
        Text(stringResource(R.string.table_title), style = MaterialTheme.typography.headlineSmall)
        Spacer(Modifier.height(8.dp))
        LeagueTableView(modifier = Modifier.weight(1f))
    }
}
