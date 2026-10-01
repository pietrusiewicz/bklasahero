// Ekran wykonywania rzutu karnego.
// Przeciągnij palcem po bramce → celownik. Przytrzymaj dłużej → mocniejszy strzał
// (ale mniej precyzyjny — kompromis siła/precyzja po stronie rdzenia).
// Puszczenie palca = strzał.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.IntSize
import androidx.compose.ui.unit.dp
import pl.bklasahero.AppViewModel
import pl.bklasahero.R
import pl.bklasahero.engine.FrameBuffer
import pl.bklasahero.ui.AppUiState
import pl.bklasahero.ui.ScoreboardBar
import pl.bklasahero.ui.render.GOAL_LEFT
import pl.bklasahero.ui.render.GOAL_LINE_Y
import pl.bklasahero.ui.render.GOAL_RIGHT
import pl.bklasahero.ui.render.GOAL_TOP
import pl.bklasahero.ui.render.MatchRenderer

/** Punkt ekranu → cel w metrach (x bok [-3.66..3.66], y wysokość [0..2.44]). */
internal fun screenToAim(p: Offset, w: Float, h: Float): Pair<Float, Float> {
    val gx = p.x.coerceIn(w * GOAL_LEFT, w * GOAL_RIGHT)
    val gy = p.y.coerceIn(h * GOAL_TOP, h * GOAL_LINE_Y)
    val aimX = (gx / w - 0.5f) * 7.32f / (GOAL_RIGHT - GOAL_LEFT)
    val aimY = (h * GOAL_LINE_Y - gy) * 2.44f / (h * (GOAL_LINE_Y - GOAL_TOP))
    return Pair(aimX, aimY)
}

/** Pozycja celownika (px) ograniczona do prostokąta bramki na ekranie. */
internal fun aimScreenPoint(p: Offset, w: Float, h: Float): Offset =
    Offset(
        p.x.coerceIn(w * GOAL_LEFT, w * GOAL_RIGHT),
        p.y.coerceIn(h * GOAL_TOP, h * GOAL_LINE_Y),
    )

@Composable
fun ShootoutScreen(state: AppUiState, viewModel: AppViewModel) {
    LaunchedEffect(state.screen) { FrameBuffer.clear() }

    var aim by remember { mutableStateOf<Offset?>(null) }
    var power by remember { mutableStateOf(0f) }
    var canvasSize by remember { mutableStateOf(IntSize.Zero) }
    val density = LocalDensity.current

    Box(
        modifier = Modifier
            .fillMaxSize()
            .onSizeChanged { canvasSize = it }
            .pointerInput(Unit) {
                awaitEachGesture {
                    val down = awaitFirstDown()
                    aim = down.position
                    power = 0f
                    val holdStart = android.os.SystemClock.uptimeMillis()
                    var end = down.position
                    while (true) {
                        val event = awaitPointerEvent()
                        val change = event.changes.firstOrNull { it.id == down.id } ?: break
                        if (!change.pressed) break
                        end = change.position
                        aim = end
                        val holdMs = (android.os.SystemClock.uptimeMillis() - holdStart).coerceAtLeast(0L)
                        power = (holdMs / 700f).coerceIn(0f, 1f)
                    }
                    val (ax, ay) = screenToAim(end, size.width.toFloat(), size.height.toFloat())
                    val effort = 0.45f + 0.55f * power
                    aim = null
                    power = 0f
                    viewModel.shoot(ax, ay, effort)
                }
            },
    ) {
        MatchRenderer(state)
        ScoreboardBar(state.scoreboard)

        // Celownik w obrębie bramki
        val a = aim
        if (a != null && canvasSize.width > 0) {
            val c = aimScreenPoint(a, canvasSize.width.toFloat(), canvasSize.height.toFloat())
            val rPx = with(density) { 22.dp.toPx() }
            Canvas(modifier = Modifier.fillMaxSize()) {
                drawCircle(color = Color.White, radius = rPx, center = c, style = Stroke(width = 3f))
                drawCircle(color = Color.White, radius = 4f, center = c)
                val o = rPx + 8f
                drawLine(Color.White, Offset(c.x - o, c.y), Offset(c.x - rPx + 4f, c.y), strokeWidth = 3f)
                drawLine(Color.White, Offset(c.x + rPx - 4f, c.y), Offset(c.x + o, c.y), strokeWidth = 3f)
                drawLine(Color.White, Offset(c.x, c.y - o), Offset(c.x, c.y - rPx + 4f), strokeWidth = 3f)
                drawLine(Color.White, Offset(c.x, c.y + rPx - 4f), Offset(c.x, c.y + o), strokeWidth = 3f)
            }
        }

        // Pasek mocy
        if (power > 0f) {
            Box(
                modifier = Modifier
                    .align(Alignment.BottomCenter)
                    .fillMaxWidth()
                    .padding(horizontal = 32.dp, vertical = 28.dp),
            ) {
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(10.dp)
                        .background(Color(0x66000000), RoundedCornerShape(5.dp)),
                )
                Box(
                    modifier = Modifier
                        .fillMaxWidth(power)
                        .height(10.dp)
                        .background(
                            color = when {
                                power < 0.45f -> Color(0xFF7FD24E)
                                power < 0.8f -> Color(0xFFFFC93C)
                                else -> Color(0xFFE5484D)
                            },
                            shape = RoundedCornerShape(5.dp),
                        ),
                )
            }
        }

        // Podpowiedź
        Text(
            text = stringResource(R.string.shootout_shoot_hint),
            style = MaterialTheme.typography.bodyMedium,
            color = Color.White,
            modifier = Modifier
                .align(Alignment.TopCenter)
                .padding(top = 92.dp, start = 24.dp, end = 24.dp),
        )
    }
}
