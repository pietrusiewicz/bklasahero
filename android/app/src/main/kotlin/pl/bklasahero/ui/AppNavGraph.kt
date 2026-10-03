// Główny przełącznik ekranów. Stan ekranu żyje w AppUiState.screen, więc
// renderujemy go wprost przez `when` — bez osobnego NavHost (mniej miejsca na
// desynchronizację stanu vs. graf nawigacji).
// Tutaj mieszka też obsługa systemowego „wstecz” i komunikaty (Snackbar).
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import pl.bklasahero.AppViewModel
import pl.bklasahero.R
import pl.bklasahero.ui.screens.CareerScreen
import pl.bklasahero.ui.screens.DefendScreen
import pl.bklasahero.ui.screens.FixturesScreen
import pl.bklasahero.ui.screens.HomeScreen
import pl.bklasahero.ui.screens.LoadingScreen
import pl.bklasahero.ui.screens.MatchScreen
import pl.bklasahero.ui.screens.NewCareerScreen
import pl.bklasahero.ui.screens.ResultScreen
import pl.bklasahero.ui.screens.ShootoutScreen
import pl.bklasahero.ui.screens.TableScreen
import pl.bklasahero.ui.screens.TutorialScreen

/** Ekrany, które są częścią trwającego meczu — wyjście z nich wymaga potwierdzenia. */
private val kMatchScreens = setOf(
    AppScreen.Match,
    AppScreen.Shootout,
    AppScreen.Defend,
    AppScreen.Result,
)

@Composable
fun AppNavGraph(state: AppUiState, viewModel: AppViewModel) {
    val dispatch = viewModel::dispatch
    val snackbarHostState = remember { SnackbarHostState() }
    var confirmAbortMatch by remember { mutableStateOf(false) }

    val toastText = toastMessage(state.toast)

    // Na Loading/Home „wstecz” zostawiamy systemowi (Home = wyjście z gry).
    BackHandler(enabled = state.screen != AppScreen.Home && state.screen != AppScreen.Loading) {
        if (state.screen in kMatchScreens) confirmAbortMatch = true else dispatch(AppIntent.GoBack)
    }

    LaunchedEffect(toastText) {
        if (toastText != null) {
            snackbarHostState.showSnackbar(toastText)
            dispatch(AppIntent.ShowToast(null))
        }
    }

    Box(modifier = Modifier.fillMaxSize()) {
        when (state.screen) {
            AppScreen.Loading -> LoadingScreen(state, dispatch)
            AppScreen.Tutorial -> TutorialScreen(state, dispatch)
            AppScreen.Home -> HomeScreen(state, viewModel)
            AppScreen.NewCareer -> NewCareerScreen(state, viewModel)
            AppScreen.Career -> CareerScreen(state, viewModel)
            AppScreen.Match -> MatchScreen(state, viewModel)
            AppScreen.Shootout -> ShootoutScreen(state, viewModel)
            AppScreen.Defend -> DefendScreen(state, viewModel)
            AppScreen.Result -> ResultScreen(state, viewModel)
            AppScreen.Table -> TableScreen(dispatch)
            AppScreen.Fixtures -> FixturesScreen(dispatch)
        }
        SnackbarHost(
            hostState = snackbarHostState,
            modifier = Modifier.align(Alignment.BottomCenter).padding(16.dp),
        )
    }

    if (confirmAbortMatch) {
        AlertDialog(
            onDismissRequest = { confirmAbortMatch = false },
            title = { Text(stringResource(R.string.match_abort_title)) },
            text = { Text(stringResource(R.string.match_abort_body)) },
            confirmButton = {
                TextButton(onClick = {
                    confirmAbortMatch = false
                    dispatch(AppIntent.GoBack)
                }) { Text(stringResource(R.string.match_abort_ok)) }
            },
            dismissButton = {
                TextButton(onClick = { confirmAbortMatch = false }) {
                    Text(stringResource(R.string.match_abort_cancel))
                }
            },
        )
    }
}

/**
 * Zamienia techniczny klucz komunikatu z warstwy danych na tekst dla gracza.
 * Nieznane komunikaty pokazujemy jako ogólny błąd zamiast surowego wyjątku.
 */
@Composable
private fun toastMessage(raw: String?): String? = when (raw) {
    null -> null
    "saved" -> stringResource(R.string.toast_saved)
    "places.load_failed" -> stringResource(R.string.toast_places_failed)
    else -> stringResource(R.string.toast_error, raw)
}
