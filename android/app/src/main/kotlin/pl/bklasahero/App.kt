// B-Klasa Hero — klasa aplikacji. Trzyma globalne zależności.
// Świadomie unikamy Hilt'a — zależności jest kilka, a DI dla 5 klas to overkill.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero

import android.app.Application
import pl.bklasahero.data.AppContainer

class BKlasaHeroApp : Application() {
    lateinit var container: AppContainer
        private set

    override fun onCreate() {
        super.onCreate()
        container = AppContainer(this)
    }
}