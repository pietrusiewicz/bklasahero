// Mostek JNI — współdzielony układ bufora klatek (mirror w Kotlin FrameBuffer.kt).
// Plik jest jedynym miejscem definiującym stałe protokołu renderowania.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>

#include "bkh/facade.h"

namespace bkh::jni {

// Te stałe MUSZĄ być zgodne z `frame::k*` z rdzenia. Trzymamy je osobno,
// żeby móc je obliczać bez włączania `bkh/facade.h` z JNI.
namespace layout {
inline constexpr int kHeaderFloats = 8;
inline constexpr int kBallFloatsPerSample = 7;
inline constexpr int kKeeperFloatsPerSample = 6;
}  // namespace layout

// Liczba floatów potrzebnych dla podanej liczby próbek piłki i bramkarza.
inline constexpr std::size_t requiredFloats(std::size_t ballSamples,
                                            std::size_t keeperSamples) {
    return static_cast<std::size_t>(layout::kHeaderFloats) +
           ballSamples * static_cast<std::size_t>(layout::kBallFloatsPerSample) +
           keeperSamples * static_cast<std::size_t>(layout::kKeeperFloatsPerSample);
}

}  // namespace bkh::jni