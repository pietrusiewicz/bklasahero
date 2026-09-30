// Mostek JNI — pomocnicze RAII do obsługi JNIEnv bez wycieków referencji.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <jni.h>

#include <cstddef>
#include <string>
#include <string_view>

#include "bkh/facade.h"

namespace bkh::jni {

// Globalny singleton Facade — jeden rdzeń na proces JVM.
Facade& facade();

// Podnosi java.lang.RuntimeException z komunikatem. Bezpiecznie wywoływać
// wielokrotnie — pomija, gdy wyjątek już jest w toku.
void throwRuntimeError(JNIEnv* env, std::string_view message);

// Zwraca ciąg UTF-8 z java.lang.String, albo pusty string dla nullptr/exception.
std::string toUtf8(JNIEnv* env, jstring jstr);

// Tworzy java.lang.String z bufora UTF-8 (zwraca nullptr przy błędzie alokacji).
jstring fromUtf8(JNIEnv* env, std::string_view text);

// Sprawdza, czy po wywołaniu metody JNI nie pozostał wyjątek do odkluczenia
// i jeśli tak, podnosi wyjątek po stronie C++ (rzuca JNIException).
void checkException(JNIEnv* env, const char* where);

// Zwraca globalną referencję do klasy, tworząc ją w razie potrzeby
// (przydatne przy wielokrotnych wywołaniach z wątku renderowania).
jclass findGlobalClass(JNIEnv* env, const char* name);

// Pomocnik: kopiuje dane z wewnętrznego std::vector do JNI floatArray
// z pełną obsługą błędów alokacji po stronie JVM.
void copyToFloatArray(JNIEnv* env, jfloatArray target,
                      const float* source, std::size_t count);

}  // namespace bkh::jni