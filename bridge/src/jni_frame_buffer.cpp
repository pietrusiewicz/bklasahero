// Mostek JNI — kanał renderujący (bufor float[]).
// Wysokoprzepustowy kanał binarny dla 60 fps animacji — brak alokacji,
// brak parsowania JSON po stronie Javy.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <jni.h>

#include <cstddef>
#include <memory>
#include <vector>

#include "bkh_bridge/jni_env.h"
#include "bkh/facade.h"

extern "C" {

// Zwraca minimalny rozmiar bufora w floatach dla ostatniego odtworzenia.
// 0 = brak odtwarzania. Kotlin woła to przed allocacją tablicy.
JNIEXPORT jint JNICALL
Java_pl_bklasahero_engine_NativeBridge_nativeFrameFloatCount(JNIEnv* env, jclass) {
    try {
        return static_cast<jint>(bkh::jni::facade().frameFloatCount());
    } catch (...) {
        bkh::jni::throwRuntimeError(env, "bkh: frameFloatCount crashed");
        return 0;
    }
}

JNIEXPORT jboolean JNICALL
Java_pl_bklasahero_engine_NativeBridge_nativeHasPlayback(JNIEnv* env, jclass) {
    try {
        return bkh::jni::facade().hasPlayback() ? JNI_TRUE : JNI_FALSE;
    } catch (...) {
        bkh::jni::throwRuntimeError(env, "bkh: hasPlayback crashed");
        return JNI_FALSE;
    }
}

JNIEXPORT void JNICALL
Java_pl_bklasahero_engine_NativeBridge_nativeClearPlayback(JNIEnv*, jclass) {
    try {
        bkh::jni::facade().clearPlayback();
    } catch (...) {
        // Bezgłośnie — animacja się zakończyła i tak.
    }
}

// Wypełnia bufor float[] danymi ostatniego rzutu. Jeśli bufor jest za mały,
// zwraca UJEMNĄ wartość `-required`, a Java musi powiększyć tablicę.
// Po udanym zapisie zwraca liczbę zapisanych floatów.
JNIEXPORT jint JNICALL
Java_pl_bklasahero_engine_NativeBridge_nativeWriteFrame(JNIEnv* env, jclass,
                                                        jfloatArray jBuffer) {
    if (jBuffer == nullptr) {
        bkh::jni::throwRuntimeError(env, "buffer is null");
        return 0;
    }
    const jsize capacity = env->GetArrayLength(jBuffer);
    if (capacity <= 0) {
        bkh::jni::throwRuntimeError(env, "buffer has non-positive length");
        return 0;
    }
    std::vector<float> tmp(static_cast<std::size_t>(capacity));
    int written = 0;
    try {
        written =
bkh::jni::facade().writeFrame(tmp.data(), static_cast<int>(tmp.size()));
    } catch (const std::exception& e) {
        bkh::jni::throwRuntimeError(env, std::string("bkh: ") + e.what());
        return 0;
    } catch (...) {
        bkh::jni::throwRuntimeError(env, "bkh: writeFrame crashed");
        return 0;
    }
    if (written <= 0) return written;
    env->SetFloatArrayRegion(jBuffer, 0, static_cast<jsize>(written), tmp.data());
    return written;
}

}  // extern "C"