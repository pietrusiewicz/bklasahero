// Samouczek na wejściu — pokazywany raz, po pierwszym uruchomieniu.
// Trzy kroki: jak się gra → jak się strzela → jak się broni.
// Obrazki pokazują sylwetki zawodników (strzelec od tyłu, nurkujący bramkarz).
// Po ostatnim kroku gracz przechodzi do wyboru swojego miasta.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.PathEffect
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import pl.bklasahero.R
import pl.bklasahero.ui.AppIntent
import pl.bklasahero.ui.AppUiState

private const val PAGES = 3

// Paleta zgodna z rendererem meczu.
private val ArtJersey = Color(0xFFD9342B)
private val ArtShorts = Color(0xFF20242A)
private val ArtSock = Color(0xFFD9342B)
private val ArtBoot = Color(0xFF16181C)
private val ArtSkin = Color(0xFFE8B48C)
private val ArtKeeper = Color(0xFFF4B400)
private val ArtGlove = Color(0xFF3A3F45)
private val ArtGrass = Color(0xFF2F8B3B)

@Composable
fun TutorialScreen(state: AppUiState, dispatch: (AppIntent) -> Unit) {
    var page by remember { mutableStateOf(0) }
    val title = when (page) {
        0 -> R.string.tutorial_p1_title
        1 -> R.string.tutorial_p2_title
        else -> R.string.tutorial_p3_title
    }
    val body = when (page) {
        0 -> R.string.tutorial_p1_body
        1 -> R.string.tutorial_p2_body
        else -> R.string.tutorial_p3_body
    }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Brush.verticalGradient(listOf(Color(0xFF12335C), Color(0xFF0E2A1E), Color(0xFF1C4B2A))))
            .padding(20.dp),
        contentAlignment = Alignment.Center,
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(16.dp),
            modifier = Modifier.fillMaxWidth(),
        ) {
            Surface(
                shape = RoundedCornerShape(20.dp),
                color = Color(0xE6202B3A),
                modifier = Modifier.fillMaxWidth(),
            ) {
                Column(
                    modifier = Modifier.fillMaxWidth().padding(20.dp),
                    horizontalAlignment = Alignment.CenterHorizontally,
                    verticalArrangement = Arrangement.spacedBy(12.dp),
                ) {
                    TutorialArt(page)
                    Text(
                        text = stringResource(title),
                        style = MaterialTheme.typography.headlineSmall,
                        color = Color.White,
                        textAlign = TextAlign.Center,
                    )
                    Text(
                        text = stringResource(body),
                        style = MaterialTheme.typography.bodyLarge,
                        color = Color(0xFFCFE0F0),
                        textAlign = TextAlign.Center,
                    )
                }
            }

            // Wskaźnik kroków
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                repeat(PAGES) { i ->
                    Box(
                        modifier = Modifier
                            .size(if (i == page) 12.dp else 9.dp)
                            .background(
                                color = if (i == page) Color.White else Color(0x66FFFFFF),
                                shape = CircleShape,
                            ),
                    )
                }
            }

            Button(
                onClick = {
                    if (page < PAGES - 1) page++ else dispatch(AppIntent.FinishTutorial)
                },
                modifier = Modifier.fillMaxWidth().height(56.dp),
            ) {
                Text(
                    stringResource(
                        if (page < PAGES - 1) R.string.tutorial_next else R.string.tutorial_choose_city
                    )
                )
            }
            TextButton(onClick = { dispatch(AppIntent.FinishTutorial) }) {
                Text(stringResource(R.string.tutorial_skip), color = Color(0xFFCFE0F0))
            }
        }
    }
}

/** Obrazek-instrukcja: boisko, bramka i zawodnicy pokazujący dany krok. */
@Composable
private fun TutorialArt(page: Int) {
    Canvas(
        modifier = Modifier
            .fillMaxWidth()
            .height(200.dp),
    ) {
        val w = size.width
        val h = size.height

        // Murawa + pasy koszenia
        drawRect(color = ArtGrass, size = Size(w, h))
        drawRect(color = Color(0x22000000), topLeft = Offset(0f, h * 0.62f), size = Size(w, h * 0.38f))

        // Bramka
        val gl = w * 0.10f
        val gr = w * 0.90f
        val gt = h * 0.08f
        val gb = h * 0.52f
        drawRect(color = Color(0x1AFFFFFF), topLeft = Offset(gl, gt), size = Size(gr - gl, gb - gt))
        for (i in 1..8) {
            val x = gl + (gr - gl) * i / 9f
            drawLine(Color(0x55FFFFFF), Offset(x, gt), Offset(x, gb), strokeWidth = 1.5f)
        }
        for (i in 1..4) {
            val y = gt + (gb - gt) * i / 5f
            drawLine(Color(0x55FFFFFF), Offset(gl, y), Offset(gr, y), strokeWidth = 1.5f)
        }
        drawLine(Color.White, Offset(gl, gt), Offset(gl, gb), strokeWidth = 5f)
        drawLine(Color.White, Offset(gr, gt), Offset(gr, gb), strokeWidth = 5f)
        drawLine(Color.White, Offset(gl, gt), Offset(gr, gt), strokeWidth = 5f)
        // linia bramkowa + punkt karny
        drawLine(Color(0x99FFFFFF), Offset(0f, gb), Offset(w, gb), strokeWidth = 2f)

        when (page) {
            0 -> {
                // Jak się gra: strzelec od tyłu + piłka na punkcie karnym
                drawCircle(Color.White, radius = h * 0.05f, center = Offset(w * 0.5f, h * 0.66f))
                drawShooterFigure(cx = w * 0.5f, feetY = h * 0.99f, s = h * 0.115f)
            }
            1 -> {
                // Jak się strzela: celownik, tor lotu, pasek mocy
                val ball = Offset(w * 0.5f, h * 0.64f)
                val aim = Offset(w * 0.28f, h * 0.28f)
                drawLine(
                    color = Color(0x99FFFFFF),
                    start = ball,
                    end = aim,
                    strokeWidth = 3f,
                    pathEffect = PathEffect.dashPathEffect(floatArrayOf(14f, 14f)),
                )
                val r = h * 0.12f
                drawCircle(Color.White, radius = r, center = aim, style = Stroke(width = 3f))
                drawCircle(Color.White, radius = 3f, center = aim)
                drawLine(Color.White, Offset(aim.x - r - 14f, aim.y), Offset(aim.x - r + 2f, aim.y), strokeWidth = 3f)
                drawLine(Color.White, Offset(aim.x + r - 2f, aim.y), Offset(aim.x + r + 14f, aim.y), strokeWidth = 3f)
                drawLine(Color.White, Offset(aim.x, aim.y - r - 14f), Offset(aim.x, aim.y - r + 2f), strokeWidth = 3f)
                drawLine(Color.White, Offset(aim.x, aim.y + r - 2f), Offset(aim.x, aim.y + r + 14f), strokeWidth = 3f)

                drawCircle(Color.White, radius = h * 0.045f, center = ball)
                drawShooterFigure(cx = w * 0.5f, feetY = h * 0.90f, s = h * 0.105f)

                // Pasek mocy
                drawRoundRect(
                    color = Color(0x66000000),
                    topLeft = Offset(w * 0.22f, h * 0.93f),
                    size = Size(w * 0.56f, h * 0.06f),
                    cornerRadius = CornerRadius(8f, 8f),
                )
                drawRoundRect(
                    color = Color(0xFFFFC93C),
                    topLeft = Offset(w * 0.22f, h * 0.93f),
                    size = Size(w * 0.56f * 0.65f, h * 0.06f),
                    cornerRadius = CornerRadius(8f, 8f),
                )
            }
            else -> {
                // Jak się broni: strzelec przeciwnika, nurkujący bramkarz, piłka w rogu
                drawCircle(Color.White, radius = h * 0.05f, center = Offset(w * 0.72f, h * 0.68f))
                drawShooterFigure(cx = w * 0.62f, feetY = h * 0.99f, s = h * 0.11f)
                drawKeeperFigure(cx = w * 0.42f, cy = h * 0.36f, s = h * 0.10f, dir = -1f)

                // Gest: strzałka w lewo (kierunek nurkowania)
                val ay = h * 0.86f
                drawLine(Color(0xCCFFFFFF), Offset(w * 0.68f, ay), Offset(w * 0.34f, ay), strokeWidth = 5f, cap = StrokeCap.Round)
                drawLine(Color(0xCCFFFFFF), Offset(w * 0.34f, ay), Offset(w * 0.42f, ay - h * 0.05f), strokeWidth = 5f, cap = StrokeCap.Round)
                drawLine(Color(0xCCFFFFFF), Offset(w * 0.34f, ay), Offset(w * 0.42f, ay + h * 0.05f), strokeWidth = 5f, cap = StrokeCap.Round)
            }
        }
    }
}

/** Sylwetka strzelca od tyłu (jak w meczu): nogi, spodenki, koszulka, ręce, głowa. */
private fun DrawScope.drawShooterFigure(cx: Float, feetY: Float, s: Float) {
    drawLine(ArtSock, Offset(cx - 0.30f * s, feetY - 0.85f * s), Offset(cx - 0.38f * s, feetY), strokeWidth = 0.24f * s, cap = StrokeCap.Round)
    drawLine(ArtSock, Offset(cx + 0.30f * s, feetY - 0.85f * s), Offset(cx + 0.38f * s, feetY), strokeWidth = 0.24f * s, cap = StrokeCap.Round)
    drawCircle(ArtBoot, 0.19f * s, Offset(cx - 0.42f * s, feetY))
    drawCircle(ArtBoot, 0.19f * s, Offset(cx + 0.42f * s, feetY))
    drawRoundRect(
        color = ArtShorts,
        topLeft = Offset(cx - 0.50f * s, feetY - 1.45f * s),
        size = Size(1.00f * s, 0.62f * s),
        cornerRadius = CornerRadius(0.18f * s, 0.18f * s),
    )
    drawRoundRect(
        color = ArtJersey,
        topLeft = Offset(cx - 0.55f * s, feetY - 2.60f * s),
        size = Size(1.10f * s, 1.25f * s),
        cornerRadius = CornerRadius(0.20f * s, 0.20f * s),
    )
    drawLine(ArtJersey, Offset(cx - 0.55f * s, feetY - 2.35f * s), Offset(cx - 0.78f * s, feetY - 1.50f * s), strokeWidth = 0.20f * s, cap = StrokeCap.Round)
    drawLine(ArtJersey, Offset(cx + 0.55f * s, feetY - 2.35f * s), Offset(cx + 0.78f * s, feetY - 1.50f * s), strokeWidth = 0.20f * s, cap = StrokeCap.Round)
    drawCircle(ArtSkin, 0.36f * s, Offset(cx, feetY - 3.00f * s))
}

/** Sylwetka bramkarza w nurkowaniu; `dir` = -1 w lewo, +1 w prawo. */
private fun DrawScope.drawKeeperFigure(cx: Float, cy: Float, s: Float, dir: Float) {
    val body = Offset(cx + dir * 0.9f * s, cy - 0.4f * s)
    // nogi
    drawLine(ArtBoot, body, Offset(cx - dir * 0.5f * s, cy + 0.9f * s), strokeWidth = 0.30f * s, cap = StrokeCap.Round)
    drawLine(ArtBoot, body, Offset(cx - dir * 0.1f * s, cy + 1.0f * s), strokeWidth = 0.30f * s, cap = StrokeCap.Round)
    // ręce do piłki
    val hand1 = Offset(body.x + dir * 1.4f * s, body.y - 0.6f * s)
    val hand2 = Offset(body.x + dir * 1.2f * s, body.y + 0.7f * s)
    drawLine(ArtKeeper, body, hand1, strokeWidth = 0.26f * s, cap = StrokeCap.Round)
    drawLine(ArtKeeper, body, hand2, strokeWidth = 0.26f * s, cap = StrokeCap.Round)
    drawCircle(ArtGlove, 0.22f * s, hand1)
    drawCircle(ArtGlove, 0.22f * s, hand2)
    // tułów + głowa
    drawCircle(ArtKeeper, 0.50f * s, body)
    drawCircle(ArtSkin, 0.28f * s, Offset(body.x - dir * 0.35f * s, body.y - 0.35f * s))
}
