// Ekran kariery — podsumowanie: nick, liga, następny mecz, akcje (tabela,
// terminarz, zapis).
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import kotlinx.serialization.json.jsonPrimitive
import pl.bklasahero.AppViewModel
import pl.bklasahero.R
import pl.bklasahero.ui.AppIntent
import pl.bklasahero.ui.AppScreen
import pl.bklasahero.ui.AppUiState

@Composable
fun CareerScreen(state: AppUiState, viewModel: AppViewModel) {
    LaunchedEffect(Unit) { viewModel.refreshCareer() }

    val career = state.careerJson
    val nickname = career?.get("nickname")?.jsonPrimitive?.content ?: ""
    val tier = career?.get("tierIndex")?.jsonPrimitive?.content?.toIntOrNull() ?: 0

    Column(
        modifier = Modifier.fillMaxSize().padding(24.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp),
    ) {
        Text(stringResource(R.string.career_hello, nickname), style = MaterialTheme.typography.headlineMedium)
        Text(stringResource(R.string.career_tier, tier), style = MaterialTheme.typography.titleMedium)
        Row(horizontalArrangement = Arrangement.spacedBy(12.dp), modifier = Modifier.fillMaxWidth()) {
            Button(
                onClick = { viewModel.dispatch(AppIntent.SetScreen(AppScreen.Match)) },
                modifier = Modifier.weight(1f),
            ) { Text(stringResource(R.string.career_next_match)) }
        }
        Row(horizontalArrangement = Arrangement.spacedBy(12.dp), modifier = Modifier.fillMaxWidth()) {
            OutlinedButton(
                onClick = { viewModel.dispatch(AppIntent.SetScreen(AppScreen.Table)) },
                modifier = Modifier.weight(1f),
            ) { Text(stringResource(R.string.career_table)) }
            OutlinedButton(
                onClick = { viewModel.dispatch(AppIntent.SetScreen(AppScreen.Fixtures)) },
                modifier = Modifier.weight(1f),
            ) { Text(stringResource(R.string.career_fixtures)) }
        }
        OutlinedButton(
            onClick = { viewModel.dispatch(AppIntent.SaveCareer) },
            modifier = Modifier.fillMaxWidth(),
        ) { Text(stringResource(R.string.career_save)) }
    }
}