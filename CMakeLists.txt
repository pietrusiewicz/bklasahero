# B-Klasa Hero / Sunday League Hero — przenośny rdzeń C++23
#
# Ten plik jest punktem wejścia do budowy NA HOŚCIE (testy jednostkowe).
# Na Androidzie Gradle wywołuje bridge/CMakeLists.txt (zob. android/app/build.gradle.kts),
# który dołącza core/ jako podkatalog — logika budowy rdzenia jest więc wspólna.
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.24)

project(bkh
        VERSION 0.1.0
        DESCRIPTION "B-Klasa Hero — deterministic penalty-shootout and league core"
        LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Opcje
# ---------------------------------------------------------------------------
option(BKH_BUILD_TESTS  "Buduj testy jednostkowe rdzenia (GoogleTest, tylko host)" ON)
option(BKH_BUILD_BRIDGE "Buduj mostek JNI (wymaga nagłówków jni.h — Android/NDK)"   OFF)
option(BKH_WERROR       "Traktuj ostrzeżenia kompilatora jako błędy"               OFF)
option(BKH_SANITIZERS   "Włącz ASan+UBSan (tylko host, debug)"                     OFF)

set(BKH_DEPS_DIR "" CACHE PATH
    "Katalog z wcześniej pobranymi zależnościami (googletest/, json/); pusty = FetchContent")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ---------------------------------------------------------------------------
# Ustawienia globalne
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Testy muszą być stabilne niezależnie od tego, czy buduje host (aarch64),
# CI (x86_64) czy serwer F-Droid — dlatego nie używamy -ffast-math ani LTO w debug.
include(BkhCompileOptions)
bkh_apply_compile_options()

include(BkhDependencies)

# ---------------------------------------------------------------------------
# Komponenty
# ---------------------------------------------------------------------------
add_subdirectory(core)

if(BKH_BUILD_BRIDGE)
    add_subdirectory(bridge)
endif()

if(BKH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(core/tests)
endif()

# ---------------------------------------------------------------------------
# Podsumowanie konfiguracji
# ---------------------------------------------------------------------------
message(STATUS "-------------------------------------------------------")
message(STATUS " ${PROJECT_NAME} ${PROJECT_VERSION}  (${CMAKE_BUILD_TYPE})")
message(STATUS "   testy        : ${BKH_BUILD_TESTS}")
message(STATUS "   mostek JNI   : ${BKH_BUILD_BRIDGE}")
message(STATUS "   sanitizery   : ${BKH_SANITIZERS}")
message(STATUS "   kompilator   : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "-------------------------------------------------------")
