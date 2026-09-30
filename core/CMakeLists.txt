# Rdzeń aplikacji: czysty C++23, ZERO zależności od Androida/JNI/platformy.
#
# Reguły nienaruszalne (zob. docs/ADR/0001-core-boundaries.md):
#   1. Żadnego #include <jni.h>, <android/...> ani API Androida w core/.
#   2. Rdzeń nie czyta z dysku ani z sieci — dane wejściowe (np. katalog
#      miejscowości z assets) dostarcza warstwa platformowa jako bufor bajtów.
#   3. Wszystko, co losowe, przechodzi przez bkh::Random z jawnym seedem
#      (determinizm i odtwarzalność testów).
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(bkh_core STATIC
        src/random.cpp
        src/physics.cpp
        src/keeper.cpp
        src/shooter.cpp
        src/shot.cpp
        src/shootout.cpp
        src/place.cpp
        src/club.cpp
        src/league.cpp
        src/career.cpp
        src/json_io.cpp
        src/facade.cpp)

add_library(bkh::core ALIAS bkh_core)

target_include_directories(bkh_core PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_compile_features(bkh_core PUBLIC cxx_std_23)

target_link_libraries(bkh_core PUBLIC nlohmann_json::nlohmann_json)

# Wersja rdzenia dostępna w kodzie (ekran „O aplikacji", logi crashy).
target_compile_definitions(bkh_core PUBLIC
        BKH_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        BKH_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        BKH_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        BKH_VERSION_STRING="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

set_target_properties(bkh_core PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)
