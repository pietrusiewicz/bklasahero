// Wspólne widoki ligi: tabela i terminarz. Korzystają z nich zarówno zakładki
// w menu głównym, jak i samodzielne ekrany (TableScreen, FixturesScreen) —
// dane z rdzenia wczytujemy w jednym miejscu.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.components

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.RowScope
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.buildJsonObject
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonPrimitive
import kotlinx.serialization.json.put
import pl.bklasahero.R
import pl.bklasahero.engine.NativeBridge
import pl.bklasahero.ui.theme.BkhColors
import pl.bklasahero.ui.theme.LocalBkhColors

// --- Tabela ligi ------------------------------------------------------------

private data class LeagueTableRow(
    val position: Int,
    val name: String,
    val town: String,
    val goalsFor: Int,
    val goalsAgainst: Int,
    val points: Int,
    val isPlayer: Boolean,
)

@Composable
fun LeagueTableView(
    modifier: Modifier = Modifier,
    contentPadding: PaddingValues = PaddingValues(0.dp),
) {
    val colors = LocalBkhColors.current
    var rows by remember { mutableStateOf<List<LeagueTableRow>?>(null) }
    var label by remember { mutableStateOf("") }

    LaunchedEffect(Unit) {
        NativeBridge.command(buildJsonObject { put("cmd", "table") })
            .onSuccess { data ->
                label = data["leagueLabel"]?.jsonPrimitive?.content ?: ""
                val arr = data["table"]?.jsonArray ?: return@onSuccess
                rows = arr.mapNotNull { el ->
                    val o = el as? JsonObject ?: return@mapNotNull null
                    LeagueTableRow(
                        position = o.int("position"),
                        name = o.text("shortName").ifBlank { o.text("name") },
                        town = o.text("town"),
                        goalsFor = o.int("goalsFor"),
                        goalsAgainst = o.int("goalsAgainst"),
                        points = o.int("points"),
                        isPlayer = o["isPlayer"]?.jsonPrimitive?.content?.toBooleanStrictOrNull() ?: false,
                    )
                }
            }
    }

    LazyColumn(
        modifier = modifier.fillMaxWidth(),
        contentPadding = contentPadding,
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        if (label.isNotEmpty()) {
            item { Caption(label, colors) }
        }
        when {
            rows == null -> item { LoadingBox(colors) }
            rows!!.isEmpty() -> item { EmptyBox(colors) }
            else -> item {
                LeagueCard(colors) {
                    TableHeader(colors)
                    HorizontalDivider(color = colors.cardBorder)
                    rows!!.forEach { row -> TableRowView(row, colors) }
                }
            }
        }
    }
}

@Composable
private fun TableHeader(colors: BkhColors) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 14.dp, vertical = 10.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        HeaderCell("#", 0.14f, colors)
        HeaderCell(stringResource(R.string.table_col_team), 0.46f, colors)
        HeaderCell(stringResource(R.string.table_col_goals), 0.20f, colors, TextAlign.Center)
        HeaderCell(stringResource(R.string.table_col_points), 0.20f, colors, TextAlign.End)
    }
}

@Composable
private fun RowScope.HeaderCell(text: String, weight: Float, colors: BkhColors, align: TextAlign = TextAlign.Start) {
    Text(
        text = text,
        style = MaterialTheme.typography.labelSmall,
        color = colors.onHeroMuted,
        textAlign = align,
        modifier = Modifier.weight(weight),
    )
}

@Composable
private fun TableRowView(row: LeagueTableRow, colors: BkhColors) {
    val shape = RoundedCornerShape(10.dp)
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 8.dp, vertical = 2.dp)
            .clip(shape)
            .background(if (row.isPlayer) colors.accent.copy(alpha = 0.16f) else Color.Transparent)
            .padding(horizontal = 6.dp, vertical = 8.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        val weight = if (row.isPlayer) FontWeight.Bold else FontWeight.Normal
        Text(
            text = row.position.toString(),
            style = MaterialTheme.typography.bodyMedium,
            color = if (row.isPlayer) colors.onHero else colors.onHeroMuted,
            fontWeight = weight,
            modifier = Modifier.weight(0.14f),
        )
        Text(
            text = clubLabel(row.name, row.town),
            // Mniejszy stopień: nazwa klubu niesie teraz także miejscowość,
            // a kolumna jest wąska (10 drużyn w tabeli).
            style = MaterialTheme.typography.bodySmall,
            color = colors.onHero,
            fontWeight = weight,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
            modifier = Modifier.weight(0.46f),
        )
        Text(
            text = "${row.goalsFor}:${row.goalsAgainst}",
            style = MaterialTheme.typography.bodyMedium,
            color = colors.onHeroMuted,
            textAlign = TextAlign.Center,
            modifier = Modifier.weight(0.20f),
        )
        Text(
            text = row.points.toString(),
            style = MaterialTheme.typography.bodyMedium,
            color = colors.onHero,
            fontWeight = FontWeight.Bold,
            textAlign = TextAlign.End,
            modifier = Modifier.weight(0.20f),
        )
    }
}

// --- Terminarz --------------------------------------------------------------

private data class FixtureRow(
    val round: Int,
    val home: String,
    val homeTown: String,
    val away: String,
    val awayTown: String,
    val played: Boolean,
    val homeGoals: Int,
    val awayGoals: Int,
)

@Composable
fun FixturesView(
    modifier: Modifier = Modifier,
    contentPadding: PaddingValues = PaddingValues(0.dp),
) {
    val colors = LocalBkhColors.current
    var rows by remember { mutableStateOf<List<FixtureRow>?>(null) }
    var playerShort by remember { mutableStateOf("") }

    LaunchedEffect(Unit) {
        // Skrót klubu gracza pochodzi z tabeli (w terminarzu nie ma flagi gracza).
        NativeBridge.command(buildJsonObject { put("cmd", "table") })
            .onSuccess { data ->
                val arr = data["table"]?.jsonArray ?: return@onSuccess
                playerShort = arr.mapNotNull { it as? JsonObject }
                    .firstOrNull { it["isPlayer"]?.jsonPrimitive?.content?.toBooleanStrictOrNull() == true }
                    ?.text("shortName")
                    .orEmpty()
            }
        NativeBridge.command(buildJsonObject { put("cmd", "fixtures") })
            .onSuccess { data ->
                val arr = data["fixtures"]?.jsonArray ?: return@onSuccess
                rows = arr.mapNotNull { el ->
                    val o = el as? JsonObject ?: return@mapNotNull null
                    FixtureRow(
                        round = o.int("round"),
                        home = o.text("homeShort").ifBlank { o.text("home") },
                        homeTown = o.text("homeTown"),
                        away = o.text("awayShort").ifBlank { o.text("away") },
                        awayTown = o.text("awayTown"),
                        played = o["played"]?.jsonPrimitive?.content?.toBooleanStrictOrNull() ?: false,
                        homeGoals = o.int("homeGoals"),
                        awayGoals = o.int("awayGoals"),
                    )
                }
            }
    }

    LazyColumn(
        modifier = modifier.fillMaxWidth(),
        contentPadding = contentPadding,
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        val grouped = rows?.groupBy { it.round } ?: emptyMap()
        when {
            rows == null -> item { LoadingBox(colors) }
            grouped.isEmpty() -> item { EmptyBox(colors) }
            else -> items(grouped.keys.toList()) { round ->
                LeagueCard(colors) {
                    Text(
                        text = stringResource(R.string.fixtures_round, round),
                        style = MaterialTheme.typography.labelMedium,
                        color = colors.onHeroMuted,
                        modifier = Modifier.padding(start = 14.dp, top = 12.dp, bottom = 4.dp),
                    )
                    grouped.getValue(round).forEach { row ->
                        FixtureRowView(
                            row = row,
                            colors = colors,
                            isPlayer = playerShort.isNotEmpty() &&
                                (row.home == playerShort || row.away == playerShort),
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun FixtureRowView(row: FixtureRow, colors: BkhColors, isPlayer: Boolean) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 14.dp, vertical = 6.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        val weight = if (isPlayer) FontWeight.Bold else FontWeight.Normal
        Text(
            text = clubLabel(row.home, row.homeTown),
            style = MaterialTheme.typography.bodySmall,
            color = if (isPlayer) colors.onHero else colors.onHeroMuted,
            fontWeight = weight,
            maxLines = 2,  // długie pary (patron + miejscowość) mogą się zawinąć
            overflow = TextOverflow.Ellipsis,
            modifier = Modifier.weight(1f),
        )
        Text(
            text = if (row.played) "${row.homeGoals}:${row.awayGoals}" else "—",
            style = MaterialTheme.typography.bodyMedium,
            color = if (isPlayer) colors.accent else colors.onHero,
            fontWeight = FontWeight.Bold,
            textAlign = TextAlign.Center,
            modifier = Modifier.width(48.dp),
        )
        Text(
            text = clubLabel(row.away, row.awayTown),
            style = MaterialTheme.typography.bodySmall,
            color = if (isPlayer) colors.onHero else colors.onHeroMuted,
            fontWeight = weight,
            maxLines = 2,
            overflow = TextOverflow.Ellipsis,
            textAlign = TextAlign.End,
            modifier = Modifier.weight(1f),
        )
    }
}

// --- Klocki wspólne ---------------------------------------------------------

@Composable
private fun LeagueCard(colors: BkhColors, content: @Composable ColumnScope.() -> Unit) {
    Surface(
        shape = RoundedCornerShape(18.dp),
        color = colors.card,
        border = BorderStroke(1.dp, colors.cardBorder),
        modifier = Modifier.fillMaxWidth(),
    ) {
        Column(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp), content = content)
    }
}

@Composable
private fun Caption(text: String, colors: BkhColors) {
    Text(
        text = text,
        style = MaterialTheme.typography.labelMedium,
        color = colors.onHeroMuted,
        modifier = Modifier.padding(start = 4.dp),
    )
}

@Composable
private fun LoadingBox(colors: BkhColors) {
    Box(modifier = Modifier.fillMaxWidth().padding(vertical = 32.dp), contentAlignment = Alignment.Center) {
        CircularProgressIndicator(color = colors.accent, modifier = Modifier.size(28.dp))
    }
}

@Composable
private fun EmptyBox(colors: BkhColors) {
    Text(
        text = stringResource(R.string.league_no_data),
        style = MaterialTheme.typography.bodyMedium,
        color = colors.onHeroMuted,
        textAlign = TextAlign.Center,
        modifier = Modifier.fillMaxWidth().padding(vertical = 32.dp),
    )
}

private fun JsonObject.text(key: String): String =
    (this[key] as? kotlinx.serialization.json.JsonPrimitive)?.content.orEmpty()

private fun JsonObject.int(key: String): Int =
    (this[key] as? kotlinx.serialization.json.JsonPrimitive)?.content?.toIntOrNull() ?: 0
