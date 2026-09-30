// Fasada JNI — jeden punkt wejścia do rdzenia C++ dla reszty aplikacji.
// Cała reszta kodu Kotlin NIE używa bezpośrednio JNI: rozmawia z tą klasą.
//
// Konwencja: każda metoda jest totalna (nie rzuca po stronie Kotlin, tylko
// zwraca null lub Result.failure), żeby warstwa UI mogła ją wołać z
// dowolnego dispatcher'a i nie musiała pamiętać o try/catch.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.engine

import androidx.annotation.MainThread
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

/** Wyjątek wewnętrzny — nie powinien nigdy opuścić fasady. */
internal class NativeBridgeException(message: String) : RuntimeException(message)

/** Odpowiedź fasady: albo sukces (data), albo błąd (code/message). */
sealed class BridgeResponse<out T> {
    data class Ok<T>(val data: T) : BridgeResponse<T>()
    data class Err(val code: String, val message: String) : BridgeResponse<Nothing>()
}

/**
 * Singleton łączący się z libbkh_bridge.so. Pierwsze wywołanie inicjuje
 * fasadę C++ (tworzy globalną instancję bkh::Facade).
 */
object NativeBridge {

    @Volatile
    private var initialized = false

    private val json = Json {
        ignoreUnknownKeys = true
        isLenient = true
        encodeDefaults = true
    }

    /** Wersja protokołu JSON (z dokumentu PROTOCOL.md). */
    val protocolVersion: Int
        @MainThread
        get() {
            ensureLoaded()
            return nativeProtocolVersion()
        }

    /** Wersja schematu save'a — bumpowana przy każdej migracji JSON. */
    val saveSchemaVersion: Int
        @MainThread
        get() {
            ensureLoaded()
            return nativeSaveSchemaVersion()
        }

    /** Np. "0.1.0" — z BKH_VERSION_STRING z CMake. */
    val coreVersion: String
        @MainThread
        get() {
            ensureLoaded()
            return nativeCoreVersion()
        }

    /** Ładuje listę miejscowości z bufora bajtów (np. z assets/places_pl.csv). */
    fun loadPlacesCsv(bytes: ByteArray): Result<Unit> = runCatchingUnit {
        ensureLoaded()
        if (!nativeLoadPlacesCsv(bytes)) {
            throw NativeBridgeException("places CSV load failed")
        }
    }

    /** Wysyła polecenie JSON, zwraca sparsowaną odpowiedź JSON. */
    fun command(payload: JsonObject): Result<JsonObject> = runCatching {
        ensureLoaded()
        val text = json.encodeToString(JsonObject.serializer(), payload)
        val response = nativeCommand(text)
        parseResponse(response)
    }

    /** Zapis gry → JSON (surowy blob poza protokołem poleceń). */
    fun saveCareer(savedAtEpochMs: Long = System.currentTimeMillis()): Result<String> =
        runCatchingString { nativeSaveCareer(savedAtEpochMs) }

    /** Wczytanie save'a — zwraca true przy powodzeniu. */
    fun loadCareer(json: String): Result<Unit> = runCatchingUnit {
        if (!nativeLoadCareer(json)) {
            throw NativeBridgeException("career load failed")
        }
    }

    private fun parseResponse(raw: String): JsonObject {
        val root = json.parseToJsonElement(raw).jsonObject
        if (root["ok"]?.jsonPrimitive?.content == "true") {
            val data = root["data"]?.jsonObject
            requireNotNull(data) { "missing data in ok response" }
            return data
        }
        val error = root["error"]?.jsonObject
            ?: error("missing error in failure response")
        val code = error["code"]?.jsonPrimitive?.content ?: "error.unknown"
        val message = error["message"]?.jsonPrimitive?.content ?: ""
        throw NativeBridgeException("$code: $message")
    }

    private fun ensureLoaded() {
        if (initialized) return
        synchronized(this) {
            if (initialized) return
            System.loadLibrary("bkh_bridge")
            initialized = true
        }
    }

    // --- Metody native -----------------------------------------------------

    private external fun nativeCommand(cmd: String): String
    private external fun nativeSaveCareer(savedAtEpochMs: Long): String
    private external fun nativeLoadCareer(json: String): Boolean
    private external fun nativeLoadPlacesCsv(bytes: ByteArray): Boolean
    private external fun nativeProtocolVersion(): Int
    private external fun nativeSaveSchemaVersion(): Int
    private external fun nativeCoreVersion(): String
    private external fun nativeFrameFloatCount(): Int
    private external fun nativeWriteFrame(target: FloatArray): Int
    private external fun nativeClearPlayback()
    private external fun nativeHasPlayback(): Boolean

    // --- Kanał renderujący (publiczne wrappery) ---------------------------

    /** Minimalny rozmiar bufora klatki (0 = brak odtwarzania). */
    fun frameFloatCount(): Int {
        ensureLoaded()
        return nativeFrameFloatCount()
    }

    /** Zwraca liczbę zapisanych floatów, lub wartość ujemną gdy bufor za mały. */
    fun writeFrame(target: FloatArray): Int {
        ensureLoaded()
        return nativeWriteFrame(target)
    }

    fun clearPlayback() {
        ensureLoaded()
        nativeClearPlayback()
    }

    fun hasPlayback(): Boolean {
        ensureLoaded()
        return nativeHasPlayback()
    }

    // --- Wynikowe helpers --------------------------------------------------

    private inline fun <T> runCatchingUnit(block: () -> T): Result<Unit> = try {
        block()
        Result.success(Unit)
    } catch (e: NativeBridgeException) {
        Result.failure(e)
    } catch (e: Throwable) {
        Result.failure(e)
    }

    private inline fun runCatchingString(block: () -> String): Result<String> = try {
        Result.success(block())
    } catch (e: Throwable) {
        Result.failure(e)
    }
}