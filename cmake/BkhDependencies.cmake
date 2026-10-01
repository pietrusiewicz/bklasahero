# Zależności zewnętrzne rdzenia — piny wersji + sumy kontrolne.
#
# Dwie drogi (żeby build działał też bez sieci, np. na serwerze F-Droid z pustym cache):
#   1. Domyślnie: FetchContent pobiera archiwum z GitHub i weryfikuje SHA256.
#   2. -DBKH_DEPS_DIR=/ścieżka: oczekuje podkatalogów googletest/ i json/
#      (rozpakowane źródła). Pobierzesz je skryptem scripts/fetch-deps.sh.
#
# UWAGA (F-Droid): obie zależności mają licencje swobodne i NIE trafiają do APK
# w postaci prekompilowanej — są budowane ze źródeł w trakcie buildu.
#   * GoogleTest  — BSD-3-Clause (tylko testy hosta, nigdy nie linkowany do aplikacji)
#   * nlohmann/json — MIT (linkowany do rdzenia aplikacji)
#
# SPDX-License-Identifier: GPL-3.0-or-later

include_guard(GLOBAL)
include(FetchContent)

# DOWNLOAD_EXTRACT_TIMESTAMP (CMP0135) wymaga CMake >= 3.24. Android SDK domyślnie
# udostępnia CMake 3.22.1 (minimum dla NDK r27), więc opcję dodajemy warunkowo.
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    set(BKH_FETCH_TIMESTAMP DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
else()
    set(BKH_FETCH_TIMESTAMP)
endif()

set(BKH_GTEST_VERSION  "1.18.0" CACHE STRING "Wersja GoogleTest")
set(BKH_GTEST_URL      "https://github.com/google/googletest/archive/refs/tags/v${BKH_GTEST_VERSION}.tar.gz")
set(BKH_GTEST_SHA256   "6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5")

set(BKH_JSON_VERSION   "3.12.0" CACHE STRING "Wersja nlohmann/json")
set(BKH_JSON_URL       "https://github.com/nlohmann/json/archive/refs/tags/v${BKH_JSON_VERSION}.tar.gz")
set(BKH_JSON_SHA256    "4b92eb0c06d10683f7447ce9406cb97cd4b453be18d7279320f7b2f025c10187")

# ---------------------------------------------------------------------------
# nlohmann/json — potrzebny zawsze (serializacja save'ów i protokół mostka JNI)
# ---------------------------------------------------------------------------
if(TARGET nlohmann_json::nlohmann_json)
    # Już dodany (np. przez nadrzędny projekt Gradle/NDK).
elseif(BKH_DEPS_DIR AND EXISTS "${BKH_DEPS_DIR}/json/CMakeLists.txt")
    message(STATUS "nlohmann/json: używam lokalnego katalogu ${BKH_DEPS_DIR}/json")
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    add_subdirectory("${BKH_DEPS_DIR}/json" "${CMAKE_BINARY_DIR}/_deps/json-build" EXCLUDE_FROM_ALL)
else()
    message(STATUS "nlohmann/json ${BKH_JSON_VERSION}: pobieram (FetchContent, weryfikacja SHA256)")
    FetchContent_Declare(json
            URL "${BKH_JSON_URL}"
            URL_HASH "SHA256=${BKH_JSON_SHA256}"
            ${BKH_FETCH_TIMESTAMP})
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    set(JSON_Install OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(json)
endif()

# ---------------------------------------------------------------------------
# GoogleTest — tylko testy hosta
# ---------------------------------------------------------------------------
function(bkh_add_googletest)
    if(TARGET GTest::gtest_main)
        return()
    endif()

    if(BKH_DEPS_DIR AND EXISTS "${BKH_DEPS_DIR}/googletest/CMakeLists.txt")
        message(STATUS "GoogleTest: używam lokalnego katalogu ${BKH_DEPS_DIR}/googletest")
        set(BUILD_GMOCK OFF CACHE INTERNAL "")
        set(INSTALL_GTEST OFF CACHE INTERNAL "")
        add_subdirectory("${BKH_DEPS_DIR}/googletest"
                         "${CMAKE_BINARY_DIR}/_deps/googletest-build" EXCLUDE_FROM_ALL)
        return()
    endif()

    message(STATUS "GoogleTest ${BKH_GTEST_VERSION}: pobieram (FetchContent, weryfikacja SHA256)")
    FetchContent_Declare(googletest
            URL "${BKH_GTEST_URL}"
            URL_HASH "SHA256=${BKH_GTEST_SHA256}"
            ${BKH_FETCH_TIMESTAMP})
    set(BUILD_GMOCK OFF CACHE INTERNAL "")
    set(INSTALL_GTEST OFF CACHE INTERNAL "")
    set(gtest_force_shared_crt ON CACHE INTERNAL "")
    FetchContent_MakeAvailable(googletest)
endfunction()
