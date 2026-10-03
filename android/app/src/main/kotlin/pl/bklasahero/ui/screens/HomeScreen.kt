// Ekran główny — menu gry: nagłówek (z przełącznikiem motywu i pomocą),
// skrót kariery, „Następny mecz” oraz przesuwalne zakładki
// Tabela / Terminarz / Kariera. Kolory bierzemy z motywu, więc zmiana
// trybu jasny/ciemny obejmuje cały ekran.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawing
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.pager.HorizontalPager
import androidx.compose.foundation.pager.rememberPagerState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.outlined.HelpOutline
import androidx.compose.material.icons.outlined.DarkMode
import androidx.compose.material.icons.outlined.LightMode
import androidx.compose.material.icons.outlined.RestartAlt
import androidx.compose.material.icons.outlined.SportsSoccer
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Tab
import androidx.compose.material3.TabRow
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.runtime.snapshotFlow
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.launch
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.JsonPrimitive
import pl.bklasahero.AppViewModel
import pl.bklasahero.BuildConfig
import pl.bklasahero.R
import pl.bklasahero.ui.AppIntent
import pl.bklasahero.ui.AppScreen
import pl.bklasahero.ui.AppUiState
import pl.bklasahero.ui.components.FixturesView
import pl.bklasahero.ui.components.LeagueTableView
import pl.bklasahero.ui.theme.BkhColors
import pl.bklasahero.ui.theme.LocalBkhColors

@Composable
fun HomeScreen(state: AppUiState, viewModel: AppViewModel) {
    val colors = LocalBkhColors.current
    val dispatch = viewModel::dispatch
    var confirmNewCareer by remember { mutableStateOf(false) }

    val career = state.careerJson
    val hasCareer = state.careerReady

    // Po restarcie aplikacji rdzeń ma karierę, ale ekran nie zna jeszcze jej
    // podsumowania (a odpowiedź `newCareer` nie zawiera nicku ani XP) —
    // dociągamy je raz, żeby menu nie pokazywało pustych pól.
    LaunchedEffect(hasCareer, career) {
        if (hasCareer && career.str("nickname").isBlank()) viewModel.refreshCareer()
    }

    val placeholderName = stringResource(R.string.menu_career)
    val nickname = career.str("nickname").ifBlank { placeholderName }
    val leagueLabel = career.str("leagueLabel").ifBlank {
        stringResource(R.string.career_tier, career.int("tierIndex") ?: 0)
    }
    val season = career.int("seasonNumber") ?: 1

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Brush.verticalGradient(listOf(colors.heroTop, colors.heroBottom))),
    ) {
        Column(
            modifier = Modifier
                .align(Alignment.TopCenter)
                .fillMaxWidth()
                .widthIn(max = 560.dp)
                .windowInsetsPadding(WindowInsets.safeDrawing)
                .padding(start = 20.dp, end = 20.dp, top = 12.dp, bottom = 4.dp),
        ) {
            MenuHeader(
                colors = colors,
                darkTheme = state.darkTheme,
                onToggleTheme = { dispatch(AppIntent.ToggleTheme) },
                onHelp = { dispatch(AppIntent.SetScreen(AppScreen.Tutorial)) },
            )

            Spacer(Modifier.height(14.dp))

            if (hasCareer) {
                CareerHero(
                    colors = colors,
                    nickname = nickname,
                    subtitle = "$leagueLabel · ${stringResource(R.string.home_season, season)}",
                    homeCity = career.str("homeCity"),
                )
                Spacer(Modifier.height(6.dp))
                HomeTabs(
                    state = state,
                    viewModel = viewModel,
                    colors = colors,
                    onNewCareer = { confirmNewCareer = true },
                    modifier = Modifier.weight(1f),
                )
            } else {
                StartCard(colors = colors)
                Spacer(Modifier.height(10.dp))
                OutlinedButton(
                    onClick = { dispatch(AppIntent.SetScreen(AppScreen.Tutorial)) },
                    shape = RoundedCornerShape(16.dp),
                    modifier = Modifier.fillMaxWidth().height(48.dp),
                ) {
                    Text(stringResource(R.string.menu_how_to_play))
                }
                Spacer(Modifier.height(12.dp))
                Text(
                    text = stringResource(R.string.home_locked_hint),
                    style = MaterialTheme.typography.bodySmall,
                    color = colors.onHeroMuted,
                )
                Spacer(Modifier.weight(1f))
            }

            // Główna akcja zawsze na dole ekranu — pod kciukiem.
            Button(
                onClick = {
                    dispatch(
                        AppIntent.SetScreen(if (hasCareer) AppScreen.Match else AppScreen.NewCareer)
                    )
                },
                shape = RoundedCornerShape(18.dp),
                modifier = Modifier.fillMaxWidth().height(58.dp),
            ) {
                Icon(Icons.Outlined.SportsSoccer, contentDescription = null)
                Spacer(Modifier.width(10.dp))
                Text(
                    text = stringResource(R.string.home_play),
                    style = MaterialTheme.typography.titleLarge,
                    fontWeight = FontWeight.Bold,
                )
            }

            Text(
                text = stringResource(
                    R.string.home_footer,
                    BuildConfig.VERSION_NAME,
                    state.protocolVersion,
                ),
                style = MaterialTheme.typography.bodySmall,
                color = colors.onHeroMuted,
                textAlign = TextAlign.Center,
                modifier = Modifier.fillMaxWidth().padding(vertical = 6.dp),
            )
        }
    }

    if (confirmNewCareer) {
        AlertDialog(
            onDismissRequest = { confirmNewCareer = false },
            title = { Text(stringResource(R.string.home_new_career_confirm_title)) },
            text = { Text(stringResource(R.string.home_new_career_confirm_body, nickname)) },
            confirmButton = {
                TextButton(onClick = {
                    confirmNewCareer = false
                    dispatch(AppIntent.SetScreen(AppScreen.NewCareer))
                }) {
                    Text(stringResource(R.string.home_new_career_confirm_ok), color = colors.danger)
                }
            },
            dismissButton = {
                TextButton(onClick = { confirmNewCareer = false }) {
                    Text(stringResource(R.string.common_cancel))
                }
            },
        )
    }
}

// --- Zakładki: Tabela / Terminarz / Kariera --------------------------------

@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun HomeTabs(
    state: AppUiState,
    viewModel: AppViewModel,
    colors: BkhColors,
    onNewCareer: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val titles = listOf(R.string.career_table, R.string.career_fixtures, R.string.menu_career)
    val pagerState = rememberPagerState(
        initialPage = state.homeTab.coerceIn(0, titles.lastIndex),
        pageCount = { titles.size },
    )
    val scope = rememberCoroutineScope()
    val pagePadding = PaddingValues(top = 12.dp, bottom = 12.dp)

    // Przesunięcie palcem (i tapnięcie w zakładkę) zapisujemy w stanie, żeby
    // powrót do menu wracał na tę samą zakładkę.
    LaunchedEffect(pagerState) {
        snapshotFlow { pagerState.currentPage }.collect { viewModel.dispatch(AppIntent.SetHomeTab(it)) }
    }

    Column(modifier = modifier.fillMaxWidth()) {
        TabRow(
            selectedTabIndex = pagerState.currentPage,
            containerColor = Color.Transparent,
            contentColor = colors.onHero,
            divider = {},
        ) {
            titles.forEachIndexed { index, titleRes ->
                val selected = pagerState.currentPage == index
                Tab(
                    selected = selected,
                    onClick = { scope.launch { pagerState.animateScrollToPage(index) } },
                    text = {
                        Text(
                            text = stringResource(titleRes),
                            style = MaterialTheme.typography.titleSmall,
                            fontWeight = if (selected) FontWeight.SemiBold else FontWeight.Normal,
                            color = if (selected) colors.onHero else colors.onHeroMuted,
                            maxLines = 1,
                        )
                    },
                )
            }
        }
        HorizontalPager(
            state = pagerState,
            modifier = Modifier.fillMaxWidth().weight(1f),
        ) { page ->
            when (page) {
                0 -> LeagueTableView(
                    modifier = Modifier.fillMaxSize(),
                    contentPadding = pagePadding,
                )
                1 -> FixturesView(
                    modifier = Modifier.fillMaxSize(),
                    contentPadding = pagePadding,
                )
                else -> CareerTab(
                    state = state,
                    viewModel = viewModel,
                    colors = colors,
                    onNewCareer = onNewCareer,
                    modifier = Modifier.fillMaxSize(),
                    contentPadding = pagePadding,
                )
            }
        }
    }
}

@Composable
private fun CareerTab(
    state: AppUiState,
    viewModel: AppViewModel,
    colors: BkhColors,
    onNewCareer: () -> Unit,
    modifier: Modifier = Modifier,
    contentPadding: PaddingValues = PaddingValues(0.dp),
) {
    val career = state.careerJson
    val level = career.int("level") ?: 0
    val xp = career.int("xp") ?: 0
    val xpToNext = career.int("xpToNext") ?: 0

    LazyColumn(
        modifier = modifier.fillMaxWidth(),
        contentPadding = contentPadding,
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        if (xpToNext > 0) {
            item {
                Surface(
                    shape = RoundedCornerShape(18.dp),
                    color = colors.card,
                    border = BorderStroke(1.dp, colors.cardBorder),
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    Column(
                        modifier = Modifier.fillMaxWidth().padding(14.dp),
                        verticalArrangement = Arrangement.spacedBy(8.dp),
                    ) {
                        Text(
                            text = stringResource(R.string.home_level_xp, level, xp, xpToNext),
                            style = MaterialTheme.typography.bodyMedium,
                            color = colors.onHero,
                            fontWeight = FontWeight.SemiBold,
                        )
                        LinearProgressIndicator(
                            progress = { (xp.toFloat() / xpToNext.toFloat()).coerceIn(0f, 1f) },
                            color = colors.accent,
                            trackColor = colors.cardBorder,
                            modifier = Modifier.fillMaxWidth(),
                        )
                    }
                }
            }
        }
        item {
            Column(verticalArrangement = Arrangement.spacedBy(10.dp)) {
                Row(horizontalArrangement = Arrangement.spacedBy(10.dp), modifier = Modifier.fillMaxWidth()) {
                    StatTile(
                        label = stringResource(R.string.stat_matches),
                        value = career.int("matches") ?: 0,
                        colors = colors,
                        modifier = Modifier.weight(1f),
                    )
                    StatTile(
                        label = stringResource(R.string.stat_wins),
                        value = career.int("wins") ?: 0,
                        colors = colors,
                        modifier = Modifier.weight(1f),
                    )
                }
                Row(horizontalArrangement = Arrangement.spacedBy(10.dp), modifier = Modifier.fillMaxWidth()) {
                    StatTile(
                        label = stringResource(R.string.stat_goals),
                        value = career.int("goalsScored") ?: 0,
                        colors = colors,
                        modifier = Modifier.weight(1f),
                    )
                    StatTile(
                        label = stringResource(R.string.stat_saves),
                        value = career.int("saves") ?: 0,
                        colors = colors,
                        modifier = Modifier.weight(1f),
                    )
                }
            }
        }
        item {
            Surface(
                shape = RoundedCornerShape(18.dp),
                color = colors.card,
                border = BorderStroke(1.dp, colors.cardBorder),
                modifier = Modifier.fillMaxWidth(),
            ) {
                Column(modifier = Modifier.fillMaxWidth()) {
                    Box(modifier = Modifier.fillMaxWidth().padding(14.dp)) {
                        OutlinedButton(
                            onClick = { viewModel.dispatch(AppIntent.SaveCareer) },
                            shape = RoundedCornerShape(14.dp),
                            modifier = Modifier.fillMaxWidth().height(46.dp),
                        ) {
                            Text(stringResource(R.string.career_save))
                        }
                    }
                    HorizontalDivider(color = colors.cardBorder)
                    OptionRow(
                        icon = Icons.Outlined.RestartAlt,
                        title = stringResource(R.string.home_new_career),
                        subtitle = stringResource(R.string.home_new_career_hint),
                        colors = colors,
                        tint = colors.danger,
                        onClick = onNewCareer,
                    ) {
                        Text(
                            text = "›",
                            style = MaterialTheme.typography.titleLarge,
                            color = colors.danger,
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun StatTile(label: String, value: Int, colors: BkhColors, modifier: Modifier = Modifier) {
    Surface(
        shape = RoundedCornerShape(18.dp),
        color = colors.card,
        border = BorderStroke(1.dp, colors.cardBorder),
        modifier = modifier,
    ) {
        Column(
            modifier = Modifier.fillMaxWidth().padding(14.dp),
            verticalArrangement = Arrangement.spacedBy(2.dp),
        ) {
            Text(
                text = value.toString(),
                style = MaterialTheme.typography.headlineSmall,
                fontWeight = FontWeight.Bold,
                color = colors.onHero,
            )
            Text(
                text = label,
                style = MaterialTheme.typography.bodySmall,
                color = colors.onHeroMuted,
            )
        }
    }
}

// --- Elementy menu ---------------------------------------------------------

@Composable
private fun MenuHeader(
    colors: BkhColors,
    darkTheme: Boolean,
    onToggleTheme: () -> Unit,
    onHelp: () -> Unit,
) {
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier = Modifier.fillMaxWidth(),
    ) {
        Box(
            modifier = Modifier
                .size(48.dp)
                .clip(CircleShape)
                .background(Brush.linearGradient(listOf(colors.accent, colors.heroTop))),
            contentAlignment = Alignment.Center,
        ) {
            Icon(
                imageVector = Icons.Outlined.SportsSoccer,
                contentDescription = null,
                tint = Color.White,
                modifier = Modifier.size(28.dp),
            )
        }
        Spacer(Modifier.width(14.dp))
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = stringResource(R.string.app_name),
                style = MaterialTheme.typography.titleLarge,
                fontWeight = FontWeight.Bold,
                color = colors.onHero,
                maxLines = 1,
            )
            Text(
                text = stringResource(R.string.app_subtitle),
                style = MaterialTheme.typography.bodySmall,
                color = colors.onHeroMuted,
                maxLines = 1,
            )
        }
        RoundIconButton(
            icon = Icons.AutoMirrored.Outlined.HelpOutline,
            description = stringResource(R.string.menu_how_to_play),
            colors = colors,
            onClick = onHelp,
        )
        Spacer(Modifier.width(8.dp))
        RoundIconButton(
            icon = if (darkTheme) Icons.Outlined.LightMode else Icons.Outlined.DarkMode,
            description = stringResource(R.string.menu_theme),
            colors = colors,
            onClick = onToggleTheme,
        )
    }
}

/** Ikona-akcja w nagłówku: okrągłe tło, żeby była czytelna na gradiencie. */
@Composable
private fun RoundIconButton(
    icon: ImageVector,
    description: String,
    colors: BkhColors,
    onClick: () -> Unit,
) {
    IconButton(
        onClick = onClick,
        modifier = Modifier
            .size(42.dp)
            .clip(CircleShape)
            .background(colors.card),
    ) {
        Icon(
            imageVector = icon,
            contentDescription = description,
            tint = colors.onHero,
            modifier = Modifier.size(22.dp),
        )
    }
}

@Composable
private fun CareerHero(
    colors: BkhColors,
    nickname: String,
    subtitle: String,
    homeCity: String,
) {
    Surface(
        shape = RoundedCornerShape(18.dp),
        color = colors.card,
        border = BorderStroke(1.dp, colors.cardBorder),
        modifier = Modifier.fillMaxWidth(),
    ) {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            modifier = Modifier.fillMaxWidth().padding(14.dp),
        ) {
            Box(
                modifier = Modifier
                    .size(42.dp)
                    .clip(CircleShape)
                    .background(colors.accent),
                contentAlignment = Alignment.Center,
            ) {
                Text(
                    text = nickname.trim().take(1).uppercase().ifBlank { "?" },
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.Bold,
                    color = Color.White,
                )
            }
            Spacer(Modifier.width(12.dp))
            Column(modifier = Modifier.weight(1f)) {
                Text(
                    text = stringResource(R.string.career_hello, nickname),
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.Bold,
                    color = colors.onHero,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )
                Text(
                    text = subtitle,
                    style = MaterialTheme.typography.bodySmall,
                    color = colors.onHeroMuted,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )
                if (homeCity.isNotBlank()) {
                    Text(
                        text = homeCity,
                        style = MaterialTheme.typography.bodySmall,
                        color = colors.onHeroMuted,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis,
                    )
                }
            }
        }
    }
}

@Composable
private fun StartCard(colors: BkhColors) {
    Surface(
        shape = RoundedCornerShape(18.dp),
        color = colors.card,
        border = BorderStroke(1.dp, colors.cardBorder),
        modifier = Modifier.fillMaxWidth(),
    ) {
        Column(
            modifier = Modifier.fillMaxWidth().padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(6.dp),
        ) {
            Text(
                text = stringResource(R.string.home_start_title),
                style = MaterialTheme.typography.titleMedium,
                fontWeight = FontWeight.SemiBold,
                color = colors.onHero,
            )
            Text(
                text = stringResource(R.string.home_start_hint),
                style = MaterialTheme.typography.bodyMedium,
                color = colors.onHeroMuted,
            )
        }
    }
}

@Composable
private fun OptionRow(
    icon: ImageVector,
    title: String,
    subtitle: String,
    colors: BkhColors,
    onClick: () -> Unit,
    tint: Color = colors.onHero,
    trailing: @Composable () -> Unit,
) {
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier = Modifier
            .fillMaxWidth()
            .clickable(onClick = onClick)
            .padding(horizontal = 16.dp, vertical = 12.dp),
    ) {
        Icon(imageVector = icon, contentDescription = null, tint = tint)
        Spacer(Modifier.width(14.dp))
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = title,
                style = MaterialTheme.typography.titleSmall,
                fontWeight = FontWeight.SemiBold,
                color = tint,
            )
            Text(
                text = subtitle,
                style = MaterialTheme.typography.bodySmall,
                color = colors.onHeroMuted,
            )
        }
        trailing()
    }
}

// --- Pomocnicze odczyty JSON-a --------------------------------------------

/** Bezpieczny odczyt pola tekstowego (brak klucza / inny typ → ""). */
private fun JsonObject?.str(key: String): String =
    (this?.get(key) as? JsonPrimitive)?.content.orEmpty()

/** Bezpieczny odczyt pola liczbowego (brak klucza / inny typ → null). */
private fun JsonObject?.int(key: String): Int? =
    (this?.get(key) as? JsonPrimitive)?.content?.toIntOrNull()
