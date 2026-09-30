# B-Klasa Hero — reguły ProGuard dla modułu release.
# JNI: zachowujemy KLASY i NATYWNE metody fasady, ale reszta może być przycięta.
# SPDX-License-Identifier: GPL-3.0-or-later

-keep class pl.bklasahero.engine.NativeBridge {
    native <methods>;
}
-keep class pl.bklasahero.engine.FrameBuffer { *; }

# kotlinx.serialization — nazwy @SerialName muszą zostać.
-keepattributes *Annotation*, InnerClasses
-dontnote kotlinx.serialization.AnnotationsKt
-keepclassmembers class pl.bklasahero.**$Companion {
    kotlinx.serialization.KSerializer serializer(...);
}
-if @kotlinx.serialization.Serializable class **
-keepclassmembers class <1> {
    static <1>$Companion Companion;
}

# Compose: narzędzie samo wstrzykuje, ale dmuchamy na zimne.
-dontwarn androidx.compose.**