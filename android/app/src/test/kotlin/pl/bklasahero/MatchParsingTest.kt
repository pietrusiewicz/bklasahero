// Test JVM na PRAWDZIWYCH odpowiedziach rdzenia (zrzut z hosta, katalog 7778 miejscowości).
//
// Pilnuje kontraktu, na którym aplikacja się wywracała: `setup` i `resolution`
// przychodzą jako STRINGI z JSON-em w środku, a nie jako obiekty. Sięgnięcie po
// `.jsonObject` rzucało wyjątek w `viewModelScope` i kończyło proces — z punktu
// widzenia gracza „Rozpocznij rzuty karne" wyrzucało do menu.
//
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero

import kotlinx.serialization.json.Json
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.jsonObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import pl.bklasahero.ui.AppUiState

class MatchParsingTest {

    private val json = Json { ignoreUnknownKeys = true; isLenient = true }

    private fun obj(text: String): JsonObject = json.parseToJsonElement(text).jsonObject

    /** `data` z odpowiedzi `beginMatch` (prawdziwy payload, skrócony w polach nieużywanych). */
    private val setupPayload = obj(
        """
        {"setup":"{\"fixture\":{\"away\":1,\"home\":0,\"matchIndex\":0,\"round\":0},\"isDecisive\":false,
        \"leagueLabel\":\"B klasa · mazowieckie\",\"distanceKm\":17.84092874231727,
        \"opponent\":\"GKS Huragan Leszno\",\"opponentShort\":\"Huragan\",\"opponentTown\":\"Leszno\",
        \"playerClub\":\"MKS Pogoń Maków Mazowiecki\",\"playerClubShort\":\"Pogoń\",\"playerTown\":\"Maków Mazowiecki\",
        \"round\":0,\"seasonNumber\":1,\"rules\":{\"firstKicker\":\"side.home\",\"kicksPerSide\":5,
        \"maxSuddenDeathRounds\":10,\"suddenDeath\":false},
        \"map\":{\"homeLat\":52.86,\"homeLon\":21.10,\"awayLat\":52.05,\"awayLon\":19.75,
        \"places\":[{\"isPlayer\":true,\"lat\":52.86,\"lon\":21.10,\"short\":\"Pogoń\",\"town\":\"Maków Mazowiecki\"},
        {\"isPlayer\":false,\"lat\":52.05,\"lon\":19.75,\"short\":\"Huragan\",\"town\":\"Leszno\"}]}}"}
        """.trimIndent()
    )

    /** `data` z odpowiedzi `shoot` — `kick` to string, a `resolution` string w nim. */
    private val kickPayload = obj(
        """
        {"kick":"{\"keeperClubId\":0,\"playerRole\":\"role.shooter\",
        \"resolution\":\"{\\\"outcome\\\":\\\"outcome.goal\\\",\\\"scored\\\":true,\\\"crossedGoalLine\\\":true}\"}",
        "shootout":{"homeScore":1,"awayScore":0,"homeTaken":1,"awayTaken":0,"kicks":[
        {"sequence":0,"outcome":"outcome.goal","playerRole":"role.shooter","round":0,"scored":true,"side":"side.home"}]},
        "finished":false}
        """.trimIndent()
    )

    @Test
    fun matchSetupIsParsedFromNestedString() {
        val state = AppUiState().applyMatchSetup(setupPayload)
        val sb = state.scoreboard
        assertEquals("Pogoń", sb.home)
        assertEquals("Huragan", sb.away)
        assertEquals("Maków Mazowiecki", sb.homeTown)
        assertEquals("Leszno", sb.awayTown)
        assertEquals(17.84, sb.distanceKm, 0.01)
        assertEquals("B klasa · mazowieckie", sb.leagueLabel)
        assertEquals(5, sb.kicksPerSide)
        assertEquals(2, sb.mapPoints.size)
        assertTrue(sb.mapPoints.first().isPlayer)
    }

    @Test
    fun kickResultIsParsedFromDoublyNestedString() {
        val state = AppUiState().applyKickResult(kickPayload, "shooter")
        assertEquals("outcome.goal", state.lastOutcomeKey)
        assertEquals("shooter", state.lastKickRole)
        assertEquals(1, state.scoreboard.homeScore)
        assertEquals(listOf(true), state.scoreboard.homeKicks)
    }

    @Test
    fun brokenPayloadDoesNotThrow() {
        // Regresja: zła odpowiedź nie może wywalić aplikacji — najwyżej brak danych.
        val state = AppUiState().applyMatchSetup(obj("""{"setup":"to nie jest JSON"}"""))
        assertEquals("", state.scoreboard.home)
        assertNull(AppUiState().copy(lastOutcomeKey = null).applyKickResult(obj("{}"), "shooter").lastOutcomeKey)
    }
}
