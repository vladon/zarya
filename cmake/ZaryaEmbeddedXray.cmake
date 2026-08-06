option(ZARYA_BUILD_EMBEDDED_XRAY "Build the vendored in-process Xray bridge" ON)
set(ZARYA_XRAY_GO_EXECUTABLE "" CACHE FILEPATH "Path to the pinned Go 1.26 executable")

function(zarya_configure_embedded_xray target)
    if(NOT ZARYA_BUILD_EMBEDDED_XRAY)
        target_compile_definitions(${target} PRIVATE ZARYA_EMBEDDED_XRAY_DISABLED=1)
        return()
    endif()

    # A blank cache entry must not suppress discovery. This occurs when a
    # developer first configures without an explicit Go path.
    set(_zarya_xray_go "${ZARYA_XRAY_GO_EXECUTABLE}")
    if(NOT _zarya_xray_go)
        unset(_zarya_xray_go)
        find_program(_zarya_xray_go NAMES go)
    endif()
    if(NOT _zarya_xray_go)
        message(FATAL_ERROR
            "Embedded Xray requires Go 1.26.x. Install Go or set ZARYA_XRAY_GO_EXECUTABLE.")
    endif()
    set(ZARYA_XRAY_GO_EXECUTABLE "${_zarya_xray_go}" CACHE FILEPATH
        "Path to the pinned Go 1.26 executable" FORCE)
    execute_process(
        COMMAND "${ZARYA_XRAY_GO_EXECUTABLE}" env GOVERSION
        OUTPUT_VARIABLE _zarya_go_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    if(NOT _zarya_go_version MATCHES "^go1\\.26(\\.|$)")
        message(FATAL_ERROR "Embedded Xray requires Go 1.26.x; found ${_zarya_go_version}")
    endif()

    if(WIN32)
        file(GLOB _zarya_vs_clang_candidates
            "${CMAKE_GENERATOR_INSTANCE}/VC/Tools/Llvm/x64/bin/clang.exe"
            "$ENV{VSINSTALLDIR}/VC/Tools/Llvm/x64/bin/clang.exe"
            "$ENV{ProgramFiles}/Microsoft Visual Studio/*/*/VC/Tools/Llvm/x64/bin/clang.exe")
        if(NOT _zarya_vs_clang_candidates)
            message(FATAL_ERROR
                "Embedded Xray requires Visual Studio LLVM clang.exe. Install the Visual Studio C++ Clang tools component.")
        endif()
        list(GET _zarya_vs_clang_candidates 0 _zarya_xray_clang)
    endif()

    set(_zarya_bridge_source "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/embedded/xray/bridge")
    set(_zarya_bridge_output_dir "${CMAKE_CURRENT_BINARY_DIR}/embedded-xray")
    if(WIN32)
        set(_zarya_bridge_file "zarya-xray.dll")
        set(_zarya_build_mode "c-shared")
    elseif(APPLE)
        set(_zarya_bridge_file "libzarya-xray.a")
        set(_zarya_build_mode "c-archive")
    else()
        set(_zarya_bridge_file "libzarya-xray.so")
        set(_zarya_build_mode "c-shared")
    endif()
    set(_zarya_bridge_output "${_zarya_bridge_output_dir}/${_zarya_bridge_file}")

    set(_zarya_go_env ${CMAKE_COMMAND} -E env "CGO_ENABLED=1")
    if(WIN32)
        # Go's Windows CGO driver unconditionally adds -mthreads. Visual Studio
        # LLVM targets the MSVC CRT and rejects that MinGW-only flag, so use a
        # generated wrapper that removes only that argument before invoking the
        # Visual Studio clang executable.
        set(_zarya_xray_cgo_wrapper "${CMAKE_CURRENT_BINARY_DIR}/${target}-cgo-clang-msvc.cmd")
        file(GENERATE OUTPUT "${_zarya_xray_cgo_wrapper}" CONTENT
            "@echo off\nsetlocal EnableExtensions\nset \"args=%*\"\nset \"args=%args:-mthreads=%\"\n\"${_zarya_xray_clang}\" %args%\n")
        list(APPEND _zarya_go_env "CC=${_zarya_xray_cgo_wrapper}")
    endif()

    file(GLOB_RECURSE _zarya_bridge_inputs CONFIGURE_DEPENDS
        "${_zarya_bridge_source}/*.go"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/xray-core/*.go"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/xray-core/go.mod"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/xray-core/go.sum")
    add_custom_command(
        OUTPUT "${_zarya_bridge_output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_zarya_bridge_output_dir}"
        COMMAND ${_zarya_go_env} "${ZARYA_XRAY_GO_EXECUTABLE}" mod verify
        COMMAND ${_zarya_go_env} "${ZARYA_XRAY_GO_EXECUTABLE}" build
            -trimpath -buildmode=${_zarya_build_mode} -o "${_zarya_bridge_output}" .
        WORKING_DIRECTORY "${_zarya_bridge_source}"
        DEPENDS ${_zarya_bridge_inputs}
        VERBATIM
        COMMENT "Building embedded Xray bridge with ${_zarya_go_version}")
    if(NOT TARGET zarya-xray-bridge)
        add_custom_target(zarya-xray-bridge DEPENDS "${_zarya_bridge_output}")
    endif()
    add_dependencies(${target} zarya-xray-bridge)
    target_include_directories(${target} PRIVATE "${_zarya_bridge_source}")
    target_compile_definitions(${target} PRIVATE
        ZARYA_EMBEDDED_XRAY_ABI_VERSION=1
        ZARYA_EMBEDDED_XRAY_LIBRARY_NAME="${_zarya_bridge_file}")

    if(APPLE)
        find_library(_zarya_corefoundation_framework CoreFoundation REQUIRED)
        find_library(_zarya_security_framework Security REQUIRED)
        find_library(_zarya_resolv_library resolv REQUIRED)
        target_compile_definitions(${target} PRIVATE ZARYA_EMBEDDED_XRAY_STATIC_ABI=1)
        target_link_libraries(${target} PRIVATE
            "${_zarya_bridge_output}"
            "${_zarya_corefoundation_framework}"
            "${_zarya_security_framework}"
            "${_zarya_resolv_library}")
    else()
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_zarya_bridge_output}" "$<TARGET_FILE_DIR:${target}>/${_zarya_bridge_file}"
            COMMENT "Copy embedded Xray bridge next to ${target}")
    endif()
endfunction()
