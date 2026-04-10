cmake_minimum_required(VERSION 3.10)
project(CarlsGarage)

set(CMAKE_CXX_STANDARD 11)

include_directories(src)

add_library(logger src/logger/logger.c)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(test_logger tests/test_logger.cpp)

target_link_libraries(test_logger ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} logger)

# ---------------------------------------------------------------------------
# Allegro 5 integration (optional)
# ---------------------------------------------------------------------------
# Strategy: try CMake's built-in find_package first (works when Allegro
# installs a proper AllegroConfig.cmake / allegro-config.cmake).  If that
# fails, fall back to pkg-config, which is the common case on Linux distros
# that ship allegro5 via their package manager (apt, pacman, dnf, etc.).
# If neither mechanism finds Allegro, the main library and tests still build
# normally — Allegro targets are simply skipped.

find_package(Allegro 5 QUIET)

if(NOT Allegro_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(ALLEGRO QUIET allegro-5)
        if(ALLEGRO_FOUND)
            # Wrap the pkg-config result in an IMPORTED target so downstream
            # targets can use target_link_libraries() uniformly.
            add_library(allegro::allegro INTERFACE IMPORTED)
            target_include_directories(allegro::allegro
                INTERFACE ${ALLEGRO_INCLUDE_DIRS})
            target_link_libraries(allegro::allegro
                INTERFACE ${ALLEGRO_LIBRARIES})
            target_compile_options(allegro::allegro
                INTERFACE ${ALLEGRO_CFLAGS_OTHER})
            target_link_directories(allegro::allegro
                INTERFACE ${ALLEGRO_LIBRARY_DIRS})
            set(ALLEGRO_TARGET allegro::allegro)
        endif()
    endif()
elseif(TARGET allegro)
    set(ALLEGRO_TARGET allegro)
elseif(TARGET Allegro::allegro)
    set(ALLEGRO_TARGET Allegro::allegro)
endif()

if(ALLEGRO_TARGET)
    message(STATUS "Allegro 5 found — building allegro_demo target")
    add_executable(allegro_demo src/allegro_demo/main.c)
    target_link_libraries(allegro_demo PRIVATE logger ${ALLEGRO_TARGET})
else()
    message(STATUS "Allegro 5 not found — skipping allegro_demo target."
        " Install liballegro5-dev (Debian/Ubuntu) or allegro (Arch/Fedora) to enable it.")
endif()
