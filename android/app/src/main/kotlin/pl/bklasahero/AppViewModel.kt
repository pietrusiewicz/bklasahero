// ViewModel trzymający globalny stan UI. Każda zmiana ekranu idzie przez
// `dispatch`, żeby zachować jedno źródło prawdy i łatwe testowanie.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.update
import pl.bklasahero.data.AppContainer
import pl.bklasahero.ui.AppIntent
import pl.bklasahero.ui.AppUiState
import pl.bklasahero.ui.AppScreen
import pl.bklasahero.ui.reduce

class AppViewModel(private val container: AppContainer) : ViewModel() {

    private val _uiState = MutableStateFlow(AppUiState())
    val uiState: StateFlow<AppUiState> = _uiState

    fun dispatch(intent: AppIntent) {
        _uiState.update { reduce(it, intent, container) }
    }

    fun onProtocolReady(protocolVersion: Int, saveSchemaVersion: Int) {
        _uiState.update {
            it.copy(
                protocolVersion = protocolVersion,
                saveSchemaVersion = saveSchemaVersion,
            )
        }
    }

    class Factory(private val container: AppContainer) : ViewModelProvider.Factory {
        @Suppress("UNCHECKED_CAST")
        override fun <T : ViewModel> create(modelClass: Class<T>): T {
            return AppViewModel(container) as T
        }
    }
}

val AppUiState.currentScreen: AppScreen
    get() = screen