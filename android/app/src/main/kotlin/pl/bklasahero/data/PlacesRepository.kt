// Repozytorium CSV miejscowości — jednorazowy load do pamięci rdzenia
// przy starcie aplikacji. Dane zostają w rdzeniu, My nie trzymamy kopii.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.data

import android.content.res.AssetManager
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import pl.bklasahero.engine.NativeBridge

class PlacesRepository(
    private val assets: AssetManager,
    private val scope: CoroutineScope,
) {

    suspend fun loadFromAssets(path: String = "places_pl.csv"): Result<Unit> =
        withContext(Dispatchers.IO) {
            val bytes = assets.open(path).use { it.readBytes() }
            NativeBridge.loadPlacesCsv(bytes)
        }
}