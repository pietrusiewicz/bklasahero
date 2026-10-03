// Etykieta klubu do wąskich kolumn (tabela, terminarz).
//
// Nazwy w grze są generowane z miejscowości: „LKS Orzeł Węgrzynowo". W tabeli
// zostaje sam skrót („Orzeł"), przez co ginie związek z mapą. Dlatego skrót
// sklejamy z miejscowością — ale tylko wtedy, gdy skrót jej nie niesie
// (forma przymiotnikowa „Węgrzynowianka" już mówi, skąd klub jest).
//
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.components

/** Minimalna długość rdzenia miejscowości, żeby skracanie miało sens. */
private const val kStemLength = 5

/** „Orzeł" + „Węgrzynowo" → „Orzeł Węgrzynowo"; „Węgrzynowianka" → bez zmian. */
internal fun clubLabel(shortName: String, town: String): String {
    val short = shortName.trim()
    val place = town.trim()
    if (place.isEmpty()) return short
    if (short.isEmpty()) return place
    if (place.length >= kStemLength) {
        val stem = normalizedPl(place).take(kStemLength)
        if (stem.isNotEmpty() && normalizedPl(short).contains(stem)) return short
    }
    return "$short $place"
}

/** Małe litery bez ogonków — porównanie „Węgrzynowianka" z „Węgrzynowo". */
private fun normalizedPl(text: String): String {
    val lowered = text.lowercase()
    val decomposed = java.text.Normalizer.normalize(lowered, java.text.Normalizer.Form.NFD)
    val builder = StringBuilder(decomposed.length)
    for (ch in decomposed) {
        // Usuwamy znaki diakrytyczne (kategoria Mn) i zostawiamy litery/cyfry.
        if (ch.isLetterOrDigit()) builder.append(ch)
    }
    return builder.toString()
}
