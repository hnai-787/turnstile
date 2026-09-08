cmake_minimum_required(VERSION 3.20)
project(schedsim CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(nlohmann_json CONFIG REQUIRED)

add_library(schedsim_core
    src/simulator.cpp
    src/metrics.cpp
    src/policies.cpp
    src/runner.cpp
    src/report.cpp
    src/validation.cpp
    src/workload_io.cpp
    src/workload_presets.cpp
)
target_include_directories(schedsim_core PUBLIC include)
target_link_libraries(schedsim_core PUBLIC nlohmann_json::nlohmann_json)
if(MINGW)
    target_link_options(schedsim_core PUBLIC -static)
endif()

add_executable(schedsim src/main.cpp)
target_link_libraries(schedsim PRIVATE schedsim_core)

option(SCHEDSIM_BUILD_TESTS "Build the schedsim unit test suite" ON)
if(SCHEDSIM_BUILD_TESTS)
    find_package(Catch2 3 CONFIG REQUIRED)
    add_executable(schedsim_tests
        tests/test_policies.cpp
        tests/test_simulator.cpp
        tests/test_invariants.cpp
        tests/test_metrics.cpp
    )
    target_link_libraries(schedsim_tests PRIVATE schedsim_core Catch2::Catch2WithMain)

    enable_testing()
    include(CTest)
    add_test(NAME schedsim_tests COMMAND schedsim_tests)
endif()
