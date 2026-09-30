# Wspólne flagi kompilacji dla rdzenia i mostka JNI.
#
# Zasady (zob. docs/ADR/0002-build-flags.md):
#  * C++23, bez rozszerzeń GNU — kod musi budować się identycznie na GCC (host),
#    clang z NDK (urządzenie) i w CI (x86_64).
#  * Wyjątki i RTTI WŁĄCZONE (uproszczenie kodu; hot-path bez wyjątków).
#  * Brak -ffast-math: determinizm symulacji jest cechą produktu.
#  * 16 KB page size dla bibliotek współdzielonych (wymóg Androida 15+).
#
# SPDX-License-Identifier: GPL-3.0-or-later

include_guard(GLOBAL)

function(bkh_apply_compile_options)
    add_compile_options(-Wall -Wextra -Wpedantic
                        -Wshadow -Wnon-virtual-dtor -Wold-style-cast
                        -Wcast-qual -Wconversion -Wsign-conversion
                        -Wnull-dereference -Wdouble-promotion
                        -Wimplicit-fallthrough -Wuseless-cast
                        -Wformat=2)

    if(BKH_WERROR)
        add_compile_options(-Werror)
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fno-strict-aliasing)
        add_compile_options("$<$<CONFIG:Release>:-O2>")
        add_compile_options("$<$<CONFIG:Release>:-fno-plt>")
        add_compile_options("$<$<CONFIG:RelWithDebInfo>:-O2;-g>")
        # Widoczność symboli: rdzeń jest biblioteką — eksportujemy tylko to, co trzeba.
        add_compile_options("$<$<CONFIG:Release>:-fvisibility=hidden;-fvisibility-inlines-hidden>")
    endif()

    if(BKH_SANITIZERS)
        if(ANDROID)
            message(FATAL_ERROR "BKH_SANITIZERS jest przewidziane tylko dla buildu hosta.")
        endif()
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address,undefined)
    endif()

    # 16 KB alignment dla .so — wymagane przez Google Play i zalecane dla F-Droid
    # na urządzeniach z Androidem 15+. Dla bibliotek statycznych (host) to no-op.
    if(ANDROID)
        add_link_options("-Wl,-z,max-page-size=16384")
    endif()
endfunction()
