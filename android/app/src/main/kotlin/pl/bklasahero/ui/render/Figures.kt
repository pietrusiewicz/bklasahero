// Sylwetki zawodników rysowane na Canvas — wspólne dla meczu i samouczka.
//
// Celowo NIE są to patyki: figura ma proporcje ~7 głów, ręce (bark → łokieć →
// dłoń), nogi (udo → kolano → łydka → getry → but), tors z rękawami, szyję,
// głowę z włosami i lekkie cieniowanie. Bramkarz jest tą samą figurą z rękami
// nad głową, obróconą o kąt nurkowania.
// SPDX-License-Identifier: GPL-3.0-or-later
package pl.bklasahero.ui.render

import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.rotate

/** Kolory stroju zawodnika. */
data class KitColors(
    val jersey: Color,
    val shorts: Color,
    val socks: Color,
    val boots: Color = Color(0xFF16181C),
    val skin: Color = Color(0xFFE8B48C),
    val hair: Color = Color(0xFF2B2118),
)

private val Shade = Color(0x26000000)

/**
 * Piłkarz widziany OD TYŁU (kamera rzutu karnego).
 *
 * @param feetY linia stóp (dół figury)
 * @param height wzrost zawodnika w pikselach
 */
fun DrawScope.drawPlayerBackView(
    centerX: Float,
    feetY: Float,
    height: Float,
    kit: KitColors,
    number: Boolean = true,
) {
    val h = height
    val cx = centerX
    val ankleY = feetY - 0.055f * h
    val kneeY = feetY - 0.28f * h
    val hipY = feetY - 0.47f * h
    val waistY = feetY - 0.60f * h
    val shoulderY = feetY - 0.79f * h
    val headR = 0.072f * h
    val headCY = feetY - 0.895f * h
    val legX = 0.062f * h
    val hipX = 0.052f * h
    val shoulderHalf = 0.135f * h
    val armW = 0.055f * h

    // --- nogi ---
    for (side in floatArrayOf(-1f, 1f)) {
        val hip = Offset(cx + side * hipX, hipY)
        val knee = Offset(cx + side * legX, kneeY)
        val ankle = Offset(cx + side * legX, ankleY)
        // udo i łydka (skóra)
        drawLine(kit.skin, hip, knee, strokeWidth = 0.088f * h, cap = StrokeCap.Round)
        drawLine(kit.skin, knee, ankle, strokeWidth = 0.062f * h, cap = StrokeCap.Round)
        // getry
        drawLine(kit.socks, Offset(knee.x, knee.y + 0.02f * h), ankle, strokeWidth = 0.068f * h, cap = StrokeCap.Round)
        // but
        drawRoundRect(
            color = kit.boots,
            topLeft = Offset(ankle.x - 0.058f * h, ankle.y - 0.010f * h),
            size = Size(0.116f * h, 0.046f * h),
            cornerRadius = CornerRadius(0.020f * h, 0.020f * h),
        )
    }

    // --- spodenki ---
    drawRoundRect(
        color = kit.shorts,
        topLeft = Offset(cx - 0.105f * h, waistY - 0.005f * h),
        size = Size(0.210f * h, 0.165f * h),
        cornerRadius = CornerRadius(0.035f * h, 0.035f * h),
    )

    // --- tors (koszulka, zwężona w talii) ---
    val torso = Path().apply {
        moveTo(cx - shoulderHalf, shoulderY)
        lineTo(cx + shoulderHalf, shoulderY)
        lineTo(cx + 0.108f * h, waistY)
        lineTo(cx - 0.108f * h, waistY)
        close()
    }
    drawPath(torso, color = kit.jersey)
    // cieniowanie lewej strony + delikatny rozjaśniony środek
    val side = Path().apply {
        moveTo(cx - shoulderHalf, shoulderY)
        lineTo(cx - 0.035f * h, shoulderY)
        lineTo(cx - 0.035f * h, waistY)
        lineTo(cx - 0.108f * h, waistY)
        close()
    }
    drawPath(side, color = Shade)
    // rękawy
    for (s in floatArrayOf(-1f, 1f)) {
        drawRoundRect(
            color = kit.jersey,
            topLeft = Offset(
                if (s < 0f) cx - shoulderHalf - 0.030f * h else cx + shoulderHalf - 0.015f * h,
                shoulderY - 0.005f * h,
            ),
            size = Size(0.045f * h, 0.085f * h),
            cornerRadius = CornerRadius(0.018f * h, 0.018f * h),
        )
    }
    // numer na plecach
    if (number) {
        drawCircle(Color(0x33FFFFFF), radius = 0.042f * h, center = Offset(cx, waistY - 0.075f * h))
    }

    // --- ręce: bark → łokieć (koszulka), łokieć → dłoń (skóra) ---
    for (s in floatArrayOf(-1f, 1f)) {
        val shoulder = Offset(cx + s * (shoulderHalf - 0.012f * h), shoulderY + 0.018f * h)
        val elbow = Offset(cx + s * 0.168f * h, waistY - 0.035f * h)
        val hand = Offset(cx + s * 0.170f * h, hipY - 0.010f * h)
        drawLine(kit.jersey, shoulder, elbow, strokeWidth = armW, cap = StrokeCap.Round)
        drawLine(kit.skin, elbow, hand, strokeWidth = 0.044f * h, cap = StrokeCap.Round)
        drawCircle(kit.skin, 0.026f * h, hand)
    }

    // --- szyja, głowa, włosy ---
    drawRoundRect(
        color = kit.skin,
        topLeft = Offset(cx - 0.026f * h, shoulderY - 0.055f * h),
        size = Size(0.052f * h, 0.06f * h),
        cornerRadius = CornerRadius(0.018f * h, 0.018f * h),
    )
    drawCircle(kit.skin, headR, Offset(cx, headCY))
    // widok od tyłu → włosy zasłaniają czubek i tył głowy
    drawCircle(kit.hair, headR * 0.99f, Offset(cx, headCY - headR * 0.20f))
}

/**
 * Bramkarz. Przy `progress = 0` stoi z rękami w górze; obrót o `dir * 82°`
 * daje pełne nurkowanie w bok (ręce wyciągnięte do piłki).
 *
 * @param feetY linia stóp
 * @param height wzrost (bez rąk)
 * @param dir -1 = w lewo, +1 = w prawo
 */
fun DrawScope.drawGoalkeeper(
    feetX: Float,
    feetY: Float,
    height: Float,
    kit: KitColors,
    dir: Float,
    progress: Float,
    glove: Color = Color(0xFF3A3F45),
) {
    val h = height
    val cx = feetX
    val pivot = Offset(cx, feetY - 0.45f * h)
    rotate(degrees = dir * 82f * progress.coerceIn(0f, 1f), pivot = pivot) {
        val ankleY = feetY - 0.055f * h
        val kneeY = feetY - 0.28f * h
        val hipY = feetY - 0.46f * h
        val waistY = feetY - 0.58f * h
        val shoulderY = feetY - 0.76f * h
        val headR = 0.072f * h
        val headCY = feetY - 0.865f * h
        val legX = 0.055f * h

        // nogi
        for (s in floatArrayOf(-1f, 1f)) {
            val hip = Offset(cx + s * 0.045f * h, hipY)
            val knee = Offset(cx + s * legX, kneeY)
            val ankle = Offset(cx + s * legX * 0.95f, ankleY)
            drawLine(kit.skin, hip, knee, strokeWidth = 0.085f * h, cap = StrokeCap.Round)
            drawLine(kit.skin, knee, ankle, strokeWidth = 0.060f * h, cap = StrokeCap.Round)
            drawLine(kit.socks, Offset(knee.x, knee.y + 0.02f * h), ankle, strokeWidth = 0.066f * h, cap = StrokeCap.Round)
            drawRoundRect(
                color = kit.boots,
                topLeft = Offset(ankle.x - 0.055f * h, ankle.y - 0.010f * h),
                size = Size(0.110f * h, 0.044f * h),
                cornerRadius = CornerRadius(0.018f * h, 0.018f * h),
            )
        }

        // spodenki + tors
        drawRoundRect(
            color = kit.shorts,
            topLeft = Offset(cx - 0.100f * h, waistY - 0.005f * h),
            size = Size(0.200f * h, 0.155f * h),
            cornerRadius = CornerRadius(0.033f * h, 0.033f * h),
        )
        val shoulderHalf = 0.130f * h
        val torso = Path().apply {
            moveTo(cx - shoulderHalf, shoulderY)
            lineTo(cx + shoulderHalf, shoulderY)
            lineTo(cx + 0.104f * h, waistY)
            lineTo(cx - 0.104f * h, waistY)
            close()
        }
        drawPath(torso, color = kit.jersey)
        val side = Path().apply {
            moveTo(cx - shoulderHalf, shoulderY)
            lineTo(cx - 0.032f * h, shoulderY)
            lineTo(cx - 0.032f * h, waistY)
            lineTo(cx - 0.104f * h, waistY)
            close()
        }
        drawPath(side, color = Shade)
        for (s in floatArrayOf(-1f, 1f)) {
            drawRoundRect(
                color = kit.jersey,
                topLeft = Offset(
                    if (s < 0f) cx - shoulderHalf - 0.028f * h else cx + shoulderHalf - 0.014f * h,
                    shoulderY - 0.005f * h,
                ),
                size = Size(0.043f * h, 0.082f * h),
                cornerRadius = CornerRadius(0.017f * h, 0.017f * h),
            )
        }

        // ręce wyciągnięte nad głowę (po obrocie — w bok, do piłki)
        val reach = 1.02f + 0.10f * progress
        for (s in floatArrayOf(-1f, 1f)) {
            val shoulder = Offset(cx + s * (shoulderHalf - 0.012f * h), shoulderY + 0.015f * h)
            val hand = Offset(cx + s * 0.115f * h, feetY - reach * h)
            drawLine(kit.jersey, shoulder, Offset(cx + s * 0.105f * h, feetY - 0.92f * h), strokeWidth = 0.052f * h, cap = StrokeCap.Round)
            drawLine(kit.skin, Offset(cx + s * 0.105f * h, feetY - 0.92f * h), hand, strokeWidth = 0.044f * h, cap = StrokeCap.Round)
            drawCircle(glove, 0.032f * h, hand)
        }

        // szyja, głowa, włosy (bramkarz patrzy na strzelca — widzimy twarz)
        drawRoundRect(
            color = kit.skin,
            topLeft = Offset(cx - 0.026f * h, shoulderY - 0.055f * h),
            size = Size(0.052f * h, 0.06f * h),
            cornerRadius = CornerRadius(0.018f * h, 0.018f * h),
        )
        drawCircle(kit.skin, headR, Offset(cx, headCY))
        // włosy tylko z góry
        drawArc(
            color = kit.hair,
            startAngle = 180f,
            sweepAngle = 180f,
            useCenter = true,
            topLeft = Offset(cx - headR, headCY - headR * 1.05f),
            size = Size(headR * 2f, headR * 2f),
        )
    }
}
