# Optional Desktop App Toolkit (lib_ui) integration for Zarya.
# Enabled with -DZARYA_DESKTOP_APP_UI=ON. Produces a GPLv3+ derivative binary.

option(ZARYA_DESKTOP_APP_UI
    "Link desktop-app lib_ui (GPLv3+). Spike UI; OFF by default for CI."
    OFF)

if(NOT ZARYA_DESKTOP_APP_UI)
    return()
endif()

message(STATUS "ZARYA_DESKTOP_APP_UI=ON — linking Desktop App Toolkit (GPLv3+)")

# lz4 and other bundled deps ship .c sources.
enable_language(C)

set(DESKTOP_APP_SPECIAL_TARGET "" CACHE STRING "" FORCE)
set(DESKTOP_APP_USE_PACKAGED ON CACHE BOOL "" FORCE)
set(DESKTOP_APP_DISABLE_CRASH_REPORTS ON CACHE BOOL "" FORCE)
set(DESKTOP_APP_USE_PACKAGED_FONTS OFF CACHE BOOL "" FORCE)

get_filename_component(cmake_helpers_loc
    "${CMAKE_SOURCE_DIR}/third_party/desktop-app/cmake_helpers" REALPATH)
get_filename_component(submodules_loc
    "${CMAKE_SOURCE_DIR}/third_party/desktop-app" REALPATH)
get_filename_component(third_party_loc
    "${CMAKE_SOURCE_DIR}/third_party/desktop-app" REALPATH)

if(NOT EXISTS "${cmake_helpers_loc}/init_target.cmake")
    message(FATAL_ERROR "Desktop App cmake_helpers missing. Run: git submodule update --init --recursive")
endif()

# Qt 6.8 lacks QAccessible::Attribute::Orientation (added in 6.9+).
set(_zarya_lib_ui_orient_patch
    "${CMAKE_SOURCE_DIR}/cmake/desktop_app_patches/lib_ui_qt68_accessible_orientation.patch")
set(_zarya_lib_ui_orient_marker
    "${submodules_loc}/lib_ui/ui/accessible/ui_accessible_widget.cpp")
file(READ "${_zarya_lib_ui_orient_marker}" _zarya_lib_ui_orient_src)
if(NOT _zarya_lib_ui_orient_src MATCHES "QT_VERSION_CHECK\\(6, 9, 0\\)")
    find_program(GIT_EXECUTABLE git REQUIRED)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} apply --ignore-whitespace "${_zarya_lib_ui_orient_patch}"
        WORKING_DIRECTORY "${submodules_loc}/lib_ui"
        RESULT_VARIABLE _zarya_patch_rc
        ERROR_VARIABLE _zarya_patch_err)
    if(NOT _zarya_patch_rc EQUAL 0)
        message(WARNING "lib_ui Qt 6.8 accessibility patch failed: ${_zarya_patch_err}")
    else()
        message(STATUS "Applied lib_ui Qt 6.8 QAccessible::Attribute::Orientation compat patch")
    endif()
endif()

list(PREPEND CMAKE_MODULE_PATH ${cmake_helpers_loc})

include(${cmake_helpers_loc}/nice_target_sources.cmake)
include(${cmake_helpers_loc}/target_compile_options_if_exists.cmake)
include(${cmake_helpers_loc}/target_link_options_if_exists.cmake)
include(${cmake_helpers_loc}/target_link_frameworks.cmake)
include(${cmake_helpers_loc}/init_target.cmake)
include(${cmake_helpers_loc}/generate_target.cmake)
include(${cmake_helpers_loc}/target_prepare_qrc.cmake)
include(${cmake_helpers_loc}/variables.cmake)

# tdesktop keeps cmake_helpers at <root>/cmake so options.cmake can
# include(cmake/options_*.cmake) from CMAKE_SOURCE_DIR. We include the
# platform file by absolute path after creating common_options.
add_library(common_options INTERFACE)
add_library(desktop-app::common_options ALIAS common_options)
target_compile_definitions(common_options
INTERFACE
    $<$<CONFIG:Debug>:_DEBUG>
    QT_NO_KEYWORDS
    QT_NO_CAST_FROM_BYTEARRAY
    QT_DEPRECATED_WARNINGS_SINCE=0x051500
)
if(WIN32)
    include(${cmake_helpers_loc}/options_win.cmake)
elseif(APPLE)
    include(${cmake_helpers_loc}/options_mac.cmake)
elseif(LINUX)
    include(${cmake_helpers_loc}/options_linux.cmake)
else()
    message(FATAL_ERROR "ZARYA_DESKTOP_APP_UI: unsupported platform")
endif()

# Prefer Shining Light / system OpenSSL on Windows if present.
if(WIN32 AND NOT OPENSSL_ROOT_DIR)
    foreach(_openssl_candidate
        "C:/Program Files/OpenSSL-Win64"
        "C:/Program Files/OpenSSL"
        "C:/OpenSSL-Win64")
        if(EXISTS "${_openssl_candidate}/include/openssl/ssl.h")
            set(OPENSSL_ROOT_DIR "${_openssl_candidate}" CACHE PATH "" FORCE)
            break()
        endif()
    endforeach()
endif()

# Static Zarya uses /MT — prefer matching OpenSSL static libs.
if(ZARYA_STATIC_QT AND WIN32)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
    set(OPENSSL_MSVC_STATIC_RT TRUE)
endif()

find_package(OpenSSL REQUIRED)
find_package(ZLIB)
find_package(JPEG)

# Qt modules required by toolkit.
find_package(Qt6 REQUIRED COMPONENTS OpenGL OpenGLWidgets)

# Static Qt kits often omit Svg; codegen_style / lib_ui need QSvgRenderer.
# find_package(Qt6 COMPONENTS Svg) only searches the already-resolved Qt6 prefix,
# so a shared-kit fallback must set Qt6Svg_DIR explicitly.
# Never link a shared Qt6Svg into a static-Qt app (mixed linking is unsupported).
find_package(Qt6 COMPONENTS Svg QUIET)
if(NOT Qt6Svg_FOUND)
    get_filename_component(_zarya_app_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
    foreach(_svg_qt_candidate
        "${_zarya_app_qt_prefix}"
        "C:/Qt/6.8.3/msvc2022_64"
        "C:/Qt/6.9.1/msvc2022_64"
        "C:/Qt/6.7.3/msvc2022_64")
        set(_svg_config "${_svg_qt_candidate}/lib/cmake/Qt6Svg/Qt6SvgConfig.cmake")
        if(NOT EXISTS "${_svg_config}")
            continue()
        endif()
        get_filename_component(_svg_prefix "${_svg_qt_candidate}" ABSOLUTE)
        if(ZARYA_STATIC_QT AND NOT _svg_prefix STREQUAL _zarya_app_qt_prefix)
            message(STATUS
                "Shared Qt Svg at ${_svg_prefix} found but not linked into static build "
                "(mixed Qt linking unsupported). Rebuild static Qt with Svg: "
                "scripts/build-qt-static-msvc2026.ps1 -SvgOnly")
            continue()
        endif()
        set(Qt6Svg_DIR "${_svg_qt_candidate}/lib/cmake/Qt6Svg")
        find_package(Qt6Svg CONFIG QUIET)
        if(Qt6Svg_FOUND)
            message(STATUS "Using Qt Svg from ${_svg_prefix}")
            break()
        endif()
    endforeach()
endif()
if(Qt6Svg_FOUND)
    set(ZARYA_HAS_QT_SVG ON)
else()
    set(ZARYA_HAS_QT_SVG OFF)
    message(WARNING "Qt6 Svg not found — codegen_style will use a stub RenderSvg")
endif()
if(QT_VERSION VERSION_GREATER_EQUAL 6.10)
    find_package(Qt6 REQUIRED COMPONENTS GuiPrivate WidgetsPrivate)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/ZaryaDesktopAppExternals.cmake)

add_subdirectory(${submodules_loc}/lib_crl ${CMAKE_BINARY_DIR}/desktop_app/lib_crl)
add_subdirectory(${submodules_loc}/lib_rpl ${CMAKE_BINARY_DIR}/desktop_app/lib_rpl)
add_subdirectory(${submodules_loc}/lib_base ${CMAKE_BINARY_DIR}/desktop_app/lib_base)

# Codegen tools (style + emoji generators).
add_subdirectory(${submodules_loc}/codegen/codegen/common
    ${CMAKE_BINARY_DIR}/desktop_app/codegen_common)
add_subdirectory(${submodules_loc}/codegen/codegen/style
    ${CMAKE_BINARY_DIR}/desktop_app/codegen_style)
add_subdirectory(${submodules_loc}/codegen/codegen/emoji
    ${CMAKE_BINARY_DIR}/desktop_app/codegen_emoji)

if(NOT ZARYA_HAS_QT_SVG)
    # Replace SVG-dependent translation unit with a stub and inject a
    # minimal QSvgRenderer so generator.cpp / style_core_icon.cpp still compile.
    get_target_property(_codegen_style_sources codegen_style SOURCES)
    list(FILTER _codegen_style_sources EXCLUDE REGEX "render_svg\\.cpp$")
    set_property(TARGET codegen_style PROPERTY SOURCES ${_codegen_style_sources})
    target_sources(codegen_style PRIVATE
        ${CMAKE_SOURCE_DIR}/cmake/desktop_app_stubs/render_svg_stub.cpp)
    target_include_directories(codegen_style BEFORE PRIVATE
        ${CMAKE_SOURCE_DIR}/cmake/desktop_app_stubs)
endif()

add_subdirectory(${submodules_loc}/lib_ui ${CMAKE_BINARY_DIR}/desktop_app/lib_ui)

if(NOT ZARYA_HAS_QT_SVG AND TARGET lib_ui)
    target_include_directories(lib_ui BEFORE PRIVATE
        ${CMAKE_SOURCE_DIR}/cmake/desktop_app_stubs)
endif()

# Parent project enables CMAKE_AUTOUIC; lib_ui headers like
# ui/platform/linux/ui_utility_linux.h confuse uic. Toolkit has no .ui files.
foreach(_zarya_da_target lib_ui lib_base lib_crl lib_rpl codegen_common codegen_style codegen_emoji external_lz4_bundled)
    if(TARGET ${_zarya_da_target})
        set_target_properties(${_zarya_da_target} PROPERTIES AUTOUIC OFF)
    endif()
endforeach()

set(ZARYA_DESKTOP_APP_UI_SOURCES
    src/ui/desktopapp/ZaryaBaseIntegration.cpp
    src/ui/desktopapp/ZaryaUiIntegration.cpp
    src/ui/desktopapp/ZaryaCrlIntegration.cpp
    src/ui/desktopapp/LibUiSpikeDialog.cpp)

# Spike sources consume toolkit headers that assume QT_NO_KEYWORDS + base PCH.
set_source_files_properties(${ZARYA_DESKTOP_APP_UI_SOURCES} PROPERTIES
    COMPILE_DEFINITIONS "QT_NO_KEYWORDS"
    COMPILE_OPTIONS "/FI${submodules_loc}/lib_ui/ui/ui_pch.h")

# lz4 is PRIVATE on lib_ui; static link requires the final exe to pull it in.
set(ZARYA_DESKTOP_APP_UI_LIBS
    desktop-app::lib_ui
    desktop-app::external_lz4
    desktop-app::lib_crl)
