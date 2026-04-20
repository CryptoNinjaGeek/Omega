cmake_minimum_required(VERSION 3.21)

# Unit tests for the Omega engine. Gated on the top-level OMEGA_BUILD_TESTS
# option; dependencies (GoogleTest, enable_testing) are set up there.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# One executable for all engine unit tests so developers can run the whole
# suite via `ctest` or a single binary (`./oEngineTests`). When the suite
# grows we can split it per subsystem.
add_executable(oEngineTests
        PortalTransformTest.cpp
        PortalCameraTest.cpp
        FrustumTest.cpp
        LogTest.cpp
)

target_link_libraries(oEngineTests
        PRIVATE
        oEngine
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(oEngineTests
        PRIVATE
        ${CMAKE_SOURCE_DIR}/Engine/include
)

# Discover individual TEST() cases so ctest shows each one separately and
# failures are easy to pinpoint.
gtest_discover_tests(oEngineTests
        PROPERTIES
        LABELS "engine;unit"
)
