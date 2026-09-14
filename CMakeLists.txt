cmake_minimum_required(VERSION 3.24)
project(TrafficSim VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
option(TRAFFICSIM_BUILD_DESKTOP "Build the Qt Widgets desktop harness" ON)
option(TRAFFICSIM_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
include(CTest)

add_library(trafficsim_options INTERFACE)
target_compile_definitions(trafficsim_options INTERFACE
    TRAFFICSIM_VERSION="${PROJECT_VERSION}"
    TRAFFICSIM_COMPILER="${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}")
if(MSVC)
    target_compile_options(trafficsim_options INTERFACE /W4 /permissive- /utf-8 /fp:strict)
else()
    target_compile_options(trafficsim_options INTERFACE -Wall -Wextra -Wpedantic -fno-fast-math -ffp-contract=off)
    if(TRAFFICSIM_SANITIZERS)
        target_compile_options(trafficsim_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(trafficsim_options INTERFACE -fsanitize=address,undefined)
    endif()
endif()

# System/vcpkg packages; no network downloads happen implicitly during configure.
find_package(nlohmann_json 3.11 CONFIG QUIET)
if(NOT TARGET nlohmann_json::nlohmann_json)
    find_path(TRAFFICSIM_JSON_INCLUDE_DIR nlohmann/json.hpp REQUIRED)
    add_library(trafficsim_json INTERFACE)
    target_include_directories(trafficsim_json SYSTEM INTERFACE "${TRAFFICSIM_JSON_INCLUDE_DIR}")
    add_library(nlohmann_json::nlohmann_json ALIAS trafficsim_json)
endif()

add_library(trafficsim_core STATIC
    src/core/demand.cpp src/core/following.cpp src/core/routes.cpp
    src/core/signals.cpp src/core/simulation.cpp src/core/validate.cpp)
target_link_libraries(trafficsim_core PRIVATE trafficsim_options)

add_library(trafficsim_model STATIC src/model/network/geometry.cpp
    src/model/network/compile.cpp src/model/network/validate.cpp src/model/network/connector_geometry.cpp
    src/model/network/diagnostics.cpp)
target_link_libraries(trafficsim_model PUBLIC trafficsim_core PRIVATE trafficsim_options)
add_library(trafficsim_eval STATIC src/eval/summary.cpp)
target_link_libraries(trafficsim_eval PUBLIC trafficsim_core PRIVATE trafficsim_options)
add_library(trafficsim_project STATIC src/project/load.cpp src/project/parse.cpp src/project/output.cpp
    src/project/document.cpp src/project/diagnostics.cpp)
target_link_libraries(trafficsim_project PUBLIC trafficsim_model trafficsim_eval
    nlohmann_json::nlohmann_json PRIVATE trafficsim_options)

add_library(trafficsim_commands STATIC src/commands/history.cpp src/commands/network_commands.cpp
    src/commands/split_link.cpp src/commands/connector_commands.cpp src/commands/delete_objects.cpp)
target_link_libraries(trafficsim_commands PUBLIC trafficsim_project PRIVATE trafficsim_options)

function(trafficsim_runtime_data target)
    add_dependencies(${target} trafficsim-data)
endfunction()
# Recopy on every build, so catalog edits work without touching C++ or relinking.
add_custom_target(trafficsim-data ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/data" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<BOOL:${CMAKE_CONFIGURATION_TYPES}>,$<CONFIG>/,>data")

add_executable(trafficsim-cli tools/run_simulation.cpp)
target_link_libraries(trafficsim-cli PRIVATE trafficsim_project trafficsim_options)
trafficsim_runtime_data(trafficsim-cli)
install(TARGETS trafficsim-cli RUNTIME DESTINATION bin)
install(DIRECTORY data/ DESTINATION bin/data)

add_executable(trafficsim-check-architecture tools/check_architecture.cpp)
add_executable(trafficsim-check-file-sizes tools/check_file_sizes.cpp)
target_link_libraries(trafficsim-check-architecture PRIVATE trafficsim_options)
target_link_libraries(trafficsim-check-file-sizes PRIVATE trafficsim_options)
add_custom_target(check
    COMMAND $<TARGET_FILE:trafficsim-check-architecture> "${PROJECT_SOURCE_DIR}"
    COMMAND $<TARGET_FILE:trafficsim-check-file-sizes> "${PROJECT_SOURCE_DIR}"
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
    DEPENDS trafficsim-cli trafficsim-check-architecture trafficsim-check-file-sizes
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

if(BUILD_TESTING)
    add_executable(trafficsim-tests tests/main.cpp tests/core_tests.cpp
        tests/network_tests.cpp tests/reference_tests.cpp tests/project_tests.cpp tests/editor_model_tests.cpp
        tests/connector_tests.cpp tests/diagnostics_tests.cpp)
    target_link_libraries(trafficsim-tests PRIVATE trafficsim_commands trafficsim_options)
    target_compile_definitions(trafficsim-tests PRIVATE TRAFFICSIM_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    foreach(group core network reference project editor connectors diagnostics)
        add_test(NAME ${group} COMMAND trafficsim-tests ${group})
    endforeach()
    add_test(NAME architecture COMMAND trafficsim-check-architecture "${PROJECT_SOURCE_DIR}")
    add_test(NAME architecture-negative COMMAND trafficsim-check-architecture --self-test)
    add_test(NAME file-sizes COMMAND trafficsim-check-file-sizes "${PROJECT_SOURCE_DIR}")
    add_test(NAME cli COMMAND trafficsim-cli 42)
    set_tests_properties(cli PROPERTIES PASS_REGULAR_EXPRESSION "29\\.24935")
    add_test(NAME cli-invalid-seed COMMAND trafficsim-cli 4294967296)
    set_tests_properties(cli-invalid-seed PROPERTIES WILL_FAIL TRUE)
    add_test(NAME cli-help COMMAND trafficsim-cli --help)
    add_dependencies(check trafficsim-tests)
endif()

if(TRAFFICSIM_BUILD_DESKTOP)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)
    add_subdirectory(src/shell)
endif()
