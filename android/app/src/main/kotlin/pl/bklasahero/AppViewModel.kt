// ViewModel trzymający globalny stan UI. Każda zmiana ekranu idzie przez
// `dispatch` (czyste przejścia), a operacje asynchroniczne przez
// `initialize` / metody `on...` wywoływane z ekranów.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.booleanOrNull
import kotlinx.serialization.json.buildJsonObject
import kotlinx.serialization.json.intOrNull
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive
import kotlinx.serialization.json.put
import pl.bklasahero.data.AppContainer
import pl.bklasahero.engine.FrameBuffer
import pl.bklasahero.engine.NativeBridge
import pl.bklasahero.ui.AppIntent
import pl.bklasahero.ui.AppScreen
import pl.bklasahero.ui.AppUiState
import pl.bklasahero.ui.Scoreboard
import pl.bklasahero.ui.reduce

class AppViewModel(private val container: AppContainer) : ViewModel() {

    private val _uiState = MutableStateFlow(AppUiState())
    val uiState: StateFlow<AppUiState> = _uiState

    fun dispatch(intent: AppIntent) {
        _uiState.update { reduce(it, intent, container) }
    }

    /** Sekwencja startowa: wersje → CSV miejscowości → save z bazy → Home. */
    fun initialize() {
        _uiState.update {
            it.copy(
                protocolVersion = NativeBridge.protocolVersion,
                saveSchemaVersion = NativeBridge.saveSchemaVersion,
            )
        }
        viewModelScope.launch {
            val places = container.placesRepository.loadFromAssets()
            if (places.isFailure) {
                _uiState.update { it.copy(screen = AppScreen.Loading, toast = "places.load_failed") }
                return@launch
            }
            // Przywróć karierę, jeśli istnieje (suspend — wywołujemy wprost,
            // jesteśmy już w coroutine `launch`).
            val restored = try {
                container.careerRepository.loadFromDatabase()
            } catch (e: Throwable) {
                false
            }
            _uiState.update {
                it.copy(
                    // Pierwsze uruchomienie → samouczek; kolejne → prosto do menu.
                    screen = if (container.isTutorialSeen()) AppScreen.Home else AppScreen.Tutorial,
                    careerReady = restored,
                )
            }
        }
    }

    /** Polecenie "newCareer" → wgrywa świeżą karierę. */
    fun startNewCareer(nickname: String, homeOsmId: Int) {
        viewModelScope.launch {
            val cmd = buildJsonObject {
                put("cmd", "newCareer")
                put("nickname", nickname)
                put("homeOsmId", homeOsmId)
            }
            NativeBridge.command(cmd)
                .onSuccess { data -> _uiState.update { it.copy(careerJson = data, careerReady = true, screen = AppScreen.Career) } }
                .onFailure { e -> _uiState.update { it.copy(toast = e.message) } }
        }
    }

    /** Polecenie "career" → odświeża podsumowanie kariery. */
    fun refreshCareer() {
        viewModelScope.launch {
            NativeBridge.command(buildJsonObject { put("cmd", "career") })
                .onSuccess { data -> _uiState.update { it.copy(careerJson = data) } }
                .onFailure { e -> _uiState.update { it.copy(toast = e.message) } }
        }
    }

    /** Rozpoczyna nowy mecz (rzuty karne) i zeruje tablicę wyników. */
    fun beginMatch() {
        viewModelScope.launch {
            NativeBridge.command(buildJsonObject { put("cmd", "beginMatch") })
                .onSuccess { data -> _uiState.update { it.applyMatchSetup(data) } }
                .onFailure { e -> _uiState.update { it.copy(toast = e.message) } }
        }
    }

    /** Wykonuje rzut karny gracza i aktualizuje tablicę wyników. */
    fun shoot(aimX: Float, aimY: Float) {
        viewModelScope.launch {
            val cmd = buildJsonObject {
                put("cmd", "shoot")
                put("aimX", aimX)
                put("aimY", aimY)
                put("effort", 0.7f)
            }
            NativeBridge.command(cmd)
                .onSuccess { data ->
                    FrameBuffer.pull(FloatArray(0))
                    _uiState.update { it.applyShootResult(data) }
                }
                .onFailure { e -> _uiState.update { it.copy(toast = e.message) } }
        }
    }

    class Factory(private val container: AppContainer) : ViewModelProvider.Factory {
        @Suppress("UNCHECKED_CAST")
        override fun <T : ViewModel> create(modelClass: Class<T>): T {
            return AppViewModel(container) as T
        }
    }
}

// --- Parsowanie odpowiedzi meczu (czyste funkcje na AppUiState) ------------

private fun str(obj: JsonObject, key: String): String =
    obj[key]?.jsonPrimitive?.content ?: ""

private fun AppUiState.applyMatchSetup(data: JsonObject): AppUiState {
    val setup = data["setup"]?.jsonObject
    val home = setup?.let { str(it, "playerClubShort") } ?: ""
    val away = setup?.let { str(it, "opponentShort") } ?: ""
    val kps = setup?.get("rules")?.jsonObject?.get("kicksPerSide")?.jsonPrimitive?.intOrNull ?: 5
    return copy(
        scoreboard = Scoreboard(home = home, away = away, kicksPerSide = kps),
        screen = AppScreen.Shootout,
    )
}

private fun AppUiState.applyShootResult(data: JsonObject): AppUiState {
    val so = data["shootout"]?.jsonObject
    val finished = data["finished"]?.jsonPrimitive?.booleanOrNull ?: false
    val sb = scoreboard
    return copy(
        scoreboard = sb.copy(
            homeScore = so?.get("homeScore")?.jsonPrimitive?.intOrNull ?: sb.homeScore,
            awayScore = so?.get("awayScore")?.jsonPrimitive?.intOrNull ?: sb.awayScore,
            homeTaken = so?.get("homeTaken")?.jsonPrimitive?.intOrNull ?: sb.homeTaken,
            awayTaken = so?.get("awayTaken")?.jsonPrimitive?.intOrNull ?: sb.awayTaken,
            finished = finished,
        ),
        screen = AppScreen.Result,
    )
}