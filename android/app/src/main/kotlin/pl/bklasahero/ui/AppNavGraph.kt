// Główny przełącznik ekranów. Stan ekranu żyje w AppUiState.screen, więc
// renderujemy go wprost przez `when` — bez osobnego NavHost (mniej miejsca na
// desynchronizację stanu vs. graf nawigacji).
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui

import androidx.compose.runtime.Composable
import pl.bklasahero.AppViewModel
import pl.bklasahero.ui.screens.CareerScreen
import pl.bklasahero.ui.screens.FixturesScreen
import pl.bklasahero.ui.screens.HomeScreen
import pl.bklasahero.ui.screens.LoadingScreen
import pl.bklasahero.ui.screens.MatchScreen
import pl.bklasahero.ui.screens.NewCareerScreen
import pl.bklasahero.ui.screens.ResultScreen
import pl.bklasahero.ui.screens.ShootoutScreen
import pl.bklasahero.ui.screens.TableScreen
import pl.bklasahero.ui.screens.TutorialScreen

@Composable
fun AppNavGraph(state: AppUiState, viewModel: AppViewModel) {
    val dispatch = viewModel::dispatch
    when (state.screen) {
        AppScreen.Loading -> LoadingScreen(state, dispatch)
        AppScreen.Tutorial -> TutorialScreen(state, dispatch)
        AppScreen.Home -> HomeScreen(state, dispatch)
        AppScreen.NewCareer -> NewCareerScreen(state, viewModel)
        AppScreen.Career -> CareerScreen(state, viewModel)
        AppScreen.Match -> MatchScreen(state, dispatch)
        AppScreen.Shootout -> ShootoutScreen(state, viewModel)
        AppScreen.Defend -> MatchScreen(state, dispatch)
        AppScreen.Result -> ResultScreen(state, dispatch)
        AppScreen.Table -> TableScreen(state, dispatch)
        AppScreen.Fixtures -> FixturesScreen(state, dispatch)
    }
}