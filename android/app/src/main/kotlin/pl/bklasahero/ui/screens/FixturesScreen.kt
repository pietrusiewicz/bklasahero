// Ekran terminarza — samodzielny widok (z przyciskiem „wróć”). Treść renderuje
// wspólny FixturesView, ten sam, którego używa zakładka „Terminarz” w menu.
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
import pl.bklasahero.ui.components.FixturesView

@Composable
fun FixturesScreen(dispatch: (AppIntent) -> Unit) {
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        TextButton(onClick = { dispatch(AppIntent.GoBack) }) {
            Text(stringResource(R.string.menu_back))
        }
        Text(stringResource(R.string.fixtures_title), style = MaterialTheme.typography.headlineSmall)
        Spacer(Modifier.height(8.dp))
        FixturesView(modifier = Modifier.weight(1f))
    }
}
