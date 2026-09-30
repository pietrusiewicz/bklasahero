// B-Klasa Hero — aktywność główna z Compose.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.viewModels
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import pl.bklasahero.ui.AppNavGraph
import pl.bklasahero.ui.theme.BKlasaHeroTheme

class MainActivity : ComponentActivity() {

    private val viewModel: AppViewModel by viewModels {
        AppViewModel.Factory((application as BKlasaHeroApp).container)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge()
        super.onCreate(savedInstanceState)
        viewModel.initialize()
        setContent {
            val state by viewModel.uiState.collectAsState()
            BKlasaHeroTheme(state.darkTheme) {
                AppNavGraph(state, viewModel)
            }
        }
    }
}