// Pseudo-3D renderer rzutu karnego — ulepszona grafika.
// Scena: stadion (niebo + trybuny), murawa w pasy, bramka z siatką,
// strzelec z tyłu, bramkarz nurkujący oraz piłka lecąca po prawdziwym torze
// z FrameBuffer (dane C++). Animacja jest odtwarzana po każdym rzucie.
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
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.rotate
import pl.bklasahero.engine.FrameBuffer

// --- paleta ---
private val SkyTop = Color(0xFF12335C)
private val SkyBottom = Color(0xFF7FA8D9)
private val StandsColor = Color(0xFF24303E)
private val CrowdA = Color(0xFF2E3C4E)
private val CrowdB = Color(0xFF1B2530)
private val GrassBase = Color(0xFF2F8B3B)
private val GrassLight = Color(0xFF3AA24A)
private val GrassDark = Color(0xFF287C34)
private val LineWhite = Color(0xFFF2F6F2)
private val NetColor = Color(0xFFE8ECE8)
private val PostWhite = Color(0xFFFAFAFA)
private val PostShadow = Color(0xFFB9C2BC)
private val KeeperJersey = Color(0xFFF4B400)
private val KeeperSkin = Color(0xFFE8B48C)
private val KeeperGlove = Color(0xFF3A3F45)
private val ShooterJersey = Color(0xFFD9342B)
private val ShooterShorts = Color(0xFF20242A)
private val ShooterSkin = Color(0xFFE8B48C)
private val ShooterSock = Color(0xFFD9342B)
private val Boot = Color(0xFF16181C)
private val BallWhite = Color(0xFFFBFBFB)
private val BallShade = Color(0xFFB9BEC4)

// --- stałe perspektywy (w ułamkach ekranu) ---
private const val HORIZON = 0.52f        // linia horyzontu / dolna krawędź bramki
private const val GOAL_TOP = 0.30f       // górna krawędź bramki (poprzeczka)
private const val GOAL_LEFT = 0.19f      // lewy słupek (ułamek szerokości)
private const val GOAL_RIGHT = 0.81f     // prawy słupek
private const val FRONT_Y = 0.80f        // punkt karny (z=0) na ekranie
private const val SHOOTER_Y = 0.90f      // strzelec (bliżej kamery)
private const val Z_MAX = 11f            // głębokość: 0 = punkt karny, 11 = linia bramkowa

@Composable
fun MatchRenderer(@Suppress("UNUSED_PARAMETER") state: pl.bklasahero.ui.AppUiState) {
    var t by remember { mutableStateOf(0f) }

    // Klucz = tablica klatek: nowa tablica po każdym rzucie → animacja startuje od nowa.
    val floats = FrameBuffer.floats
    LaunchedEffect(floats) {
        if (floats.size > FrameBuffer.HEADER_FLOATS) {
            val duration = FrameBuffer.durationS.coerceAtLeast(0.65f)
            val start = withFrameNanos { it }
            while (true) {
                val now = withFrameNanos { it }
                val elapsed = (now - start) / 1_000_000_000.0f
                t = (elapsed / duration).coerceIn(0f, 1f)
                if (t >= 1f) break
            }
        } else {
            t = 0f
        }
    }

    Canvas(modifier = Modifier.fillMaxSize()) {
        drawSkyAndStands()
        drawGrass()
        drawFieldLines()
        drawGoal()
        drawTrajectory(t)
        drawKeeper(t)
        drawShooter(t)
        drawBall(t)
    }
}

// ---------------------------------------------------------------------------
// Niebo i trybuny
// ---------------------------------------------------------------------------
private fun DrawScope.drawSkyAndStands() {
    val h = size.height
    val horizon = h * HORIZON
    // niebo — gradient wieczorny
    drawRect(
        brush = Brush.verticalGradient(
            colors = listOf(SkyTop, SkyBottom),
            startY = 0f,
            endY = horizon,
        ),
        topLeft = Offset(0f, 0f),
        size = Size(size.width, horizon),
    )
    // trybuny — pas tuż nad horyzontem
    val standsH = h * 0.07f
    drawRect(
        color = StandsColor,
        topLeft = Offset(0f, horizon - standsH),
        size = Size(size.width, standsH),
    )
    // "tłum" — rozproszone plamki w trybunach
    val step = 14f
    var row = 0
    var y = horizon - standsH * 0.75f
    while (y < horizon - standsH * 0.15f) {
        var x = (if (row % 2 == 0) 8f else 20f)
        while (x < size.width) {
            val c = if ((row + x.toInt() / step.toInt()) % 2 == 0) CrowdA else CrowdB
            drawCircle(color = c, radius = 2.6f, center = Offset(x, y))
            x += step
        }
        y += 9f
        row++
    }
}

// ---------------------------------------------------------------------------
// Murawa w pasy
// ---------------------------------------------------------------------------
private fun DrawScope.drawGrass() {
    val w = size.width
    val h = size.height
    val horizon = h * HORIZON
    // pionowy gradient (jaśniej bliżej kamery)
    drawRect(
        brush = Brush.verticalGradient(
            colors = listOf(GrassLight, GrassBase),
            startY = horizon,
            endY = h,
        ),
        topLeft = Offset(0f, horizon),
        size = Size(w, h - horizon),
    )
    // pasy koszenia — poziome, zagęszczają się ku horyzontowi
    val bands = 9
    for (i in 0 until bands) {
        val f0 = i.toFloat() / bands
        val f1 = (i + 0.5f) / bands
        // perspektywiczne rozłożenie pasów
        val y0 = horizon + (h - horizon) * (f0 * f0)
        val y1 = horizon + (h - horizon) * (f1 * f1)
        if (i % 2 == 0) {
            drawRect(
                color = GrassDark.copy(alpha = 0.35f),
                topLeft = Offset(0f, y0),
                size = Size(w, (y1 - y0).coerceAtLeast(1f)),
            )
        }
    }
}

// ---------------------------------------------------------------------------
// Linie boiska (pole karne, punkt karny)
// ---------------------------------------------------------------------------
private fun DrawScope.drawFieldLines() {
    val w = size.width
    val h = size.height
    val horizon = h * HORIZON
    val bottom = h * 0.985f
    val cx = w / 2f
    // linie boczne zbiegające do punktu zbiegu
    drawLine(LineWhite, Offset(0f, horizon), Offset(cx, bottom), strokeWidth = 2.5f)
    drawLine(LineWhite, Offset(w, horizon), Offset(cx, bottom), strokeWidth = 2.5f)
    // pole karne — trapez
    val boxTop = h * 0.62f
    val boxHalfTop = w * 0.28f
    val boxHalfBottom = w * 0.44f
    val path = Path().apply {
        moveTo(cx - boxHalfTop, boxTop)
        lineTo(cx - boxHalfBottom, bottom)
        lineTo(cx + boxHalfBottom, bottom)
        lineTo(cx + boxHalfTop, boxTop)
        close()
    }
    drawPath(path, color = LineWhite, style = Stroke(width = 2.5f))
    // punkt karny
    drawCircle(color = LineWhite, radius = 4.5f, center = Offset(cx, h * FRONT_Y))
    // łuk pola karnego (uproszczony) — wygięty od bramki
    drawArc(
        color = LineWhite,
        startAngle = 0f,
        sweepAngle = 180f,
        useCenter = false,
        topLeft = Offset(cx - w * 0.09f, h * FRONT_Y - w * 0.09f),
        size = Size(w * 0.18f, w * 0.18f),
        style = Stroke(width = 2.5f),
    )
}

// ---------------------------------------------------------------------------
// Bramka z siatką
// ---------------------------------------------------------------------------
private fun DrawScope.drawGoal() {
    val w = size.width
    val h = size.height
    val top = h * GOAL_TOP
    val bottom = h * HORIZON
    val left = w * GOAL_LEFT
    val right = w * GOAL_RIGHT
    val gw = right - left
    val gh = bottom - top

    // siatka
    val cols = 16
    val rows = 8
    for (c in 0..cols) {
        val x = left + gw * c / cols
        drawLine(NetColor, Offset(x, top), Offset(x, bottom), strokeWidth = 1.2f)
    }
    for (r in 0..rows) {
        val y = top + gh * r / rows
        drawLine(NetColor, Offset(left, y), Offset(right, y), strokeWidth = 1.2f)
    }
    // tylna część siatki (głębia) — lekko przesunięta w dół
    val depth = gh * 0.16f
    for (c in 0..cols) {
        val x = left + gw * c / cols
        drawLine(NetColor.copy(alpha = 0.5f), Offset(x, top), Offset(x, bottom + depth), strokeWidth = 1.2f)
    }
    drawLine(NetColor.copy(alpha = 0.5f), Offset(left, bottom + depth), Offset(right, bottom + depth), strokeWidth = 1.2f)

    // słupki i poprzeczka (z cieniem)
    val postW = 6f
    drawLine(PostShadow, Offset(left - 2f, top), Offset(left - 2f, bottom), strokeWidth = postW + 2f)
    drawLine(PostShadow, Offset(right + 2f, top), Offset(right + 2f, bottom), strokeWidth = postW + 2f)
    drawLine(PostShadow, Offset(left, top + 2f), Offset(right, top + 2f), strokeWidth = postW + 2f)
    drawLine(PostWhite, Offset(left, top), Offset(left, bottom), strokeWidth = postW)
    drawLine(PostWhite, Offset(right, top), Offset(right, bottom), strokeWidth = postW)
    drawLine(PostWhite, Offset(left, top), Offset(right, top), strokeWidth = postW)
}

// ---------------------------------------------------------------------------
// Mapowanie świata (metry) → ekran (piksele)
// ---------------------------------------------------------------------------
private fun DrawScope.worldToScreen(xM: Float, yM: Float, zM: Float): Offset {
    val w = size.width
    val h = size.height
    val goalHalfW = w * (GOAL_RIGHT - GOAL_LEFT) / 2f
    val goalH = h * (HORIZON - GOAL_TOP)
    val scaleX = goalHalfW / 3.66f
    val scaleY = goalH / 2.44f
    val depth = (zM / Z_MAX).coerceIn(0f, 1f)
    val px = w / 2f + xM * scaleX
    val py = h * FRONT_Y + (h * HORIZON - h * FRONT_Y) * depth - yM * scaleY
    return Offset(px, py)
}

private fun interpolateSample(
    floats: FloatArray, start: Int, count: Int, stride: Int, timeS: Float, comp: Int,
): Float {
    if (count <= 0) return 0f
    var lo = 0
    var hi = count - 1
    while (lo < hi) {
        val mid = (lo + hi + 1) / 2
        if (floats[start + mid * stride] <= timeS) lo = mid else hi = mid - 1
    }
    val t0 = floats[start + lo * stride]
    if (lo >= count - 1) return floats[start + lo * stride + comp]
    val t1 = floats[start + (lo + 1) * stride]
    val f = if (t1 > t0) ((timeS - t0) / (t1 - t0)).coerceIn(0f, 1f) else 0f
    val v0 = floats[start + lo * stride + comp]
    val v1 = floats[start + (lo + 1) * stride + comp]
    return v0 + (v1 - v0) * f
}

private fun ballPoint(floats: FloatArray, timeS: Float): Triple<Float, Float, Float> {
    val count = FrameBuffer.ballSamples
    if (count <= 0) return Triple(0f, 0f, 0f)
    val start = FrameBuffer.HEADER_FLOATS
    val stride = FrameBuffer.BALL_FLOATS_PER_SAMPLE
    val x = interpolateSample(floats, start, count, stride, timeS, 1)
    val y = interpolateSample(floats, start, count, stride, timeS, 2)
    val z = interpolateSample(floats, start, count, stride, timeS, 3)
    return Triple(x, y, z)
}

private fun keeperPoint(floats: FloatArray, timeS: Float): Triple<Float, Float, Float> {
    val count = FrameBuffer.keeperSamples
    if (count <= 0) return Triple(0f, 0f, 0f)
    val start = FrameBuffer.HEADER_FLOATS + FrameBuffer.ballSamples * FrameBuffer.BALL_FLOATS_PER_SAMPLE
    val stride = FrameBuffer.KEEPER_FLOATS_PER_SAMPLE
    val x = interpolateSample(floats, start, count, stride, timeS, 1)
    val y = interpolateSample(floats, start, count, stride, timeS, 2)
    val z = interpolateSample(floats, start, count, stride, timeS, 3)
    return Triple(x, y, z)
}

// ---------------------------------------------------------------------------
// Tor piłki (subtelny ślad)
// ---------------------------------------------------------------------------
private fun DrawScope.drawTrajectory(t: Float) {
    val floats = FrameBuffer.floats
    val count = FrameBuffer.ballSamples
    if (count < 2) return
    val start = FrameBuffer.HEADER_FLOATS
    val stride = FrameBuffer.BALL_FLOATS_PER_SAMPLE
    val path = Path()
    var started = false
    val shown = (count * t).toInt().coerceAtLeast(2)
    for (i in 0 until shown.coerceAtMost(count)) {
        val o = start + i * stride
        val p = worldToScreen(floats[o + 1], floats[o + 2], floats[o + 3])
        if (!started) { path.moveTo(p.x, p.y); started = true } else path.lineTo(p.x, p.y)
    }
    if (started) {
        drawPath(path, color = Color.White.copy(alpha = 0.55f), style = Stroke(width = 2f, cap = StrokeCap.Round))
    }
}

// ---------------------------------------------------------------------------
// Bramkarz (nurkujący)
// ---------------------------------------------------------------------------
private fun DrawScope.drawKeeper(t: Float) {
    val w = size.width
    val h = size.height
    val floats = FrameBuffer.floats
    var center = Offset(w / 2f, h * HORIZON)
    var dive = 0f
    if (FrameBuffer.keeperSamples > 0) {
        val timeS = t * FrameBuffer.durationS
        val (kx, ky, kz) = keeperPoint(floats, timeS)
        center = worldToScreen(kx, ky, kz)
        // diveProgress z ostatniej próbki bramkarza w tym czasie
        val start = FrameBuffer.HEADER_FLOATS + FrameBuffer.ballSamples * FrameBuffer.BALL_FLOATS_PER_SAMPLE
        dive = interpolateSample(floats, start, FrameBuffer.keeperSamples, FrameBuffer.KEEPER_FLOATS_PER_SAMPLE, timeS, 4)
    }
    val scale = (h / 1920f).coerceIn(0.6f, 1.6f)
    val r = 15f * scale

    // tułów + ręce + nogi — prosta sylwetka z rotacją przy nurkowaniu
    rotate(degrees = dive * 40f, pivot = center) {
        // ręce (z rękawicami) — rozłożone przy nurkowaniu
        val armSpread = 26f * scale * (1f + dive)
        drawLine(KeeperJersey, center, Offset(center.x - armSpread, center.y - 8f * scale), strokeWidth = 7f * scale, cap = StrokeCap.Round)
        drawLine(KeeperJersey, center, Offset(center.x + armSpread, center.y - 8f * scale), strokeWidth = 7f * scale, cap = StrokeCap.Round)
        drawCircle(KeeperGlove, radius = 6f * scale, center = Offset(center.x - armSpread, center.y - 8f * scale))
        drawCircle(KeeperGlove, radius = 6f * scale, center = Offset(center.x + armSpread, center.y - 8f * scale))
        // nogi
        drawLine(Boot, center, Offset(center.x - 10f * scale, center.y + 22f * scale), strokeWidth = 8f * scale, cap = StrokeCap.Round)
        drawLine(Boot, center, Offset(center.x + 10f * scale, center.y + 22f * scale), strokeWidth = 8f * scale, cap = StrokeCap.Round)
        // tułów
        drawCircle(KeeperJersey, radius = r, center = center)
        // głowa
        drawCircle(KeeperSkin, radius = r * 0.55f, center = Offset(center.x, center.y - r * 1.15f))
    }
}

// ---------------------------------------------------------------------------
// Strzelec (widok od tyłu)
// ---------------------------------------------------------------------------
private fun DrawScope.drawShooter(@Suppress("UNUSED_PARAMETER") t: Float) {
    val w = size.width
    val h = size.height
    val scale = (h / 1920f).coerceIn(0.6f, 1.6f)
    val cx = w / 2f
    val baseY = h * SHOOTER_Y

    // nogi (łydki + buty)
    drawLine(ShooterSock, Offset(cx - 16f * scale, baseY - 30f * scale), Offset(cx - 18f * scale, baseY), strokeWidth = 9f * scale, cap = StrokeCap.Round)
    drawLine(ShooterSock, Offset(cx + 16f * scale, baseY - 30f * scale), Offset(cx + 18f * scale, baseY), strokeWidth = 9f * scale, cap = StrokeCap.Round)
    drawCircle(Boot, radius = 8f * scale, center = Offset(cx - 20f * scale, baseY))
    drawCircle(Boot, radius = 8f * scale, center = Offset(cx + 20f * scale, baseY))
    // spodenki
    drawRoundRectShooter(ShooterShorts, cx - 20f * scale, baseY - 52f * scale, 40f * scale, 26f * scale)
    // tułów (koszulka z numerem)
    drawRoundRectShooter(ShooterJersey, cx - 22f * scale, baseY - 96f * scale, 44f * scale, 48f * scale)
    drawCircle(color = Color.White, radius = 10f * scale, center = Offset(cx, baseY - 72f * scale))
    // ręce (wzdłuż tułowia)
    drawLine(ShooterJersey, Offset(cx - 24f * scale, baseY - 84f * scale), Offset(cx - 30f * scale, baseY - 56f * scale), strokeWidth = 7f * scale, cap = StrokeCap.Round)
    drawLine(ShooterJersey, Offset(cx + 24f * scale, baseY - 84f * scale), Offset(cx + 30f * scale, baseY - 56f * scale), strokeWidth = 7f * scale, cap = StrokeCap.Round)
    // głowa
    drawCircle(ShooterSkin, radius = 14f * scale, center = Offset(cx, baseY - 108f * scale))
}

// rysuje zaokrąglony prostokąt (pomocnicza)
private fun DrawScope.drawRoundRectShooter(color: Color, left: Float, top: Float, w: Float, h: Float) {
    drawRoundRect(
        color = color,
        topLeft = Offset(left, top),
        size = Size(w, h),
        cornerRadius = androidx.compose.ui.geometry.CornerRadius(8f, 8f),
    )
}

// ---------------------------------------------------------------------------
// Piłka (animowana po torze)
// ---------------------------------------------------------------------------
private fun DrawScope.drawBall(t: Float) {
    val w = size.width
    val h = size.height
    val floats = FrameBuffer.floats
    var center = Offset(w / 2f, h * FRONT_Y)
    if (FrameBuffer.ballSamples > 0) {
        val timeS = t * FrameBuffer.durationS
        val (bx, by, bz) = ballPoint(floats, timeS)
        center = worldToScreen(bx, by, bz)
    }
    val scale = (h / 1920f).coerceIn(0.6f, 1.6f)
    val r = 9f * scale
    // cień piłki na murawie
    drawOval(
        color = Color.Black.copy(alpha = 0.18f),
        topLeft = Offset(center.x - r * 0.8f, h * HORIZON + 4f * scale),
        size = Size(r * 1.6f, r * 0.5f),
    )
    // piłka — gradient sferyczny + paski
    drawCircle(
        brush = Brush.radialGradient(
            colors = listOf(Color.White, BallWhite, BallShade),
            center = Offset(center.x - r * 0.3f, center.y - r * 0.3f),
            radius = r * 1.8f,
        ),
        radius = r,
        center = center,
    )
    drawCircle(color = BallShade.copy(alpha = 0.5f), radius = r * 0.18f, center = Offset(center.x - r * 0.2f, center.y - r * 0.2f))
    drawCircle(color = Color.White.copy(alpha = 0.9f), radius = r * 0.10f, center = Offset(center.x - r * 0.35f, center.y - r * 0.4f))
}
