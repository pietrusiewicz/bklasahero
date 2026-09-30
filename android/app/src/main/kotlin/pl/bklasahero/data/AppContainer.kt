// Kontener zależności aplikacji — minimalne, ręczne DI.
// Tu NIE używamy Hilt'a: mamy ~5 zależności i nie potrzebujemy compile-time grafu.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.data

import android.content.Context
import androidx.room.Room
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.serialization.json.Json

class AppContainer(context: Context) {

    /** Globalny scope na wszystkie IO — restartowany z aplikacją. */
    val applicationScope: CoroutineScope = CoroutineScope(SupervisorJob() + Dispatchers.IO)

    val json: Json = Json {
        ignoreUnknownKeys = true
        isLenient = true
        encodeDefaults = true
    }

    val database: AppDatabase = Room.databaseBuilder(
        context.applicationContext,
        AppDatabase::class.java,
        "bkh_career.db",
    )
        .fallbackToDestructiveMigration()  // w wersji 0.x nie mamy jeszcze migracji
        .build()

    val careerDao: CareerDao = database.careerDao()
    val careerRepository: CareerRepository = CareerRepository(careerDao, applicationScope, json)
    val placesRepository: PlacesRepository = PlacesRepository(context.assets, applicationScope)
}