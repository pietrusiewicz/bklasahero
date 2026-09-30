// Mostek JNI — implementacja pomocników RAII.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "bkh_bridge/jni_env.h"

#include <memory>

#include "bkh/facade.h"

namespace bkh::jni {

namespace {
// Singleton Facade przechowywany przez lifetime procesu JVM (loadLibrary +
// unloadClass). Wystarczy nam globalny unique_ptr — bridge żyje od pierwszego
// wywołania do zakończenia procesu.
std::unique_ptr<bkh::Facade> gFacade;
}  // namespace

Facade& facade() {
    if (!gFacade) gFacade = std::make_unique<bkh::Facade>();
    return *gFacade;
}

void throwRuntimeError(JNIEnv* env, std::string_view message) {
    if (env == nullptr) return;
    if (env->ExceptionCheck()) {
        // Już istnieje wyjątek — dorzucamy nasz tylko jeśli JVM nie jest w trakcie obsługi.
        return;
    }
    jclass cls = env->FindClass("java/lang/RuntimeException");
    if (cls == nullptr) {
        env->ExceptionClear();
        return;
    }
    env->ThrowNew(cls, std::string(message).c_str());
    env->DeleteLocalRef(cls);
}

std::string toUtf8(JNIEnv* env, jstring jstr) {
    if (jstr == nullptr) return {};
    const char* raw = env->GetStringUTFChars(jstr, nullptr);
    if (raw == nullptr) {
        checkException(env, "GetStringUTFChars");
        return {};
    }
    std::string out(raw);
    env->ReleaseStringUTFChars(jstr, raw);
    return out;
}

jstring fromUtf8(JNIEnv* env, std::string_view text) {
    if (env == nullptr) return nullptr;
    return env->NewStringUTF(std::string(text).c_str());
}

void checkException(JNIEnv* env, const char* where) {
    if (env != nullptr && env->ExceptionCheck()) {
        env->ExceptionDescribe();
        throwRuntimeError(env, std::string("JNI exception in ") + where);
    }
}

jclass findGlobalClass(JNIEnv* env, const char* name) {
    if (env == nullptr) return nullptr;
    jclass local = env->FindClass(name);
    if (local == nullptr) {
        env->ExceptionClear();
        return nullptr;
    }
    jclass global = static_cast<jclass>(env->NewGlobalRef(local));
    env->DeleteLocalRef(local);
    return global;
}

void copyToFloatArray(JNIEnv* env, jfloatArray target,
                      const float* source, std::size_t count) {
    if (env == nullptr || target == nullptr) {
        throwRuntimeError(env, "target float[] is null");
        return;
    }
    if (static_cast<jsize>(env->GetArrayLength(target)) < static_cast<jsize>(count)) {
        throwRuntimeError(env, "target float[] too small for frame");
        return;
    }
    env->SetFloatArrayRegion(target, 0, static_cast<jsize>(count), source);
}

}  // namespace bkh::jni