cmake_minimum_required(VERSION 3.20)
project(RoundCore LANGUAGES CXX)

add_library(RoundCore STATIC
    src/Game.cpp
    src/Observation.cpp
    src/VectorEnvironment.cpp
)

target_include_directories(RoundCore PUBLIC include)
target_compile_features(RoundCore PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(RoundCore PRIVATE /W4 /permissive-)
else()
    target_compile_options(RoundCore PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(ROUNDCORE_BUILD_TESTS "Build RoundCore standalone tests" ON)
option(ROUNDCORE_BUILD_PYTHON "Build the optional standalone Python module" OFF)

if(ROUNDCORE_BUILD_PYTHON)
    find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m pybind11 --cmakedir
        OUTPUT_VARIABLE ROUNDCORE_PYBIND11_CMAKE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    find_package(pybind11 CONFIG REQUIRED PATHS "${ROUNDCORE_PYBIND11_CMAKE_DIR}" NO_DEFAULT_PATH)
    pybind11_add_module(roundcore_python MODULE python/Bindings.cpp)
    target_link_libraries(roundcore_python PRIVATE RoundCore)
    add_custom_target(roundcore_python_helpers ALL
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/python/roundcore_training.py"
            "$<TARGET_FILE_DIR:roundcore_python>/roundcore_training.py"
        DEPENDS roundcore_python)
endif()

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

    add_executable(RoundCoreVectorEnvironmentTests tests/VectorEnvironmentTests.cpp)
    target_link_libraries(RoundCoreVectorEnvironmentTests PRIVATE RoundCore)
    add_test(NAME RoundCoreVectorEnvironmentTests COMMAND RoundCoreVectorEnvironmentTests)

    if(ROUNDCORE_BUILD_PYTHON)
        add_test(
            NAME RoundCorePythonSmoke
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/PythonSmoke.py"
                "$<TARGET_FILE_DIR:roundcore_python>")
    endif()
endif()
