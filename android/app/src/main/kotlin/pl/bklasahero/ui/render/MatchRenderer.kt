// Pseudo-3D renderer boiska — wszystko w jednym Canvas.
// Animacja odtwarza klatkę z FrameBuffer, w czasie rzeczywistym rysuje tor
// piłki i ruch bramkarza. Brak prawdziwego 3D — perspektywa liczona ręcznie.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.render

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import pl.bklasahero.engine.FrameBuffer

@Composable
fun MatchRenderer(@Suppress("UNUSED_PARAMETER") state: pl.bklasahero.ui.AppUiState) {
    var t by remember { mutableStateOf(0f) }

    // Animacja: jeśli są próbki w FrameBuffer, odtwarzamy je w pętli.
    LaunchedEffect(FrameBuffer.hasPlayback) {
        if (FrameBuffer.floats.size > FrameBuffer.HEADER_FLOATS) {
            val duration = FrameBuffer.durationS.coerceAtLeast(0.5f)
            val start = withFrameNanos { it }
            while (true) {
                withFrameNanos { nowNanos ->
                    val elapsed = (nowNanos - start) / 1_000_000_000.0f
                    t = (elapsed / duration).coerceIn(0f, 1f)
                }
            }
        } else {
            t = 0f
        }
    }

    Canvas(modifier = Modifier.fillMaxSize()) {
        drawPitch()
        drawGoal()
        drawKeeper(t)
        drawBall(t)
    }
}

private fun DrawScope.drawPitch() {
    val w = size.width
    val h = size.height
    val horizon = h * 0.55f
    // Murawa — zielony prostokąt zbieżny do horyzontu.
    drawRect(color = Color(0xFF2C8E3F), topLeft = Offset(0f, horizon), size = Size(w, h - horizon))
    // Linie boczne i środkowa (zbieżność w punkcie na horyzoncie).
    val center = w / 2f
    drawLine(Color.White, Offset(0f, horizon), Offset(center, h * 0.5f), strokeWidth = 2f)
    drawLine(Color.White, Offset(w, horizon), Offset(center, h * 0.5f), strokeWidth = 2f)
    drawLine(Color.White, Offset(0f, h - 20f), Offset(w, h - 20f), strokeWidth = 3f)
    drawLine(Color.White, Offset(center, horizon), Offset(center, h), strokeWidth = 2f)
}

private fun DrawScope.drawGoal() {
    val w = size.width
    val h = size.height
    val goalWidthPx = w * 0.55f
    val goalHeightPx = h * 0.25f
    val goalBottom = h * 0.55f
    val left = (w - goalWidthPx) / 2f
    val right = left + goalWidthPx
    val top = goalBottom - goalHeightPx
    // Siatka.
    val netColor = Color(0xFFF7F7F2)
    val netStroke = 1.2f
    val cols = 12
    val rows = 6
    for (c in 0..cols) {
        val x = left + (goalWidthPx * c / cols)
        drawLine(netColor, Offset(x, top), Offset(x, goalBottom), strokeWidth = netStroke)
    }
    for (r in 0..rows) {
        val y = top + (goalHeightPx * r / rows)
        drawLine(netColor, Offset(left, y), Offset(right, y), strokeWidth = netStroke)
    }
    // Słupki i poprzeczka.
    val postColor = Color.White
    drawLine(postColor, Offset(left, top), Offset(left, goalBottom), strokeWidth = 4f)
    drawLine(postColor, Offset(right, top), Offset(right, goalBottom), strokeWidth = 4f)
    drawLine(postColor, Offset(left, top), Offset(right, top), strokeWidth = 4f)
}

private fun DrawScope.drawKeeper(@Suppress("UNUSED_PARAMETER") t: Float) {
    val w = size.width
    val h = size.height
    val keeperX = w / 2f
    val keeperY = h * 0.6f
    drawCircle(
        color = Color(0xFFFFC400),
        radius = 14.dp.toPx(),
        center = Offset(keeperX, keeperY),
    )
}

private fun DrawScope.drawBall(@Suppress("UNUSED_PARAMETER") t: Float) {
    val w = size.width
    val h = size.height
    val start = Offset(w / 2f, h * 0.85f)
    drawCircle(
        color = Color.White,
        radius = 6.dp.toPx(),
        center = start,
    )
    // Przykładowy tor (gdy są próbki, można by je odtwarzać).
    if (FrameBuffer.ballSamples > 0) {
        val path = Path()
        val headerOffset = FrameBuffer.HEADER_FLOATS
        val stride = pl.bklasahero.engine.FrameBuffer.BALL_FLOATS_PER_SAMPLE
        val samples = FrameBuffer.ballSamples
        val zMax = 11f
        val aspectX = w / 7.32f
        val aspectY = h * 0.4f / 2.44f
        val baseY = h * 0.55f
        val frontY = h * 0.85f
        for (i in 0 until samples) {
            val o = headerOffset + i * stride
            val z = FrameBuffer.floats[o + 3]
            val x = FrameBuffer.floats[o + 1]
            val y = FrameBuffer.floats[o + 2]
            val depth = (z / zMax).coerceIn(0f, 1f)
            val px = w / 2f + x * aspectX
            val py = frontY + (baseY - frontY) * depth - y * aspectY
            if (i == 0) path.moveTo(px, py) else path.lineTo(px, py)
        }
        drawPath(path = path, color = Color.White, style = Stroke(width = 1.5f))
    }
}