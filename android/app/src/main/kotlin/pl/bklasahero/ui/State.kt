// UI: stan, intencje, reduktor.
// Każdy ekran buduje stan tylko przez `dispatch`, a stan jest immutable —
// Compose łatwo wykrywa zmiany.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui

import kotlinx.serialization.json.JsonObject
import pl.bklasahero.data.AppContainer

/** Wszystkie ekrany. */
enum class AppScreen { Loading, Home, NewCareer, Career, Match, Shootout, Defend, Result, Table, Fixtures }

data class AppUiState(
    val screen: AppScreen = AppScreen.Loading,
    val darkTheme: Boolean = false,
    val protocolVersion: Int = -1,
    val saveSchemaVersion: Int = -1,
    val toast: String? = null,
    val careerJson: JsonObject? = null,
    val careerReady: Boolean = false,
    val pendingKick: PendingKick? = null,
    val lastResponse: JsonObject? = null,
)

data class PendingKick(
    val round: Int,
    val isUserShooting: Boolean,
)

sealed interface AppIntent {
    data object ContinueLoading : AppIntent
    data class SetScreen(val screen: AppScreen) : AppIntent
    data class ShowToast(val text: String?) : AppIntent
    data class SetCareer(val json: JsonObject) : AppIntent
    data class StartNewCareer(val nickname: String, val homeOsmId: Int) : AppIntent
    data class BeginMatch(val opponentName: String) : AppIntent
    data class Shoot(val aimX: Float, val aimY: Float, val effort: Float) : AppIntent
    data class Dive(val side: String, val height: String) : AppIntent
    data object FinishMatch : AppIntent
    data object SaveCareer : AppIntent
    data object LoadCareer : AppIntent
    data object OpenTable : AppIntent
    data object OpenFixtures : AppIntent
    data object ToggleTheme : AppIntent
}

fun reduce(state: AppUiState, intent: AppIntent, container: AppContainer): AppUiState =
    when (intent) {
        AppIntent.ContinueLoading -> state.copy(screen = AppScreen.Home)
        is AppIntent.SetScreen -> state.copy(screen = intent.screen)
        is AppIntent.ShowToast -> state.copy(toast = intent.text)
        is AppIntent.SetCareer -> state.copy(careerJson = intent.json, careerReady = true)
        is AppIntent.StartNewCareer -> {
            // Wywołanie polecenia "newCareer" jest zrobione przez DispatcherAsync w warstwie wywołującej.
            state.copy(screen = AppScreen.Career)
        }
        is AppIntent.BeginMatch -> state.copy(screen = AppScreen.Match)
        is AppIntent.Shoot -> state.copy(screen = AppScreen.Shootout)
        is AppIntent.Dive -> state.copy(screen = AppScreen.Defend)
        AppIntent.FinishMatch -> state.copy(screen = AppScreen.Result)
        AppIntent.SaveCareer -> {
            container.careerRepository.persistCurrent()
            state.copy(toast = "Zapisano")
        }
        AppIntent.LoadCareer -> state
        AppIntent.OpenTable -> state.copy(screen = AppScreen.Table)
        AppIntent.OpenFixtures -> state.copy(screen = AppScreen.Fixtures)
        AppIntent.ToggleTheme -> state.copy(darkTheme = !state.darkTheme)
    }