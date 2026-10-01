// Ekran główny — bogate menu: karta kariery, szybkie akcje, samouczek, motyw.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive
import pl.bklasahero.R
import pl.bklasahero.ui.AppIntent
import pl.bklasahero.ui.AppScreen
import pl.bklasahero.ui.AppUiState

@Composable
fun HomeScreen(state: AppUiState, dispatch: (AppIntent) -> Unit) {
    val career = state.careerJson
    val nickname = career?.get("nickname")?.jsonPrimitive?.content ?: ""
    val tier = career?.get("tierIndex")?.jsonPrimitive?.content?.toIntOrNull() ?: 0
    val clubName = remember(career) { playerClubShort(career) }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(
                Brush.verticalGradient(
                    listOf(Color(0xFF12335C), Color(0xFF0E2A1E), Color(0xFF2F8B3B))
                )
            )
            .verticalScroll(rememberScrollState())
            .padding(24.dp),
        contentAlignment = Alignment.Center,
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(14.dp),
            modifier = Modifier.fillMaxWidth(),
        ) {
            Text(text = "⚽", fontSize = 64.sp)
            Text(
                text = stringResource(R.string.app_name),
                style = MaterialTheme.typography.displaySmall,
                fontWeight = FontWeight.Bold,
                color = Color.White,
                textAlign = TextAlign.Center,
            )
            Text(
                text = stringResource(R.string.app_subtitle),
                style = MaterialTheme.typography.bodyLarge,
                color = Color(0xFFCFE0F0),
            )

            Spacer(Modifier.height(4.dp))

            if (state.careerReady) {
                // Karta kariery — nick, klub, poziom ligi.
                Surface(
                    shape = RoundedCornerShape(16.dp),
                    color = Color(0xE6202B3A),
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    Column(
                        modifier = Modifier.fillMaxWidth().padding(16.dp),
                        verticalArrangement = Arrangement.spacedBy(2.dp),
                    ) {
                        Text(
                            text = stringResource(R.string.career_hello, nickname),
                            style = MaterialTheme.typography.titleLarge,
                            fontWeight = FontWeight.Bold,
                            color = Color.White,
                        )
                        if (clubName.isNotEmpty()) {
                            Text(
                                text = clubName,
                                style = MaterialTheme.typography.bodyLarge,
                                color = Color(0xFFCFE0F0),
                            )
                        }
                        Text(
                            text = stringResource(R.string.career_tier, tier),
                            style = MaterialTheme.typography.bodyMedium,
                            color = Color(0xFF9FB8CC),
                        )
                    }
                }

                Button(
                    onClick = { dispatch(AppIntent.SetScreen(AppScreen.Career)) },
                    modifier = Modifier.fillMaxWidth().height(56.dp),
                ) {
                    Text(stringResource(R.string.home_continue_career))
                }

                Row(
                    horizontalArrangement = Arrangement.spacedBy(12.dp),
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    OutlinedButton(
                        onClick = { dispatch(AppIntent.SetScreen(AppScreen.Match)) },
                        modifier = Modifier.weight(1f),
                    ) {
                        Text(stringResource(R.string.career_next_match))
                    }
                    OutlinedButton(
                        onClick = { dispatch(AppIntent.SetScreen(AppScreen.Tutorial)) },
                        modifier = Modifier.weight(1f),
                    ) {
                        Text(stringResource(R.string.menu_how_to_play))
                    }
                }
                Row(
                    horizontalArrangement = Arrangement.spacedBy(12.dp),
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    OutlinedButton(
                        onClick = { dispatch(AppIntent.SetScreen(AppScreen.Table)) },
                        modifier = Modifier.weight(1f),
                    ) {
                        Text(stringResource(R.string.career_table))
                    }
                    OutlinedButton(
                        onClick = { dispatch(AppIntent.SetScreen(AppScreen.Fixtures)) },
                        modifier = Modifier.weight(1f),
                    ) {
                        Text(stringResource(R.string.career_fixtures))
                    }
                }

                TextButton(
                    onClick = { dispatch(AppIntent.SetScreen(AppScreen.NewCareer)) },
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    Text(stringResource(R.string.home_new_career), color = Color(0xFFFFB4AB))
                }
            } else {
                Button(
                    onClick = { dispatch(AppIntent.SetScreen(AppScreen.NewCareer)) },
                    modifier = Modifier.fillMaxWidth().height(56.dp),
                ) {
                    Text(stringResource(R.string.home_new_career))
                }
                OutlinedButton(
                    onClick = { dispatch(AppIntent.SetScreen(AppScreen.Tutorial)) },
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    Text(stringResource(R.string.menu_how_to_play))
                }
            }

            TextButton(onClick = { dispatch(AppIntent.ToggleTheme) }) {
                Text(stringResource(R.string.menu_theme), color = Color(0xFFCFE0F0))
            }

            Text(
                text = stringResource(R.string.app_loading_protocol, state.protocolVersion),
                style = MaterialTheme.typography.bodySmall,
                color = Color(0xFF8FA8BC),
            )
        }
    }
}

/** Krótka nazwa klubu gracza z JSON-a kariery (league.clubs[playerClubIndex]). */
private fun playerClubShort(career: JsonObject?): String {
    if (career == null) return ""
    return try {
        val league = career["league"]?.jsonObject ?: return ""
        val clubs = league["clubs"]?.jsonArray ?: return ""
        val idx = league["playerClubIndex"]?.jsonPrimitive?.content?.toIntOrNull() ?: return ""
        if (idx < 0 || idx >= clubs.size) return ""
        val club = clubs[idx].jsonObject
        club["shortName"]?.jsonPrimitive?.content
            ?: club["name"]?.jsonPrimitive?.content
            ?: ""
    } catch (e: Exception) {
        ""
    }
}
