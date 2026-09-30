// Bufor klatek — wejście kanału renderującego (mirror stałych z bkh::frame).
// Wypełnia go C++ za każdym razem, gdy jest coś do odtworzenia (rzut, obrona).
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.engine

/**
 * Klatka animacji — wynik ostatniego rzutu/obrony.
 *
 * Bufor jest SUROWĄ tablicą float[]. Stałe układu:
 *
 *  [0]                                  layoutVersion
 *  [1]                                  ballSamples
 *  [2]                                  keeperSamples
 *  [3]                                  durationS
 *  [4]                                  contactTimeS (0 = brak)
 *  [5]                                  crossingTimeS (0 = brak)
 *  [6]                                  impactX
 *  [7]                                  impactY
 *  [8 .. 8+ballSamples*7)               próbki piłki: t, x, y, z, vx, vy, vz
 *  [8+ballSamples*7 ..)                 próbki bramkarza: t, x, y, z, diveProgress, side
 */
object FrameBuffer {

    const val LAYOUT_VERSION = 1
    const val HEADER_FLOATS = 8
    const val BALL_FLOATS_PER_SAMPLE = 7
    const val KEEPER_FLOATS_PER_SAMPLE = 6

    /**
     * Bufor ostatniej klatki — readonly od strony UI.
     * Mutowany wyłącznie przez [pull].
     */
    var floats: FloatArray = FloatArray(0)
        private set

    /** Czy są jakieś dane do odtworzenia. */
    val hasPlayback: Boolean
        get() = floats.isNotEmpty()

    /**
     * Pobrać klatkę z mostka JNI. Zwraca true gdy cokolwiek zapisano.
     * @param scratchBuffer — wyjściowa tablica, zostanie powiększona w razie
     *   potrzeby (koszt jednej alokacji).
     */
    fun pull(scratchBuffer: FloatArray): Boolean {
        val required = NativeBridge.frameFloatCount()
        if (required <= 0) {
            floats = FloatArray(0)
            return false
        }
        val size = if (scratchBuffer.size >= required) scratchBuffer.size else required
        val buffer = if (scratchBuffer.size >= required) scratchBuffer else FloatArray(required)
        val written = NativeBridge.writeFrame(buffer)
        require(written >= 0) { "writeFrame reported buffer too small: $written" }
        floats = buffer.copyOf(written)
        return written > 0
    }

    /** Po zakończeniu animacji czyścimy odtwarzanie po stronie rdzenia. */
    fun clear() {
        NativeBridge.clearPlayback()
        floats = FloatArray(0)
    }

    /** Pierwsze 8 floatów → nagłówek klatki. */
    val layoutVersion: Float get() = floats.getOrElse(0) { 0f }
    val ballSamples: Int get() = floats.getOrElse(1) { 0f }.toInt()
    val keeperSamples: Int get() = floats.getOrElse(2) { 0f }.toInt()
    val durationS: Float get() = floats.getOrElse(3) { 0f }
    val contactTimeS: Float get() = floats.getOrElse(4) { 0f }
    val crossingTimeS: Float get() = floats.getOrElse(5) { 0f }
    val impactX: Float get() = floats.getOrElse(6) { 0f }
    val impactY: Float get() = floats.getOrElse(7) { 0f }
}