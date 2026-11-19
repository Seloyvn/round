cmake_minimum_required(VERSION 3.20)
project(RoundCore LANGUAGES CXX)

add_library(RoundCore STATIC
    src/Game.cpp
    src/Observation.cpp
)

target_include_directories(RoundCore PUBLIC include)
target_compile_features(RoundCore PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(RoundCore PRIVATE /W4 /permissive-)
else()
    target_compile_options(RoundCore PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(ROUNDCORE_BUILD_TESTS "Build RoundCore standalone tests" ON)
if(ROUNDCORE_BUILD_TESTS)
    enable_testing()
    add_executable(RoundCoreTests tests/GameTests.cpp)
    target_link_libraries(RoundCoreTests PRIVATE RoundCore)
    add_test(NAME RoundCoreTests COMMAND RoundCoreTests)

    add_executable(RoundCoreSelfPlaySmoke tests/SelfPlaySmoke.cpp)
    target_link_libraries(RoundCoreSelfPlaySmoke PRIVATE RoundCore)
    add_test(NAME RoundCoreSelfPlaySmoke COMMAND RoundCoreSelfPlaySmoke)

    add_executable(RoundCoreRandomAgentSmoke tests/RandomAgentSmoke.cpp)
    target_link_libraries(RoundCoreRandomAgentSmoke PRIVATE RoundCore)
    add_test(NAME RoundCoreRandomAgentSmoke COMMAND RoundCoreRandomAgentSmoke)
endif()
