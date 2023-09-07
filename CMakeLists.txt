cmake_minimum_required(VERSION 3.10)
project(CarlsGarage)

set(CMAKE_CXX_STANDARD 11)

include_directories(src)

add_library(logger src/logger/logger.c)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(test_logger tests/test_logger.cpp)

target_link_libraries(test_logger ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} logger)
