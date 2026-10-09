# Run at build time (not configure time), so a rebuild after a commit never reports the old one.
# Writes OUTPUT only when the text changes, so an unchanged tree relinks nothing.
# Inputs: SOURCE_DIR, OUTPUT. The commit is provenance for batch output (M5.2), never an engine input.
set(commit "unknown")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD WORKING_DIRECTORY "${SOURCE_DIR}"
        OUTPUT_VARIABLE head OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE failed ERROR_QUIET)
    if(NOT failed AND head)
        set(commit "${head}")
        execute_process(COMMAND "${GIT_EXECUTABLE}" status --porcelain --untracked-files=no WORKING_DIRECTORY "${SOURCE_DIR}"
            OUTPUT_VARIABLE changes OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        if(changes)
            string(APPEND commit "-dirty")
        endif()
    endif()
endif()
set(text "#pragma once\n#define TRAFFICSIM_BUILD_COMMIT \"${commit}\"\n")
if(EXISTS "${OUTPUT}")
    file(READ "${OUTPUT}" previous)
endif()
if(NOT previous STREQUAL text)
    file(WRITE "${OUTPUT}" "${text}")
endif()
