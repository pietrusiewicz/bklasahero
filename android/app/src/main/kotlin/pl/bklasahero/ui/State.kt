// UI: stan, intencje, reduktor.
// Każdy ekran buduje stan tylko przez `dispatch`, a stan jest immutable —
// Compose łatwo wykrywa zmiany.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui

import kotlinx.serialization.json.JsonObject
import pl.bklasahero.data.AppContainer

/** Wszystkie ekrany. */
enum class AppScreen { Loading, Tutorial, Home, NewCareer, Career, Match, Shootout, Defend, Result, Table, Fixtures }

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
    val scoreboard: Scoreboard = Scoreboard(),
    val lastOutcomeKey: String? = null,
    val lastKickRole: String? = null,
)

/** Tablica wyników pokazywana na górze ekranu meczu. */
data class Scoreboard(
    val home: String = "",
    val away: String = "",
    val homeScore: Int = 0,
    val awayScore: Int = 0,
    val homeTaken: Int = 0,
    val awayTaken: Int = 0,
    val kicksPerSide: Int = 5,
    val finished: Boolean = false,
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
    data object SaveCareer : AppIntent
    data object ToggleTheme : AppIntent
    data object FinishTutorial : AppIntent
    data class SetScoreboard(val scoreboard: Scoreboard) : AppIntent
}

/**
 * Czysty reduktor — wyłącznie przejścia stanu. Skutki uboczne (NativeBridge,
 * Room) wykonują ViewModel / ekrany przed wywołaniem dispatch.
 */
fun reduce(state: AppUiState, intent: AppIntent, container: AppContainer): AppUiState =
    when (intent) {
        AppIntent.ContinueLoading -> state.copy(screen = AppScreen.Home)
        is AppIntent.SetScreen -> state.copy(screen = intent.screen)
        is AppIntent.ShowToast -> state.copy(toast = intent.text)
        is AppIntent.SetCareer -> state.copy(careerJson = intent.json, careerReady = true)
        AppIntent.SaveCareer -> {
            container.careerRepository.persistCurrent()
            state.copy(toast = "saved")
        }
        AppIntent.ToggleTheme -> state.copy(darkTheme = !state.darkTheme)
        AppIntent.FinishTutorial -> {
            container.markTutorialSeen()
            state.copy(screen = AppScreen.Home)
        }
        is AppIntent.SetScoreboard -> state.copy(scoreboard = intent.scoreboard)
    }