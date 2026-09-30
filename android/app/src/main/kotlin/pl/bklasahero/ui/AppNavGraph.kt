// Główny graf nawigacji — używamy Compose Navigation.
// Dla 5 ekranów nie potrzeba deep-linków, wystarczy proste route.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui

import androidx.compose.runtime.Composable
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import pl.bklasahero.ui.screens.CareerScreen
import pl.bklasahero.ui.screens.FixturesScreen
import pl.bklasahero.ui.screens.HomeScreen
import pl.bklasahero.ui.screens.LoadingScreen
import pl.bklasahero.ui.screens.MatchScreen
import pl.bklasahero.ui.screens.NewCareerScreen
import pl.bklasahero.ui.screens.ResultScreen
import pl.bklasahero.ui.screens.ShootoutScreen
import pl.bklasahero.ui.screens.TableScreen

@Composable
fun AppNavGraph(state: AppUiState, dispatch: (AppIntent) -> Unit) {
    val navController = rememberNavController()
    NavHost(navController, startDestination = routeOf(state.screen)) {
        composable("loading") { LoadingScreen(state, dispatch) }
        composable("home") { HomeScreen(state, dispatch) }
        composable("new_career") { NewCareerScreen(state, dispatch) }
        composable("career") { CareerScreen(state, dispatch) }
        composable("match") { MatchScreen(state, dispatch) }
        composable("shootout") { ShootoutScreen(state, dispatch) }
        composable("result") { ResultScreen(state, dispatch) }
        composable("table") { TableScreen(state, dispatch) }
        composable("fixtures") { FixturesScreen(state, dispatch) }
    }
}

private fun routeOf(screen: AppScreen): String = when (screen) {
    AppScreen.Loading -> "loading"
    AppScreen.Home -> "home"
    AppScreen.NewCareer -> "new_career"
    AppScreen.Career -> "career"
    AppScreen.Match -> "match"
    AppScreen.Shootout -> "shootout"
    AppScreen.Defend -> "match"  // w MVP ten sam ekran, inny overlay
    AppScreen.Result -> "result"
    AppScreen.Table -> "table"
    AppScreen.Fixtures -> "fixtures"
}