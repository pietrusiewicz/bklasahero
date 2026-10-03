// Test JVM etykiety klubu — pilnuje, żeby tabela i terminarz pokazywały
// miejscowość klubu (związek z mapą), ale bez powtarzania jej, gdy skrót
// nazwy już ją niesie.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.components

import org.junit.Assert.assertEquals
import org.junit.Test

class ClubLabelTest {

    @Test
    fun appendsTownToPatronShortName() {
        assertEquals("Orzeł Węgrzynowo", clubLabel("Orzeł", "Węgrzynowo"))
        assertEquals("Iskra Bór", clubLabel("Iskra", "Bór"))
        assertEquals("Start Stara Kamionka", clubLabel("Start", "Stara Kamionka"))
    }

    @Test
    fun keepsAdjectivalShortNameWithoutTown() {
        // Forma przymiotnikowa sama mówi, skąd jest klub.
        assertEquals("Węgrzynowianka", clubLabel("Węgrzynowianka", "Węgrzynowo"))
        assertEquals("Bartodziejanka", clubLabel("Bartodziejanka", "Bartodzieje"))
        assertEquals("Sochaczewianka", clubLabel("Sochaczewianka", "Sochaczew"))
    }

    @Test
    fun ignoresDiacriticsWhenComparing() {
        assertEquals("Żurawianka", clubLabel("Żurawianka", "Żurawica"))
        assertEquals("Orzeł Łódź", clubLabel("Orzeł", "Łódź"))
    }

    @Test
    fun handlesMissingParts() {
        assertEquals("Orzeł", clubLabel("Orzeł", ""))
        assertEquals("Węgrzynowo", clubLabel("", "Węgrzynowo"))
        assertEquals("", clubLabel("", ""))
        assertEquals("Orzeł", clubLabel("  Orzeł  ", "  "))
    }

    @Test
    fun keepsShortTownsWhole() {
        // Przy bardzo krótkiej nazwie nie zgadujemy rdzenia — dopisujemy miejscowość.
        assertEquals("Sokół Nida", clubLabel("Sokół", "Nida"))
        assertEquals("Sokół Nisko", clubLabel("Sokół", "Nisko"))
    }
}
