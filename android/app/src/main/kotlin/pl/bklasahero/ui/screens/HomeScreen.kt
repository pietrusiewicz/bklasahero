// Ekran główny — dwa przyciski: Nowa Kariera / Kontynuuj (gdy istnieje save).
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import pl.bklasahero.R
import pl.bklasahero.ui.AppIntent
import pl.bklasahero.ui.AppUiState

@Composable
fun HomeScreen(state: AppUiState, dispatch: (AppIntent) -> Unit) {
    Box(modifier = Modifier.fillMaxSize().padding(24.dp), contentAlignment = Alignment.Center) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(20.dp),
            modifier = Modifier.fillMaxWidth(),
        ) {
            Text(
                stringResource(R.string.app_name),
                style = MaterialTheme.typography.displaySmall,
            )
            Text(
                stringResource(R.string.app_subtitle),
                style = MaterialTheme.typography.bodyLarge,
            )
            Button(
                onClick = { dispatch(AppIntent.SetScreen(AppScreen.NewCareer)) },
                modifier = Modifier.fillMaxWidth().size(width = 0.dp, height = 56.dp),
            ) {
                Text(stringResource(R.string.home_new_career))
            }
            if (state.careerReady) {
                OutlinedButton(
                    onClick = { dispatch(AppIntent.SetScreen(AppScreen.Career)) },
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    Text(stringResource(R.string.home_continue_career))
                }
            }
        }
    }
}