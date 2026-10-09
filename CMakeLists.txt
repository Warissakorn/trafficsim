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
    target_compile_options(trafficsim_options INTERFACE -Wall -Wextra -Wpedantic -fno-fast-math -ffp-contract=off
        # Aggregates are built with trailing members left to their defaults on purpose; the
        # warning for that was 1120 of 1151 lines and hid the ones that mean something.
        -Wno-missing-field-initializers)
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
    src/core/signals.cpp src/core/simulation.cpp src/core/validate.cpp src/core/conflicts.cpp
    src/core/lanes.cpp src/core/routing.cpp)
target_link_libraries(trafficsim_core PRIVATE trafficsim_options)

add_library(trafficsim_model STATIC src/model/network/geometry.cpp
    src/model/network/connector_equation.cpp src/model/network/compile.cpp src/model/network/validate.cpp src/model/network/connector_geometry.cpp
    src/model/network/diagnostics.cpp src/model/network/connector_paths.cpp src/model/network/road_boundaries.cpp src/model/network/connector_surface.cpp
    src/model/network/sections.cpp src/model/network/markings.cpp src/model/network/rotation.cpp src/model/network/head_slots.cpp src/model/demand/signal_control.cpp
    src/model/network/routing.cpp src/model/network/lane_family.cpp src/model/network/routeless.cpp src/model/network/right_of_way.cpp
    src/model/network/conflict_surface.cpp src/model/network/conflict_groups.cpp src/model/network/conflict_coverage.cpp src/model/network/conflict_crossings.cpp src/model/network/conflict_polygons.cpp
    src/model/network/conflict_polygon_math.cpp
    src/model/network/conflict_bands.cpp
    src/model/network/conflict_mouth_bands.cpp src/model/network/automatic_conflicts.cpp)
target_link_libraries(trafficsim_model PUBLIC trafficsim_core PRIVATE trafficsim_options)
# Run-view geometry is Qt-free and independently testable; the core never imports it.
add_library(trafficsim_vehicle_pose STATIC src/editor/vehicle_pose.cpp src/editor/rear_axle_pose.cpp src/editor/lane_change_pose.cpp)
target_link_libraries(trafficsim_vehicle_pose PUBLIC trafficsim_model PRIVATE trafficsim_options)
add_library(trafficsim_eval STATIC src/eval/discharge.cpp src/eval/discharge_passage.cpp src/eval/summary.cpp src/eval/movement.cpp src/eval/lane_changes.cpp src/eval/segment_times.cpp src/eval/stop_lines.cpp src/eval/arrival_phases.cpp src/eval/dead_end_waits.cpp)
target_link_libraries(trafficsim_eval PUBLIC trafficsim_core PRIVATE trafficsim_options)
# M5.2 batches: independent seeds and their aggregate. Core and eval only (check_architecture).
add_library(trafficsim_runner STATIC src/runner/batch.cpp)
target_link_libraries(trafficsim_runner PUBLIC trafficsim_eval PRIVATE trafficsim_options)
add_library(trafficsim_project STATIC src/project/load.cpp src/project/parse.cpp src/project/output.cpp
    src/project/document.cpp src/project/signal_migration.cpp src/project/definition.cpp src/project/run.cpp src/project/display.cpp src/project/diagnostics.cpp
    src/project/input_sha256.cpp src/project/input_manifest.cpp src/project/behaviour_library.cpp src/project/discharge_json.cpp src/project/evaluation.cpp src/project/demand_paths.cpp src/project/demand_preview.cpp src/project/demand_catalog.cpp src/project/demand_time_types.cpp src/project/station_routing.cpp src/project/batch_output.cpp)
target_link_libraries(trafficsim_project PUBLIC trafficsim_model trafficsim_eval trafficsim_runner
    nlohmann_json::nlohmann_json PRIVATE trafficsim_options)

# nlohmann/json is a large header-only template library: 1.47 s to parse and instantiate here,
# against 0.36 s for <nlohmann/json_fwd.hpp>. src/project/json.hpp includes the forward
# declaration, so only a translation unit that builds or reads a Json pays for the definition,
# and precompiling it on the multi-source targets turns that into one cost per target. A
# single-source target gets no PCH of its own -- it would cost about what it saves -- but it can
# REUSE_FROM one that already exists, which src/shell does for the UI test executables.
target_precompile_headers(trafficsim_project PRIVATE <nlohmann/json.hpp>)

add_library(trafficsim_commands STATIC src/commands/history.cpp src/commands/network_commands.cpp
    src/commands/split_link.cpp src/commands/connector_commands.cpp src/commands/delete_objects.cpp
    src/commands/demand_commands.cpp src/commands/catalog_commands.cpp src/commands/behaviour_commands.cpp src/commands/appearance_commands.cpp src/commands/link_geometry_commands.cpp
    src/commands/right_of_way_commands.cpp src/commands/conflict_authoring.cpp
    src/commands/conflict_follow.cpp src/commands/conflict_group_commands.cpp)
target_link_libraries(trafficsim_commands PUBLIC trafficsim_project PRIVATE trafficsim_options)
target_precompile_headers(trafficsim_commands PRIVATE <nlohmann/json.hpp>)

function(trafficsim_runtime_data target)
    add_dependencies(${target} trafficsim-data)
endfunction()
# Recopy on every build, so catalog edits work without touching C++ or relinking.
add_custom_target(trafficsim-data ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/data" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<BOOL:${CMAKE_CONFIGURATION_TYPES}>,$<CONFIG>/,>data")

# The commit the CLI reports with batch output; regenerated on every build, rewritten only on change.
add_custom_target(trafficsim-build-info
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${PROJECT_SOURCE_DIR} -DOUTPUT=${CMAKE_BINARY_DIR}/generated/build_info.hpp
            -P ${PROJECT_SOURCE_DIR}/cmake/build_info.cmake
    BYPRODUCTS ${CMAKE_BINARY_DIR}/generated/build_info.hpp)
add_executable(trafficsim-cli tools/run_simulation.cpp)
target_link_libraries(trafficsim-cli PRIVATE trafficsim_project trafficsim_options)
target_include_directories(trafficsim-cli PRIVATE ${CMAKE_BINARY_DIR}/generated)
add_dependencies(trafficsim-cli trafficsim-build-info)
trafficsim_runtime_data(trafficsim-cli)
install(TARGETS trafficsim-cli RUNTIME DESTINATION bin)
install(DIRECTORY data/ DESTINATION bin/data)

# Not a test and not in `check`: it prints tick rates, it does not assert them. It needs no Qt,
# so the engine stays measurable under the headless preset.
add_executable(trafficsim-engine-benchmark tools/engine_benchmark.cpp)
target_link_libraries(trafficsim-engine-benchmark PRIVATE trafficsim_commands trafficsim_options)
target_precompile_headers(trafficsim-engine-benchmark PRIVATE <nlohmann/json.hpp>)
trafficsim_runtime_data(trafficsim-engine-benchmark)

# Regenerates the committed four-leg fixture from its one source, tools/four_leg_network.hpp.
add_executable(trafficsim-four-leg-fixture tools/four_leg_fixture.cpp)
target_link_libraries(trafficsim-four-leg-fixture PRIVATE trafficsim_commands trafficsim_options)
target_precompile_headers(trafficsim-four-leg-fixture PRIVATE <nlohmann/json.hpp>)
# Regenerates the M2.6 study template from its one source, tools/m26_study_network.hpp.
add_executable(trafficsim-m26-study tools/m26_study.cpp)
target_link_libraries(trafficsim-m26-study PRIVATE trafficsim_commands trafficsim_options)
target_precompile_headers(trafficsim-m26-study REUSE_FROM trafficsim-four-leg-fixture)
# Regenerates the M3.2.7 T-junction from its one source, tools/t_junction_network.hpp.
add_executable(trafficsim-t-junction-fixture tools/t_junction_fixture.cpp)
target_link_libraries(trafficsim-t-junction-fixture PRIVATE trafficsim_commands trafficsim_options)
target_precompile_headers(trafficsim-t-junction-fixture REUSE_FROM trafficsim-four-leg-fixture)
# The M3.2.7b diagnostic sweep: archived evidence, not a test, so not part of `check`.
add_executable(trafficsim-t-junction-sweep tools/t_junction_sweep.cpp)
target_link_libraries(trafficsim-t-junction-sweep PRIVATE trafficsim_commands trafficsim_options)
target_precompile_headers(trafficsim-t-junction-sweep REUSE_FROM trafficsim-four-leg-fixture)
# D105: clamp traces and checked 40-seed evidence, not part of check or an M5 batch API.
add_executable(trafficsim-t-junction-clamps tools/t_junction_clamps.cpp)
target_link_libraries(trafficsim-t-junction-clamps PRIVATE trafficsim_commands trafficsim_options)
target_precompile_headers(trafficsim-t-junction-clamps REUSE_FROM trafficsim-four-leg-fixture)
# D98: the lane-change lab, regenerated from tools/lane_change_network.hpp, and its sweep
# (development evidence, not part of `check`).
add_executable(trafficsim-lane-change-fixture tools/lane_change_fixture.cpp)
target_link_libraries(trafficsim-lane-change-fixture PRIVATE trafficsim_commands trafficsim_options)
target_precompile_headers(trafficsim-lane-change-fixture REUSE_FROM trafficsim-four-leg-fixture)
add_executable(trafficsim-lane-change-sweep tools/lane_change_sweep.cpp)
target_link_libraries(trafficsim-lane-change-sweep PRIVATE trafficsim_commands trafficsim_options)
target_precompile_headers(trafficsim-lane-change-sweep REUSE_FROM trafficsim-four-leg-fixture)

add_executable(trafficsim-check-architecture tools/check_architecture.cpp)
add_executable(trafficsim-check-file-sizes tools/check_file_sizes.cpp)
add_executable(trafficsim-check-docs tools/check_docs.cpp)
target_link_libraries(trafficsim-check-architecture PRIVATE trafficsim_options)
target_link_libraries(trafficsim-check-file-sizes PRIVATE trafficsim_options)
target_link_libraries(trafficsim-check-docs PRIVATE trafficsim_options)
add_custom_target(check
    COMMAND $<TARGET_FILE:trafficsim-check-architecture> "${PROJECT_SOURCE_DIR}"
    COMMAND $<TARGET_FILE:trafficsim-check-file-sizes> "${PROJECT_SOURCE_DIR}"
    COMMAND $<TARGET_FILE:trafficsim-check-docs> "${PROJECT_SOURCE_DIR}"
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
    DEPENDS trafficsim-cli trafficsim-check-architecture trafficsim-check-file-sizes trafficsim-check-docs
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

if(BUILD_TESTING)
    add_executable(trafficsim-lane-change-pose-tests tests/lane_change_pose_tests.cpp)
    target_link_libraries(trafficsim-lane-change-pose-tests PRIVATE trafficsim_vehicle_pose trafficsim_options)
    add_test(NAME lane-change-pose COMMAND trafficsim-lane-change-pose-tests)
    add_dependencies(check trafficsim-lane-change-pose-tests)
    add_executable(trafficsim-rear-axle-pose-tests tests/rear_axle_pose_tests.cpp)
    target_link_libraries(trafficsim-rear-axle-pose-tests PRIVATE trafficsim_vehicle_pose trafficsim_options)
    add_test(NAME rear-axle-pose COMMAND trafficsim-rear-axle-pose-tests)
    add_dependencies(check trafficsim-rear-axle-pose-tests)
    add_executable(trafficsim-vehicle-pose-tests tests/vehicle_pose_tests.cpp)
    target_link_libraries(trafficsim-vehicle-pose-tests PRIVATE trafficsim_vehicle_pose trafficsim_options)
    add_test(NAME vehicle-pose COMMAND trafficsim-vehicle-pose-tests)
    add_dependencies(check trafficsim-vehicle-pose-tests)
    add_executable(trafficsim-tests tests/main.cpp tests/behaviour_library_tests.cpp tests/behaviour_selection_tests.cpp tests/behaviour_selection_edge_tests.cpp tests/input_manifest_tests.cpp tests/discharge_tests.cpp tests/discharge_passage_tests.cpp tests/discharge_identity_tests.cpp tests/core_tests.cpp tests/station_routing_tests.cpp tests/source_insertion_tests.cpp
        tests/network_tests.cpp tests/reference_tests.cpp tests/project_tests.cpp tests/editor_model_tests.cpp tests/authoring_tests.cpp
        tests/network_lifecycle_tests.cpp tests/connector_tests.cpp tests/connector_shape_tests.cpp tests/connector_mouth_tests.cpp tests/connector_point_tests.cpp tests/attachment_tests.cpp tests/lane_edge_tests.cpp tests/diagnostics_tests.cpp tests/demand_tests.cpp tests/demand_correctness_tests.cpp tests/demand_catalog_tests.cpp tests/demand_time_types_tests.cpp tests/range_tests.cpp tests/four_leg_tests.cpp tests/m26_study_tests.cpp tests/right_of_way_tests.cpp tests/right_of_way_lifecycle_tests.cpp tests/right_of_way_resolution_tests.cpp tests/conflict_zone_tests.cpp tests/conflict_chain_tests.cpp tests/commitment_tests.cpp tests/lane_change_tests.cpp tests/stop_control_tests.cpp tests/stop_control_model_tests.cpp tests/signal_position_tests.cpp tests/queue_counter_tests.cpp tests/t_junction_tests.cpp tests/t_junction_controlled_tests.cpp tests/t_junction_signal_tests.cpp tests/automatic_conflict_tests.cpp tests/conflict_geometry_tests.cpp tests/conflict_polygon_tests.cpp tests/conflict_mouth_group_tests.cpp tests/conflict_follow_tests.cpp tests/right_of_way_runtime_tests.cpp tests/right_of_way_editor_tests.cpp tests/demand_interval_tests.cpp tests/demand_composition_tests.cpp tests/demand_decision_tests.cpp tests/movement_tests.cpp tests/arrival_phase_tests.cpp tests/wait_cause_tests.cpp tests/routeless_tests.cpp tests/downstream_decision_tests.cpp tests/decision_interval_tests.cpp tests/signal_control_tests.cpp tests/discretionary_tests.cpp tests/lane_change_lab_tests.cpp tests/batch_tests.cpp)
    target_link_libraries(trafficsim-tests PRIVATE trafficsim_commands trafficsim_options)
    target_compile_definitions(trafficsim-tests PRIVATE TRAFFICSIM_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    target_precompile_headers(trafficsim-tests PRIVATE <nlohmann/json.hpp>)
    target_sources(trafficsim-tests PRIVATE tests/history_tests.cpp tests/rotation_tests.cpp tests/connector_surface_tests.cpp tests/lane_correspondence_tests.cpp tests/connector_mouth_sweep_tests.cpp tests/connector_lane_centre_tests.cpp tests/connector_equation_tests.cpp tests/connector_edit_motion_tests.cpp)
    # One ctest test per group, so `ctest -j` spreads them; a single unfiltered run was 60 s of
    # serial wall time and repeated every named group.
    set(TRAFFICSIM_TEST_GROUPS behaviourlibrary behaviourselection provenance discharge core stationrouting network reference project editor connectors mouths points
        diagnostics demand ranges attachments authoring lifecycle history rotation fourleg movement
        routeless signals m26study m26study_merge rightofway rightofway_lifecycle rightofway_resolution
        conflict_zone conflict_chain commitment stop_control stop_control_model signal_position
        queue_counter tjunction tjunction_controlled tjunction_signal automatic_conflict conflict_geometry
        rightofway_runtime rightofway_editor lanechange lane_correspondence mouth_sweep conflict_follow
        discretionary lanelab lane_centres equation batch)
    foreach(group ${TRAFFICSIM_TEST_GROUPS})
        add_test(NAME ${group} COMMAND trafficsim-tests ${group})
    endforeach()
    # A newly registered group must never silently disappear from CI just because the list
    # above was not updated (previously "points"): this fails on any group it does not name.
    add_test(NAME all-model-tests COMMAND trafficsim-tests --check-groups ${TRAFFICSIM_TEST_GROUPS})
    add_test(NAME architecture COMMAND trafficsim-check-architecture "${PROJECT_SOURCE_DIR}")
    add_test(NAME architecture-negative COMMAND trafficsim-check-architecture --self-test)
    add_test(NAME file-sizes COMMAND trafficsim-check-file-sizes "${PROJECT_SOURCE_DIR}")
    add_test(NAME documentation COMMAND trafficsim-check-docs "${PROJECT_SOURCE_DIR}")
    add_test(NAME documentation-negative COMMAND trafficsim-check-docs --self-test)
    add_test(NAME cli COMMAND trafficsim-cli 42)
    set_tests_properties(cli PROPERTIES PASS_REGULAR_EXPRESSION "29\\.24935")
    add_test(NAME cli-invalid-seed COMMAND trafficsim-cli 4294967296)
    set_tests_properties(cli-invalid-seed PROPERTIES WILL_FAIL TRUE)
    add_test(NAME cli-help COMMAND trafficsim-cli --help)
    add_test(NAME cli-project COMMAND trafficsim-cli 42 --project "${PROJECT_SOURCE_DIR}/data/projects/four-leg-signalised.traffic.json")
    set_tests_properties(cli-project PROPERTIES PASS_REGULAR_EXPRESSION "North approach .* West exit")
    add_test(NAME cli-batch COMMAND trafficsim-cli --seeds 42-44 --project "${PROJECT_SOURCE_DIR}/data/projects/four-leg-signalised.traffic.json")
    set_tests_properties(cli-batch PROPERTIES PASS_REGULAR_EXPRESSION "\"halfWidth95\": [0-9]")
    add_test(NAME cli-batch-conflict COMMAND trafficsim-cli --seed 1 --seeds 2-3 --project "${PROJECT_SOURCE_DIR}/data/projects/four-leg-signalised.traffic.json")
    set_tests_properties(cli-batch-conflict PROPERTIES WILL_FAIL TRUE)
    add_dependencies(check trafficsim-tests)
endif()

if(TRAFFICSIM_BUILD_DESKTOP)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)
    add_subdirectory(src/shell)
endif()
