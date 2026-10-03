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
    /**
     * Ścieżka nawigacji (bez ekranu bieżącego). Pozwala cofnąć się tam, skąd
     * przyszliśmy, zamiast zawsze wracać do menu — inaczej systemowy przycisk
     * „wstecz” zamyka aplikację z ekranu tabeli czy terminarza.
     */
    val navStack: List<AppScreen> = emptyList(),
    /** Wybrana zakładka menu głównego: 0 = tabela, 1 = terminarz, 2 = kariera. */
    val homeTab: Int = 0,
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
    /** Miejscowości obu klubów — zapowiedź meczu pokazuje, skąd jedzie rywal. */
    val homeTown: String = "",
    val awayTown: String = "",
    /** Odległość między miejscowościami [km] (0 = brak danych). */
    val distanceKm: Double = 0.0,
    /** Kluby ligi z pozycjami — z tego powstaje mapka w zapowiedzi meczu. */
    val mapPoints: List<MapPoint> = emptyList(),
    val leagueLabel: String = "",
    val round: Int = 0,
    /** Ostatnie kolejki sezonu — mecz „o awans". */
    val isDecisive: Boolean = false,
    val homeScore: Int = 0,
    val awayScore: Int = 0,
    val homeTaken: Int = 0,
    val awayTaken: Int = 0,
    val kicksPerSide: Int = 5,
    val finished: Boolean = false,
    /** Wynik kolejnych rzutów drużyny: true = gol (zielona kropka), false = pudło (czerwona). */
    val homeKicks: List<Boolean> = emptyList(),
    val awayKicks: List<Boolean> = emptyList(),
)

/** Pojedynczy punkt na schematycznej mapce zapowiedzi meczu. */
data class MapPoint(
    val lat: Double,
    val lon: Double,
    val town: String,
    val short: String,
    val isPlayer: Boolean,
)

data class PendingKick(
    val round: Int,
    val isUserShooting: Boolean,
)

sealed interface AppIntent {
    data object ContinueLoading : AppIntent
    data class SetScreen(val screen: AppScreen) : AppIntent
    data object GoBack : AppIntent
    data class SetHomeTab(val index: Int) : AppIntent
    data class ShowToast(val text: String?) : AppIntent
    data class SetCareer(val json: JsonObject) : AppIntent
    data object SaveCareer : AppIntent
    data object ToggleTheme : AppIntent
    data object FinishTutorial : AppIntent
    data class SetScoreboard(val scoreboard: Scoreboard) : AppIntent
}

/** Maksymalna głębokość stosu — chroni przed pętlą „w przód i w tył”. */
private const val kMaxNavStack = 12

/** Dokłada bieżący ekran na stos (Loading i powtórki pomijamy). */
private fun pushScreen(stack: List<AppScreen>, current: AppScreen): List<AppScreen> {
    if (current == AppScreen.Loading || stack.lastOrNull() == current) return stack
    val trimmed = if (stack.size >= kMaxNavStack) stack.drop(1) else stack
    return trimmed + current
}

/**
 * Czysty reduktor — wyłącznie przejścia stanu. Skutki uboczne (NativeBridge,
 * Room) wykonują ViewModel / ekrany przed wywołaniem dispatch.
 */
fun reduce(state: AppUiState, intent: AppIntent, container: AppContainer): AppUiState =
    when (intent) {
        AppIntent.ContinueLoading -> state.copy(screen = AppScreen.Home, navStack = emptyList())
        is AppIntent.SetScreen ->
            if (intent.screen == state.screen) state
            else state.copy(screen = intent.screen, navStack = pushScreen(state.navStack, state.screen))
        AppIntent.GoBack -> state.copy(
            screen = state.navStack.lastOrNull() ?: AppScreen.Home,
            navStack = state.navStack.dropLast(1),
        )
        is AppIntent.ShowToast -> state.copy(toast = intent.text)
        is AppIntent.SetHomeTab -> state.copy(homeTab = intent.index)
        is AppIntent.SetCareer -> state.copy(careerJson = intent.json, careerReady = true)
        AppIntent.SaveCareer -> {
            container.careerRepository.persistCurrent()
            state.copy(toast = "saved")
        }
        AppIntent.ToggleTheme -> state.copy(darkTheme = !state.darkTheme)
        AppIntent.FinishTutorial -> {
            container.markTutorialSeen()
            // Po samouczku: nowy gracz wybiera miasto, powracający wraca do menu.
            state.copy(
                screen = if (state.careerReady) AppScreen.Home else AppScreen.NewCareer,
                navStack = emptyList(),
            )
        }
        is AppIntent.SetScoreboard -> state.copy(scoreboard = intent.scoreboard)
    }