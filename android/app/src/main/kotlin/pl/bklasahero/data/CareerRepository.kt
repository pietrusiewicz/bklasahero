// Repozytorium save'ów — trzyma karierę w bazie i informuje rdzeń o zmianach.
// Jedyne źródło prawdy dla save'a to baza. Rdzeń jest bezstanowy pomiędzy wywołaniami
// `loadCareer` (po restarcie trzeba ponownie wczytać).
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.data

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.launch
import kotlinx.serialization.json.Json
import pl.bklasahero.engine.NativeBridge

class CareerRepository(
    private val dao: CareerDao,
    private val scope: CoroutineScope,
    @Suppress("unused") private val json: Json,
) {

    /** Flow z aktualnym save'm (null = brak kariery). */
    fun observe(): Flow<CareerSaveEntity?> = dao.loadFlow()

    /** Zapisuje bieżącą karierę z rdzenia do bazy. */
    fun persistCurrent() {
        scope.launch {
            NativeBridge.saveCareer()?.let { blob ->
                dao.save(blob, System.currentTimeMillis())
            }
        }
    }

    /** Próba wczytania save'a z bazy do rdzenia. Zwraca true jeśli załadowano. */
    suspend fun loadFromDatabase(): Boolean {
        val entity = dao.load() ?: return false
        return NativeBridge.loadCareer(entity.json).isSuccess
    }

    /** Kasuje karierę z bazy i z rdzenia. */
    fun delete() {
        scope.launch { dao.clear() }
    }
}