// Mostek JNI — punkty wejścia dla pl.bklasahero.engine.NativeBridge.
//
// Wszystkie metody są noexcept(true) z perspektywy Javy: każdy wyjątek C++
// jest łapany i zamieniany na RuntimeException, a każdy błąd domenowy
// zwracany jako JSON {"ok":false,...} przez kanał kontrolny.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <jni.h>

#include <cstdint>
#include <memory>
#include <new>
#include <string>

#include "bkh_bridge/jni_env.h"
#include "bkh/facade.h"
#include "bkh/types.h"



extern "C" {

// ----- Kanał kontrolny --------------------------------------------------

JNIEXPORT jstring JNICALL
Java_pl_bklasahero_engine_NativeBridge_nativeCommand(JNIEnv* env, jclass,
                                                     jstring jCmd) {
    const std::string cmd = bkh::jni::toUtf8(env, jCmd);
    if (env->ExceptionCheck()) return nullptr;
    std::string out;
    try {
        out =
bkh::jni::facade().command(cmd);
    } catch (const std::bad_alloc&) {
        bkh::jni::throwRuntimeError(env, "bkh: bad_alloc during command()");
        return nullptr;
    } catch (const std::exception& e) {
        bkh::jni::throwRuntimeError(env, std::string("bkh: ") + e.what());
        return nullptr;
    } catch (...) {
        bkh::jni::throwRuntimeError(env, "bkh: unknown native exception");
        return nullptr;
    }
    return bkh::jni::fromUtf8(env, out);
}

// ----- Save / load (bloby) ----------------------------------------------

JNIEXPORT jstring JNICALL
Java_pl_bklasahero_engine_NativeBridge_nativeSaveCareer(JNIEnv* env, jclass,
                                                        jlong savedAtEpochMs) {
    std::string out;
    try {
        out =
bkh::jni::facade().saveCareerToJson(static_cast<std::int64_t>(savedAtEpochMs));
    } catch (const std::exception& e) {
        bkh::jni::throwRuntimeError(env, std::string("bkh: ") + e.what());
        return nullptr;
    }
    return bkh::jni::fromUtf8(env, out);
}

JNIEXPORT jboolean JNICALL
Java_pl_bklasahero_engine_NativeBridge_nativeLoadCareer(JNIEnv* env, jclass,
                                                        jstring jJson) {
    const std::string json = bkh::jni::toUtf8(env, jJson);
    if (env->ExceptionCheck()) return JNI_FALSE;
    std::string err;
    bool ok = false;
    try {
        ok =
bkh::jni::facade().loadCareerFromJson(json, err);
    } catch (const std::exception& e) {
        bkh::jni::throwRuntimeError(env, std::string("bkh: ") + e.what());
        return JNI_FALSE;
    }
    if (!ok && !err.empty()) {
        bkh::jni::throwRuntimeError(env, err);
        return JNI_FALSE;
    }
    return ok ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_pl_bklasahero_engine_NativeBridge_nativeLoadPlacesCsv(JNIEnv* env, jclass,
                                                           jbyteArray jBytes) {
    if (jBytes == nullptr) {
        bkh::jni::throwRuntimeError(env, "places CSV bytes are null");
        return JNI_FALSE;
    }
    const jsize len = env->GetArrayLength(jBytes);
    jbyte* raw = env->GetByteArrayElements(jBytes, nullptr);
    if (raw == nullptr) return JNI_FALSE;
    std::string err;
    bool ok = false;
    try {
        ok =
bkh::jni::facade().loadPlaces(reinterpret_cast<const char*>(raw),
                                           static_cast<std::size_t>(len), err);
    } catch (const std::exception& e) {
        bkh::jni::throwRuntimeError(env, std::string("bkh: ") + e.what());
        env->ReleaseByteArrayElements(jBytes, raw, JNI_ABORT);
        return JNI_FALSE;
    }
    env->ReleaseByteArrayElements(jBytes, raw, JNI_ABORT);
    if (!ok && !err.empty()) {
        bkh::jni::throwRuntimeError(env, err);
        return JNI_FALSE;
    }
    return ok ? JNI_TRUE : JNI_FALSE;
}

// ----- Metainformacje ----------------------------------------------------

JNIEXPORT jint JNICALL
Java_pl_bklasahero_engine_NativeBridge_nativeProtocolVersion(JNIEnv*, jclass) {
    return static_cast<jint>(bkh::kProtocolVersion);
}

JNIEXPORT jint JNICALL
Java_pl_bklasahero_engine_NativeBridge_nativeSaveSchemaVersion(JNIEnv*, jclass) {
    return bkh::Facade::saveSchemaVersion();
}

JNIEXPORT jstring JNICALL
Java_pl_bklasahero_engine_NativeBridge_nativeCoreVersion(JNIEnv* env, jclass) {
    return bkh::jni::fromUtf8(env, bkh::Facade::coreVersion());
}

}  // extern "C"