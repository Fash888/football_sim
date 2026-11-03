cmake_minimum_required(VERSION 3.16)
project(FootballSim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find SFML (supports SFML 2.x and SFML 3.x)
find_package(SFML COMPONENTS Graphics Window System CONFIG)
if (NOT SFML_FOUND)
    find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)
endif()

# Include directories
include_directories(include)

# Core Simulation Sources
set(CORE_SOURCES
    src/game/MatchEngine.cpp
    src/ai/Tactics.cpp
    src/data/Teams.cpp
    src/data/RealMatchData.cpp
    src/data/EmpiricalTrainedModel.cpp
    src/data/EventRecorder.cpp
    src/analytics/MatchAnalytics.cpp
    src/analytics/SpatialAnalytics.cpp
    src/tracking/TrackingSystem.cpp
)

# Main Application Executable
add_executable(FootballSim
    src/main.cpp
    src/render/PitchRenderer.cpp
    ${CORE_SOURCES}
)

if (TARGET SFML::Graphics)
    target_link_libraries(FootballSim PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
    )
else()
    target_link_libraries(FootballSim PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
    )
endif()

# Automated Test Suite Executable
add_executable(FootballSim_Tests
    tests/test_runner.cpp
    ${CORE_SOURCES}
)

# Test runner uses SFML System/Window if needed by headers
if (TARGET SFML::System)
    target_link_libraries(FootballSim_Tests PRIVATE SFML::System)
else()
    target_link_libraries(FootballSim_Tests PRIVATE sfml-system)
endif()
