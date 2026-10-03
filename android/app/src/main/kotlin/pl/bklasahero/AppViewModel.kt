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
import kotlinx.serialization.json.doubleOrNull
import kotlinx.serialization.json.buildJsonObject
import kotlinx.serialization.json.intOrNull
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive
import kotlinx.serialization.json.put
import pl.bklasahero.data.AppContainer
import pl.bklasahero.engine.FrameBuffer
import pl.bklasahero.engine.NativeBridge
import pl.bklasahero.ui.AppIntent
import pl.bklasahero.ui.AppScreen
import pl.bklasahero.ui.AppUiState
import pl.bklasahero.ui.MapPoint
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
                .onSuccess { data -> _uiState.update { it.copy(careerJson = data, careerReady = true, screen = AppScreen.Career, navStack = emptyList()) } }
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

    /** Rozpoczyna nowy mecz (rzuty karne), zeruje tablicę i wybiera pierwszą turę. */
    fun beginMatch() {
        FrameBuffer.clear()
        viewModelScope.launch {
            NativeBridge.command(buildJsonObject { put("cmd", "beginMatch") })
                .onSuccess { data ->
                    _uiState.update { it.applyMatchSetup(data) }
                    refreshTurn()
                }
                .onFailure { e -> _uiState.update { it.copy(toast = e.message) } }
        }
    }

    /** matchState → kto wykonuje następny rzut: gracz strzela (Shootout) albo broni (Defend). */
    fun refreshTurn() {
        viewModelScope.launch {
            NativeBridge.command(buildJsonObject { put("cmd", "matchState") })
                .onSuccess { data -> _uiState.update { it.applyTurn(data) } }
                .onFailure { e -> _uiState.update { it.copy(toast = e.message) } }
        }
    }

    /** Rzut karny gracza. `effort` [0.45..1] = siła (im mocniej, tym mniej precyzyjnie). */
    fun shoot(aimX: Float, aimY: Float, effort: Float) {
        viewModelScope.launch {
            val cmd = buildJsonObject {
                put("cmd", "shoot")
                put("aimX", aimX)
                put("aimY", aimY)
                put("effort", effort)
            }
            NativeBridge.command(cmd)
                .onSuccess { data ->
                    FrameBuffer.pull(FloatArray(0))
                    _uiState.update { it.applyKickResult(data, "shooter") }
                }
                .onFailure { e -> _uiState.update { it.copy(toast = e.message) } }
        }
    }

    /** Obrona gracza — rzut przeciwnika. side: Left/Center/Right, height: Low/Mid/High. */
    fun dive(side: String, height: String, timingErrorS: Float = 0f) {
        viewModelScope.launch {
            val cmd = buildJsonObject {
                put("cmd", "dive")
                put("side", side)
                put("height", height)
                put("timingErrorS", timingErrorS)
            }
            NativeBridge.command(cmd)
                .onSuccess { data ->
                    FrameBuffer.pull(FloatArray(0))
                    _uiState.update { it.applyKickResult(data, "keeper") }
                }
                .onFailure { e -> _uiState.update { it.copy(toast = e.message) } }
        }
    }

    /** Po animacji rzutu: następna tura albo zakończenie meczu. */
    fun continueAfterKick() {
        FrameBuffer.clear()
        if (_uiState.value.scoreboard.finished) finishMatch() else refreshTurn()
    }

    /** Zapisuje wynik meczu w lidze i wraca do menu kariery. */
    fun finishMatch() {
        viewModelScope.launch {
            NativeBridge.command(buildJsonObject { put("cmd", "finishMatch") })
                .onSuccess {
                    FrameBuffer.clear()
                    _uiState.update { it.copy(screen = AppScreen.Career, navStack = emptyList(), lastOutcomeKey = null) }
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

private val outcomeJson = kotlinx.serialization.json.Json {
    ignoreUnknownKeys = true
    isLenient = true
}

private fun str(obj: JsonObject, key: String): String =
    obj[key]?.jsonPrimitive?.content ?: ""

/** Pole w stylu `kick`/`resolution` jest w protokole STRINGIEM z JSON-em w środku. */
private fun nestedJson(data: JsonObject, key: String): JsonObject? {
    val raw = data[key]?.jsonPrimitive?.content ?: return null
    return try {
        outcomeJson.parseToJsonElement(raw).jsonObject
    } catch (e: Exception) {
        null
    }
}

private fun scoreboardFrom(sb: Scoreboard, so: JsonObject?): Scoreboard {
    // Wyniki kolejnych rzutów (kropki pod nazwami drużyn).
    val homeKicks = mutableListOf<Boolean>()
    val awayKicks = mutableListOf<Boolean>()
    so?.get("kicks")?.jsonArray?.forEach { el ->
        val o = el as? JsonObject ?: return@forEach
        val scored = o["scored"]?.jsonPrimitive?.booleanOrNull ?: false
        when (o["side"]?.jsonPrimitive?.content) {
            "side.home" -> homeKicks.add(scored)
            "side.away" -> awayKicks.add(scored)
        }
    }
    return sb.copy(
        homeScore = so?.get("homeScore")?.jsonPrimitive?.intOrNull ?: sb.homeScore,
        awayScore = so?.get("awayScore")?.jsonPrimitive?.intOrNull ?: sb.awayScore,
        homeTaken = so?.get("homeTaken")?.jsonPrimitive?.intOrNull ?: sb.homeTaken,
        awayTaken = so?.get("awayTaken")?.jsonPrimitive?.intOrNull ?: sb.awayTaken,
        homeKicks = homeKicks,
        awayKicks = awayKicks,
    )
}

private fun AppUiState.applyMatchSetup(data: JsonObject): AppUiState {
    val setup = data["setup"]?.jsonObject
    val home = setup?.let { str(it, "playerClubShort") } ?: ""
    val away = setup?.let { str(it, "opponentShort") } ?: ""
    val kps = setup?.get("rules")?.jsonObject?.get("kicksPerSide")?.jsonPrimitive?.intOrNull ?: 5
    return copy(
        scoreboard = Scoreboard(
            home = home,
            away = away,
            // Zapowiedź meczu: miejscowości i odległość między nimi.
            homeTown = setup?.let { str(it, "playerTown") } ?: "",
            awayTown = setup?.let { str(it, "opponentTown") } ?: "",
            distanceKm = setup?.get("distanceKm")?.jsonPrimitive?.doubleOrNull ?: 0.0,
            mapPoints = setup?.get("map")?.jsonObject?.get("places")?.jsonArray
                ?.mapNotNull { el ->
                    val o = el as? JsonObject ?: return@mapNotNull null
                    MapPoint(
                        lat = o["lat"]?.jsonPrimitive?.doubleOrNull ?: return@mapNotNull null,
                        lon = o["lon"]?.jsonPrimitive?.doubleOrNull ?: return@mapNotNull null,
                        town = str(o, "town"),
                        short = str(o, "short"),
                        isPlayer = o["isPlayer"]?.jsonPrimitive?.booleanOrNull ?: false,
                    )
                }
                .orEmpty(),
            leagueLabel = setup?.let { str(it, "leagueLabel") } ?: "",
            round = setup?.get("round")?.jsonPrimitive?.intOrNull ?: 0,
            isDecisive = setup?.get("isDecisive")?.jsonPrimitive?.booleanOrNull ?: false,
            kicksPerSide = kps,
        ),
        lastOutcomeKey = null,
        lastKickRole = null,
    )
}

/** matchState → routing na strzał albo obronę (gracz zawsze = side.home). */
private fun AppUiState.applyTurn(data: JsonObject): AppUiState {
    val kicker = data["nextKicker"]?.jsonPrimitive?.content ?: ""
    val finished = data["finished"]?.jsonPrimitive?.booleanOrNull ?: false
    return copy(
        scoreboard = scoreboardFrom(scoreboard, data["shootout"]?.jsonObject).copy(finished = finished),
        screen = if (kicker == "side.away") AppScreen.Defend else AppScreen.Shootout,
    )
}

/** Odpowiedź shoot/dive → aktualizacja tablicy + przejście na ekran wyniku. */
private fun AppUiState.applyKickResult(data: JsonObject, role: String): AppUiState {
    val so = data["shootout"]?.jsonObject
    val finished = data["finished"]?.jsonPrimitive?.booleanOrNull ?: false
    val resolution = nestedJson(data, "resolution")
    val outcomeKey = resolution?.get("outcome")?.jsonPrimitive?.content
    return copy(
        scoreboard = scoreboardFrom(scoreboard, so).copy(finished = finished),
        lastOutcomeKey = outcomeKey,
        lastKickRole = role,
        screen = AppScreen.Result,
    )
}