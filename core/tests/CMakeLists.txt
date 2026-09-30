# Testy jednostkowe rdzenia — uruchamiane NA HOŚCIE (g++/clang, bez Androida).
# To jest najszybsza pętla developerska i jedyne realne testy w środowisku
# bez urządzenia i bez emulatora (brak /dev/kvm na maszynie dev).
#
# SPDX-License-Identifier: GPL-3.0-or-later

bkh_add_googletest()

# Katalog fixture'ów (np. miniaturowy katalog miejscowości) przekazywany do testów,
# żeby nie zależały od ścieżki względnej uruchomienia.
set(BKH_TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")

add_executable(bkh_core_tests
        test_main.cpp
        test_random.cpp
        test_physics.cpp
        test_keeper.cpp
        test_shooter.cpp
        test_shootout.cpp
        test_place.cpp
        test_club.cpp
        test_league.cpp
        test_career.cpp
        test_json_io.cpp
        test_facade.cpp
        test_simulation_balance.cpp)

target_link_libraries(bkh_core_tests PRIVATE
        bkh::core
        GTest::gtest
        GTest::gtest_main)

target_compile_definitions(bkh_core_tests PRIVATE
        BKH_TEST_DATA_DIR="${BKH_TEST_DATA_DIR}")

# Testy nie są biblioteką produktową — nie ukrywajmy symboli (łatwiejsze debugowanie).
set_target_properties(bkh_core_tests PROPERTIES CXX_VISIBILITY_PRESET default)

include(GoogleTest)
gtest_discover_tests(bkh_core_tests
        PROPERTIES TIMEOUT 60
        DISCOVERY_TIMEOUT 60)
