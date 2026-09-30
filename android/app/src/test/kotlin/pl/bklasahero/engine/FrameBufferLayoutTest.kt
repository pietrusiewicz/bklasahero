// Test JVM (bez urządzenia) — pilnuje zgodności układu bufora klatek
// pomiędzy Kotlin (FrameBuffer.kt) a C++ (bridge/include/bkh_bridge/frame_buffer.h).
// Obie strony MUSZĄ trzymać te same stałe, inaczej renderer źle odczyta klatki.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.engine

import org.junit.Assert.assertEquals
import org.junit.Test

class FrameBufferLayoutTest {

    @Test
    fun headerLayoutMatchesBridge() {
        assertEquals(8, FrameBuffer.HEADER_FLOATS)
        assertEquals(7, FrameBuffer.BALL_FLOATS_PER_SAMPLE)
        assertEquals(6, FrameBuffer.KEEPER_FLOATS_PER_SAMPLE)
        assertEquals(1, FrameBuffer.LAYOUT_VERSION)
    }

    @Test
    fun requiredFloatsFormulaMatchesBridge() {
        // Mirrors bkh::jni::requiredFloats(ballSamples, keeperSamples).
        fun required(ball: Int, keeper: Int): Int =
            FrameBuffer.HEADER_FLOATS +
                ball * FrameBuffer.BALL_FLOATS_PER_SAMPLE +
                keeper * FrameBuffer.KEEPER_FLOATS_PER_SAMPLE

        assertEquals(8, required(0, 0))
        assertEquals(8 + 7, required(1, 0))
        assertEquals(8 + 6, required(0, 1))
        assertEquals(8 + 5 * 7 + 5 * 6, required(5, 5))
    }

    @Test
    fun emptyFrameHasNoPlayback() {
        // Bez native load: obiekt FrameBuffer nie woła JNI przy samym odczycie.
        assertEquals(false, FrameBuffer.hasPlayback)
        assertEquals(0, FrameBuffer.ballSamples)
        assertEquals(0, FrameBuffer.keeperSamples)
        assertEquals(0f, FrameBuffer.durationS, 0.0001f)
    }
}