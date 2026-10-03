// Ekran meczu — zapowiedź (laurka, mapka okolicy z odległością) oraz start
// konkursu rzutów karnych. Boisko rysuje MatchRenderer.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.SportsSoccer
import androidx.compose.material3.Button
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.PathEffect
import androidx.compose.ui.graphics.drawscope.rotate
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.dp
import kotlin.math.PI
import kotlin.math.cos
import kotlin.math.roundToInt
import kotlin.math.sin
import pl.bklasahero.AppViewModel
import pl.bklasahero.R
import pl.bklasahero.engine.FrameBuffer
import pl.bklasahero.ui.AppUiState
import pl.bklasahero.ui.MapPoint
import pl.bklasahero.ui.Scoreboard
import pl.bklasahero.ui.ScoreboardBar
import pl.bklasahero.ui.render.MatchRenderer
import pl.bklasahero.ui.theme.BkhColors
import pl.bklasahero.ui.theme.LocalBkhColors

@Composable
fun MatchScreen(state: AppUiState, viewModel: AppViewModel) {
    val colors = LocalBkhColors.current
    // Po wejściu na ekran — czysty podgląd boiska (bez starej animacji).
    LaunchedEffect(state.screen) { FrameBuffer.clear() }

    Box(modifier = Modifier.fillMaxSize()) {
        MatchRenderer(state)
        ScoreboardBar(state.scoreboard)
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp)
                .align(Alignment.BottomCenter),
            verticalArrangement = Arrangement.spacedBy(10.dp),
        ) {
            MatchPreview(state.scoreboard, colors)
            Text(
                stringResource(R.string.match_tap_to_shoot),
                style = MaterialTheme.typography.bodySmall,
                color = colors.onHeroMuted,
                textAlign = TextAlign.Center,
                modifier = Modifier.fillMaxWidth(),
            )
            Button(
                onClick = { viewModel.beginMatch() },
                shape = RoundedCornerShape(16.dp),
                modifier = Modifier.fillMaxWidth().height(54.dp),
            ) {
                Text(
                    text = stringResource(R.string.match_start_shootout),
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.SemiBold,
                )
            }
        }
    }
}

/**
 * Zapowiedź meczu: nazwy klubów, a pod nimi mapka okolicy — gdzie jest nasza
 * miejscowość, gdzie jedziemy i ile kilometrów to dzieli. Laurka jest pieczęcią
 * w rogu mapki.
 */
@Composable
private fun MatchPreview(scoreboard: Scoreboard, colors: BkhColors) {
    val hasTowns = scoreboard.homeTown.isNotBlank() && scoreboard.awayTown.isNotBlank()
    Surface(
        shape = RoundedCornerShape(20.dp),
        color = colors.card,
        border = BorderStroke(1.dp, colors.cardBorder),
        modifier = Modifier.fillMaxWidth(),
    ) {
        Column(
            modifier = Modifier.fillMaxWidth().padding(14.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(10.dp),
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(
                    text = scoreboard.home.ifBlank { "?" },
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.Bold,
                    color = colors.onHero,
                    textAlign = TextAlign.End,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                    modifier = Modifier.weight(1f),
                )
                Text(
                    text = "–",
                    style = MaterialTheme.typography.titleMedium,
                    color = colors.onHeroMuted,
                    modifier = Modifier.padding(horizontal = 10.dp),
                )
                Text(
                    text = scoreboard.away.ifBlank { "?" },
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.Bold,
                    color = colors.onHero,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                    modifier = Modifier.weight(1f),
                )
            }

            if (scoreboard.mapPoints.isNotEmpty()) {
                MatchMap(
                    scoreboard = scoreboard,
                    colors = colors,
                    modifier = Modifier.fillMaxWidth().height(158.dp),
                )
            } else {
                // Starszy rdzeń bez danych mapki — zostaje sam laur i miejscowości.
                LaurelCrest(colors = colors, size = 84.dp)
                if (hasTowns) {
                    Text(
                        text = if (scoreboard.distanceKm > 0.0) {
                            stringResource(
                                R.string.match_preview_towns,
                                scoreboard.homeTown,
                                formatKm(scoreboard.distanceKm),
                                scoreboard.awayTown,
                            )
                        } else {
                            "${scoreboard.homeTown} · ${scoreboard.awayTown}"
                        },
                        style = MaterialTheme.typography.bodySmall,
                        color = colors.onHeroMuted,
                        textAlign = TextAlign.Center,
                    )
                }
            }

            if (scoreboard.leagueLabel.isNotBlank()) {
                Text(
                    text = stringResource(
                        R.string.match_preview_meta,
                        scoreboard.round,
                        scoreboard.leagueLabel,
                    ),
                    style = MaterialTheme.typography.labelSmall,
                    color = colors.onHeroMuted,
                    textAlign = TextAlign.Center,
                )
            }
            if (scoreboard.isDecisive) {
                Text(
                    text = stringResource(R.string.match_preview_decisive),
                    style = MaterialTheme.typography.labelSmall,
                    fontWeight = FontWeight.Bold,
                    color = colors.accent,
                )
            }
        }
    }
}

/**
 * Schematyczna mapka ligi: siatka, punkty wszystkich klubów, przerywana trasa
 * „my → rywal", etykiety miejscowości, pieczęć z laurkiem i odległość.
 * Rysowana wektorowo — gra jest w 100% offline, więc żadnych kafelków mapy.
 */
@Composable
private fun MatchMap(scoreboard: Scoreboard, colors: BkhColors, modifier: Modifier = Modifier) {
    val density = LocalDensity.current
    val points = scoreboard.mapPoints
    BoxWithConstraints(
        modifier = modifier
            .clip(RoundedCornerShape(16.dp))
            .background(colors.mapBackground),
    ) {
        val widthPx = with(density) { maxWidth.toPx() }
        val heightPx = with(density) { maxHeight.toPx() }
        val projected = remember(points, widthPx, heightPx) {
            projectPoints(points, widthPx, heightPx)
        }
        val homeIndex = points.indexOfFirst { it.isPlayer }
        val awayIndex = points.indexOfFirst { !it.isPlayer && it.town == scoreboard.awayTown }
            .let { if (it >= 0) it else points.indexOfFirst { !it.isPlayer } }

        Canvas(modifier = Modifier.fillMaxSize()) {
            val step = size.minDimension / 4f
            var gx = step
            while (gx < size.width) {
                drawLine(colors.mapGrid, Offset(gx, 0f), Offset(gx, size.height), strokeWidth = 1f)
                gx += step
            }
            var gy = step
            while (gy < size.height) {
                drawLine(colors.mapGrid, Offset(0f, gy), Offset(size.width, gy), strokeWidth = 1f)
                gy += step
            }
            if (homeIndex >= 0 && awayIndex >= 0) {
                drawLine(
                    color = colors.accent,
                    start = Offset(projected[homeIndex].x, projected[homeIndex].y),
                    end = Offset(projected[awayIndex].x, projected[awayIndex].y),
                    strokeWidth = 3f,
                    pathEffect = PathEffect.dashPathEffect(floatArrayOf(12f, 10f)),
                )
            }
            projected.forEachIndexed { index, p ->
                val center = Offset(p.x, p.y)
                when {
                    index == homeIndex || index == awayIndex -> {
                        val color = if (index == homeIndex) colors.accent else colors.danger
                        drawCircle(color, radius = 8f, center = center)
                        drawCircle(colors.mapBackground, radius = 3.5f, center = center)
                    }
                    else -> drawCircle(
                        color = colors.onHeroMuted.copy(alpha = 0.5f),
                        radius = 3.5f,
                        center = center,
                    )
                }
            }
        }

        if (homeIndex >= 0) {
            MapLabel(
                text = scoreboard.homeTown,
                dot = projected[homeIndex],
                above = true,
                widthPx = widthPx,
                heightPx = heightPx,
                colors = colors,
            )
        }
        if (awayIndex >= 0) {
            MapLabel(
                text = scoreboard.awayTown,
                dot = projected[awayIndex],
                above = false,
                widthPx = widthPx,
                heightPx = heightPx,
                colors = colors,
            )
        }

        // Laurka — pieczęć w lewym górnym rogu mapki.
        LaurelCrest(
            colors = colors,
            size = 52.dp,
            plate = colors.mapBackground,
            modifier = Modifier.align(Alignment.TopStart).padding(6.dp),
        )

        if (scoreboard.distanceKm > 0.0) {
            Surface(
                shape = RoundedCornerShape(999.dp),
                color = colors.card,
                border = BorderStroke(1.dp, colors.cardBorder),
                modifier = Modifier.align(Alignment.TopEnd).padding(8.dp),
            ) {
                Text(
                    text = stringResource(
                        R.string.match_map_distance,
                        formatKm(scoreboard.distanceKm),
                    ),
                    style = MaterialTheme.typography.labelMedium,
                    fontWeight = FontWeight.Bold,
                    color = colors.accent,
                    modifier = Modifier.padding(horizontal = 10.dp, vertical = 4.dp),
                )
            }
        }
    }
}

/** Etykieta miejscowości przy punkcie na mapce, dociągnięta do brzegów mapki. */
@Composable
private fun MapLabel(
    text: String,
    dot: ProjectedPoint,
    above: Boolean,
    widthPx: Float,
    heightPx: Float,
    colors: BkhColors,
) {
    if (text.isBlank()) return
    val density = LocalDensity.current
    val maxWidth = 132.dp
    val labelWidthPx = with(density) { maxWidth.toPx() }
    val labelHeightPx = with(density) { 22.dp.toPx() }
    val x = (dot.x + 12f).coerceIn(4f, (widthPx - labelWidthPx - 4f).coerceAtLeast(4f))
    val y = (if (above) dot.y - labelHeightPx - 6f else dot.y + 10f)
        .coerceIn(4f, (heightPx - labelHeightPx - 4f).coerceAtLeast(4f))
    Surface(
        shape = RoundedCornerShape(6.dp),
        color = colors.card,
        modifier = Modifier.offset { IntOffset(x.roundToInt(), y.roundToInt()) },
    ) {
        Text(
            text = text,
            style = MaterialTheme.typography.labelSmall,
            color = colors.onHero,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
            modifier = Modifier.widthIn(max = maxWidth).padding(horizontal = 6.dp, vertical = 2.dp),
        )
    }
}

/** Wieniec laurowy wokół piłki — pieczęć zapowiedzi meczu. */
@Composable
private fun LaurelCrest(
    colors: BkhColors,
    size: Dp,
    plate: Color = colors.heroBottom,
    modifier: Modifier = Modifier,
) {
    Box(modifier = modifier.size(size), contentAlignment = Alignment.Center) {
        Canvas(modifier = Modifier.fillMaxSize()) {
            val cx = this.size.width / 2f
            val cy = this.size.height / 2f
            val radius = this.size.minDimension * 0.40f
            val leafLength = this.size.minDimension * 0.17f
            val leafWidth = this.size.minDimension * 0.075f
            val leaves = 7
            // Dwie gałęzie (lewa i prawa) otwarte u góry — jak w wieńcu laurowym;
            // listki są styczne do okręgu.
            for (side in intArrayOf(-1, 1)) {
                for (i in 0 until leaves) {
                    val deg = 30f + i * (130f / (leaves - 1))
                    val rad = deg * PI / 180.0
                    val x = cx + side * radius * sin(rad).toFloat()
                    val y = cy - radius * cos(rad).toFloat()
                    rotate(degrees = side * deg, pivot = Offset(x, y)) {
                        drawOval(
                            color = colors.accent,
                            topLeft = Offset(x - leafLength / 2f, y - leafWidth / 2f),
                            size = Size(leafLength, leafWidth),
                        )
                    }
                }
            }
        }
        Box(
            modifier = Modifier
                .size(size * 0.5f)
                .clip(CircleShape)
                .background(plate),
            contentAlignment = Alignment.Center,
        ) {
            Icon(
                imageVector = Icons.Outlined.SportsSoccer,
                contentDescription = null,
                tint = colors.accent,
                modifier = Modifier.size(size * 0.32f),
            )
        }
    }
}

// --- Rzutowanie geograficzne ----------------------------------------------

private data class ProjectedPoint(val x: Float, val y: Float)

/**
 * Rzutuje współrzędne klubów na prostokąt mapki. Skala jest jednakowa w obu
 * osiach (z poprawką `cos(szerokość)`), więc okolica nie jest zniekształcona;
 * północ jest u góry.
 */
private fun projectPoints(
    points: List<MapPoint>,
    widthPx: Float,
    heightPx: Float,
): List<ProjectedPoint> {
    if (points.isEmpty()) return emptyList()
    val latMid = points.map { it.lat }.average()
    val kmPerLat = 110.574
    val kmPerLon = 111.320 * cos(latMid * PI / 180.0)
    val xs = points.map { it.lon * kmPerLon }
    val ys = points.map { it.lat * kmPerLat }
    val minX = xs.min()
    val maxX = xs.max()
    val minY = ys.min()
    val maxY = ys.max()
    val spanX = (maxX - minX).coerceAtLeast(0.5)
    val spanY = (maxY - minY).coerceAtLeast(0.5)
    // Margines zostawia miejsce na etykiety, pieczęć i plakietkę z odległością.
    val scale = minOf(
        widthPx * 0.70f / spanX.toFloat(),
        heightPx * 0.60f / spanY.toFloat(),
    )
    val offX = (widthPx - spanX.toFloat() * scale) / 2f
    val offY = (heightPx - spanY.toFloat() * scale) / 2f
    return points.mapIndexed { index, _ ->
        ProjectedPoint(
            x = offX + (xs[index] - minX).toFloat() * scale,
            y = offY + (maxY - ys[index]).toFloat() * scale,
        )
    }
}

/** „8,4" dla bliskich wyjazdów, „37" dla dalszych — czytelniej niż same ułamki. */
private fun formatKm(km: Double): String =
    if (km < 10.0) String.format("%.1f", km) else km.roundToInt().toString()
