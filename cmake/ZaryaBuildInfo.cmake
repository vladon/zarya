find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE ZARYA_BUILD_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
if(NOT ZARYA_BUILD_COMMIT)
    set(ZARYA_BUILD_COMMIT "unknown")
endif()

string(TIMESTAMP ZARYA_BUILD_DATE_UTC "%Y-%m-%dT%H:%M:%SZ" UTC)

set(ZARYA_GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${ZARYA_GENERATED_DIR})

configure_file(
    ${CMAKE_SOURCE_DIR}/src/app/BuildInfo_config.h.in
    ${ZARYA_GENERATED_DIR}/BuildInfo_config.h
    @ONLY
)
