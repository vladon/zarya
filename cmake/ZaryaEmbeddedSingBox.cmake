option(ZARYA_BUILD_EMBEDDED_SINGBOX "Build the vendored in-process SingBox bridge" ON)
set(ZARYA_SINGBOX_GO_EXECUTABLE "" CACHE FILEPATH "Path to the pinned Go 1.26 executable")

function(zarya_configure_embedded_singbox target)
    if(NOT ZARYA_BUILD_EMBEDDED_SINGBOX)
        target_compile_definitions(${target} PRIVATE ZARYA_EMBEDDED_SINGBOX_DISABLED=1)
        return()
    endif()

    # A blank cache entry must not suppress discovery. This occurs when a
    # developer first configures without an explicit Go path.
    set(_zarya_singbox_go "${ZARYA_SINGBOX_GO_EXECUTABLE}")
    if(NOT _zarya_singbox_go)
        unset(_zarya_singbox_go)
        find_program(_zarya_singbox_go NAMES go)
    endif()
    if(NOT _zarya_singbox_go)
        message(FATAL_ERROR
            "Embedded SingBox requires Go 1.26.x. Install Go or set ZARYA_SINGBOX_GO_EXECUTABLE.")
    endif()
    set(ZARYA_SINGBOX_GO_EXECUTABLE "${_zarya_singbox_go}" CACHE FILEPATH
        "Path to the pinned Go 1.26 executable" FORCE)
    execute_process(
        COMMAND "${ZARYA_SINGBOX_GO_EXECUTABLE}" env GOVERSION
        OUTPUT_VARIABLE _zarya_go_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    if(NOT _zarya_go_version MATCHES "^go1\\.26(\\.|$)")
        message(FATAL_ERROR "Embedded SingBox requires Go 1.26.x; found ${_zarya_go_version}")
    endif()

    if(WIN32)
        set(_zarya_singbox_cgo "${ZARYA_SINGBOX_CGO_COMPILER}")
        if(NOT _zarya_singbox_cgo AND ZARYA_XRAY_CGO_COMPILER)
            set(_zarya_singbox_cgo "${ZARYA_XRAY_CGO_COMPILER}")
        endif()
        if(NOT _zarya_singbox_cgo)
            find_program(_zarya_singbox_cgo NAMES x86_64-w64-mingw32-gcc gcc
                HINTS "${CMAKE_BINARY_DIR}/tools/winlibs/mingw64/bin" "C:/mingw64/bin" "C:/ProgramData/chocolatey/lib/mingw/tools/install/mingw64/bin" "$ENV{MINGW_ROOT}/bin")
        endif()
        if(NOT _zarya_singbox_cgo)
            message(FATAL_ERROR
                "Embedded SingBox requires a MinGW-w64 GCC CGO compiler. This is used only for the Go bridge; all C++ targets remain Visual Studio/MSVC.")
        endif()
        set(ZARYA_SINGBOX_CGO_COMPILER "${_zarya_singbox_cgo}" CACHE FILEPATH
            "MinGW-w64 GCC compiler used only to build the embedded SingBox Go bridge" FORCE)
    endif()

    set(_zarya_bridge_source "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/embedded/singbox/bridge")
    set(_zarya_bridge_output_dir "${CMAKE_CURRENT_BINARY_DIR}/embedded-singbox")
    if(WIN32)
        set(_zarya_bridge_file "zarya-singbox.dll")
        set(_zarya_build_mode "c-shared")
    elseif(APPLE)
        set(_zarya_bridge_file "libzarya-singbox.a")
        set(_zarya_build_mode "c-archive")
    else()
        set(_zarya_bridge_file "libzarya-singbox.so")
        set(_zarya_build_mode "c-shared")
    endif()
    set(_zarya_bridge_output "${_zarya_bridge_output_dir}/${_zarya_bridge_file}")

    set(_zarya_go_env ${CMAKE_COMMAND} -E env "CGO_ENABLED=1")
    if(WIN32)
        list(APPEND _zarya_go_env "CC=${ZARYA_SINGBOX_CGO_COMPILER}")
    endif()

    file(GLOB_RECURSE _zarya_bridge_inputs CONFIGURE_DEPENDS
        "${_zarya_bridge_source}/*.go"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sing-box/*.go"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sing-box/go.mod"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sing-box/go.sum")
    if(NOT TARGET zarya-singbox-bridge)
        add_custom_command(
            OUTPUT "${_zarya_bridge_output}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_zarya_bridge_output_dir}"
            COMMAND ${_zarya_go_env} "${ZARYA_SINGBOX_GO_EXECUTABLE}" mod verify
            COMMAND ${_zarya_go_env} "${ZARYA_SINGBOX_GO_EXECUTABLE}" build
                -trimpath -buildmode=${_zarya_build_mode} -o "${_zarya_bridge_output}" .
            WORKING_DIRECTORY "${_zarya_bridge_source}"
            DEPENDS ${_zarya_bridge_inputs}
            VERBATIM
            COMMENT "Building embedded SingBox bridge with ${_zarya_go_version}")
        add_custom_target(zarya-singbox-bridge DEPENDS "${_zarya_bridge_output}")
    endif()
    add_dependencies(${target} zarya-singbox-bridge)
    target_include_directories(${target} PRIVATE "${_zarya_bridge_source}")
    target_compile_definitions(${target} PRIVATE
        ZARYA_EMBEDDED_SINGBOX_ABI_VERSION=1
        ZARYA_EMBEDDED_SINGBOX_LIBRARY_NAME="${_zarya_bridge_file}")

    if(APPLE)
        find_library(_zarya_corefoundation_framework CoreFoundation REQUIRED)
        find_library(_zarya_security_framework Security REQUIRED)
        find_library(_zarya_resolv_library resolv REQUIRED)
        target_compile_definitions(${target} PRIVATE ZARYA_EMBEDDED_SINGBOX_STATIC_ABI=1)
        target_link_libraries(${target} PRIVATE
            "${_zarya_bridge_output}"
            "${_zarya_corefoundation_framework}"
            "${_zarya_security_framework}"
            "${_zarya_resolv_library}")
    else()
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_zarya_bridge_output}" "$<TARGET_FILE_DIR:${target}>/${_zarya_bridge_file}"
            COMMENT "Copy embedded SingBox bridge next to ${target}")
    endif()
endfunction()
