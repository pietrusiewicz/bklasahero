// Samouczek na wejściu — pokazywany raz, po pierwszym uruchomieniu.
// Trzy kroki: jak się gra → jak się strzela → jak się broni.
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
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import pl.bklasahero.R
import pl.bklasahero.ui.AppIntent
import pl.bklasahero.ui.AppUiState

private const val PAGES = 3

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

/** Prosty obrazek-instrukcja: boisko, bramka i to, o czym mówi dany krok. */
@Composable
private fun TutorialArt(page: Int) {
    Canvas(
        modifier = Modifier
            .fillMaxWidth()
            .height(180.dp),
    ) {
        val w = size.width
        val h = size.height
        // murawa
        drawRect(color = Color(0xFF2F8B3B), size = Size(w, h))
        drawRect(color = Color(0x22000000), topLeft = Offset(0f, h * 0.55f), size = Size(w, h * 0.45f))

        // bramka
        val gl = w * 0.12f
        val gr = w * 0.88f
        val gt = h * 0.16f
        val gb = h * 0.70f
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

        when (page) {
            0 -> {
                // punkt karny i piłka
                drawLine(Color(0x88FFFFFF), Offset(w * 0.5f, gb), Offset(w * 0.5f, h * 0.95f), strokeWidth = 2f)
                drawCircle(Color.White, radius = h * 0.11f, center = Offset(w * 0.5f, h * 0.86f))
            }
            1 -> {
                // celownik + pasek mocy
                val c = Offset(w * 0.30f, h * 0.34f)
                val r = h * 0.14f
                drawCircle(Color.White, radius = r, center = c, style = Stroke(width = 3f))
                drawCircle(Color.White, radius = 3f, center = c)
                drawLine(Color.White, Offset(c.x - r - 12f, c.y), Offset(c.x - r + 2f, c.y), strokeWidth = 3f)
                drawLine(Color.White, Offset(c.x + r - 2f, c.y), Offset(c.x + r + 12f, c.y), strokeWidth = 3f)
                drawLine(Color.White, Offset(c.x, c.y - r - 12f), Offset(c.x, c.y - r + 2f), strokeWidth = 3f)
                drawLine(Color.White, Offset(c.x, c.y + r - 2f), Offset(c.x, c.y + r + 12f), strokeWidth = 3f)
                // pasek mocy
                drawRoundRect(
                    color = Color(0x66000000),
                    topLeft = Offset(w * 0.25f, h * 0.86f),
                    size = Size(w * 0.5f, h * 0.08f),
                    cornerRadius = CornerRadius(8f, 8f),
                )
                drawRoundRect(
                    color = Color(0xFFFFC93C),
                    topLeft = Offset(w * 0.25f, h * 0.86f),
                    size = Size(w * 0.5f * 0.65f, h * 0.08f),
                    cornerRadius = CornerRadius(8f, 8f),
                )
            }
            else -> {
                // bramkarz nurkujący w lewo + piłka w górnym rogu
                val kx = w * 0.34f
                val ky = h * 0.52f
                drawLine(
                    Color(0xFFF4B400),
                    Offset(kx - w * 0.05f, ky + h * 0.06f),
                    Offset(kx - w * 0.20f, ky - h * 0.16f),
                    strokeWidth = 16f,
                    cap = StrokeCap.Round,
                )
                drawLine(
                    Color(0xFFF4B400),
                    Offset(kx - w * 0.05f, ky + h * 0.06f),
                    Offset(kx + w * 0.06f, ky + h * 0.20f),
                    strokeWidth = 16f,
                    cap = StrokeCap.Round,
                )
                drawCircle(Color(0xFF3A3F45), radius = 9f, center = Offset(kx - w * 0.21f, ky - h * 0.17f))
                drawCircle(Color(0xFFE8B48C), radius = 12f, center = Offset(kx + w * 0.07f, ky + h * 0.22f))
                drawCircle(Color.White, radius = h * 0.09f, center = Offset(w * 0.16f, h * 0.30f))
            }
        }
    }
}
