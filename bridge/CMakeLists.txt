# Mostek JNI — JEDYNA warstwa projektu, która wie o istnieniu Androida/JNI.
#
# Zasady (zob. docs/ADR/0001-core-boundaries.md):
#   * zero logiki domenowej — tylko konwersja typów JNI <-> C++ i zarządzanie pamięcią,
#   * rdzeń (bkh_core) dołączany jako podkatalog, jeśli nie został jeszcze zdefiniowany,
#     dzięki czemu Gradle może wskazać externalNativeBuild bezpośrednio na ten plik,
#   * dwa kanały komunikacji z warstwą Kotlin:
#       - kontrolny: polecenia/odpowiedzi jako JSON (rzadkie, czytelne, łatwe w testach),
#       - renderujący: surowy bufor float dla klatek animacji (60 fps, bez alokacji).
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.22)

project(bkh_bridge LANGUAGES CXX)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# --- Dołączenie rdzenia i modułów cmake, gdy ten plik jest korzeniem buildu ---
if(NOT TARGET bkh_core)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
    include(BkhCompileOptions)
    bkh_apply_compile_options()
    include(BkhDependencies)
    # Testy hosta i sam bridge nie są tu potrzebne, gdy buduje nas Gradle.
    set(BKH_BUILD_TESTS OFF CACHE INTERNAL "")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../core" "${CMAKE_BINARY_DIR}/bkh_core")
endif()

add_library(bkh_bridge SHARED
        src/jni_facade.cpp
        src/jni_frame_buffer.cpp
        src/jni_util.cpp)

target_include_directories(bkh_bridge PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(bkh_bridge PRIVATE bkh::core)

if(ANDROID)
    find_library(log-lib log)
    find_library(android-lib android)
    target_link_libraries(bkh_bridge PRIVATE ${log-lib} ${android-lib})
    target_compile_definitions(bkh_bridge PRIVATE BKH_PLATFORM_ANDROID=1)
else()
    # Build mostka poza Androidem (np. test integracyjny na hoście) — wymaga jni.h z JDK.
    if(DEFINED ENV{JAVA_HOME})
        target_include_directories(bkh_bridge PRIVATE
                "$ENV{JAVA_HOME}/include"
                "$ENV{JAVA_HOME}/include/linux")
    endif()
    message(WARNING
            "bkh_bridge budowany poza Androidem — potrzebne jni.h (ustaw JAVA_HOME). "
            "Ten wariant służy tylko do sprawdzania kompilacji, nie do uruchamiania.")
endif()

set_target_properties(bkh_bridge PROPERTIES
        OUTPUT_NAME "bkh_bridge"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

# Eksportujemy wyłącznie symbole JNI (Java_...); reszta rdzenia pozostaje ukryta.
if(NOT ANDROID AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(bkh_bridge PRIVATE "-Wl,--exclude-libs,ALL")
endif()
