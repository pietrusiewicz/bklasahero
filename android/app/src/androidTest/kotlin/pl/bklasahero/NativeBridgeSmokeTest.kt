// Test instrumentowany (na urządzeniu/emulatorze) — smoke test mostka JNI.
// Weryfikuje, że libbkh_bridge.so się ładuje i rdzeń odpowiada na polecenia.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero

import androidx.test.ext.junit.runners.AndroidJUnit4
import kotlinx.serialization.json.buildJsonObject
import kotlinx.serialization.json.put
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import pl.bklasahero.engine.NativeBridge

@RunWith(AndroidJUnit4::class)
class NativeBridgeSmokeTest {

    @Test
    fun protocolVersionIsReadable() {
        // Wersja protokołu to int >= 1; sam odczyt wymusza załadowanie .so.
        assertTrue(NativeBridge.protocolVersion >= 1)
    }

    @Test
    fun saveSchemaVersionIsReadable() {
        assertTrue(NativeBridge.saveSchemaVersion >= 1)
    }

    @Test
    fun coreVersionIsSemverish() {
        // Np. "0.1.0" — trzy segmenty oddzielone kropkami.
        val v = NativeBridge.coreVersion
        assertTrue("core version '$v' has 3 segments", v.split('.').size == 3)
    }

    @Test
    fun languageCommandSucceeds() {
        val res = NativeBridge.command(buildJsonObject { put("cmd", "setLanguage"); put("code", "pl") })
        assertTrue("setLanguage should succeed: $res", res.isSuccess)
    }

    @Test
    fun unknownCommandFailsGracefully() {
        val res = NativeBridge.command(buildJsonObject { put("cmd", "nope_not_a_command") })
        assertTrue("unknown command should fail", res.isFailure)
    }
}