# Desktop App Toolkit (lib_ui) — required. Official binaries are GPLv3+ derivatives.

set(ZARYA_DESKTOP_APP_UI ON CACHE BOOL "Link desktop-app lib_ui (required; GPLv3+)" FORCE)

message(STATUS "Linking Desktop App Toolkit (lib_ui) — GPLv3+ binary")

# lz4 and other bundled deps ship .c sources; macOS toolkit uses ObjC++ (.mm).
enable_language(C)
if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

set(DESKTOP_APP_SPECIAL_TARGET "" CACHE STRING "" FORCE)
set(DESKTOP_APP_USE_PACKAGED ON CACHE BOOL "" FORCE)
set(DESKTOP_APP_DISABLE_CRASH_REPORTS ON CACHE BOOL "" FORCE)
set(DESKTOP_APP_USE_PACKAGED_FONTS OFF CACHE BOOL "" FORCE)

get_filename_component(cmake_helpers_loc
    "${CMAKE_SOURCE_DIR}/third_party/desktop-app/cmake_helpers" REALPATH)
get_filename_component(desktop_app_loc
    "${CMAKE_SOURCE_DIR}/third_party/desktop-app" REALPATH)
get_filename_component(third_party_loc
    "${CMAKE_SOURCE_DIR}/third_party/desktop-app" REALPATH)

# Upstream Desktop App CMake files still use this historical variable name for
# their shared source root (for example, to locate Resources).
set(submodules_loc "${desktop_app_loc}")

if(NOT EXISTS "${cmake_helpers_loc}/init_target.cmake")
    message(FATAL_ERROR
        "Vendored Desktop App cmake_helpers are missing. "
        "Restore third_party/desktop-app from the repository checkout.")
endif()

# Qt::NoTitleBarBackgroundHint is Qt 6.9+; upstream lib_ui incorrectly gates at 6.8.
# Write a patched TU into the build tree so vendored lib_ui stays unmodified.
function(zarya_write_lib_ui_qt69_window_mac_patch out_path)
    set(_orig "${desktop_app_loc}/lib_ui/ui/platform/mac/ui_window_mac.mm")
    if(NOT EXISTS "${_orig}")
        message(FATAL_ERROR "Missing ${_orig}")
    endif()
    file(READ "${_orig}" _src)
    string(REPLACE
        "QT_VERSION_CHECK(6, 8, 0)"
        "QT_VERSION_CHECK(6, 9, 0)"
        _src "${_src}")
    if(NOT _src MATCHES "QT_VERSION_CHECK\\(6, 9, 0\\)")
        message(FATAL_ERROR
            "Failed to patch ui_window_mac.mm for Qt < 6.9 "
            "(Qt::NoTitleBarBackgroundHint). Upstream lib_ui may have changed.")
    endif()
    if(_src MATCHES "QT_VERSION_CHECK\\(6, 8, 0\\)")
        message(FATAL_ERROR
            "ui_window_mac.mm still has Qt 6.8 gates after patch")
    endif()
    file(WRITE "${out_path}" "${_src}")
endfunction()

# QAccessible::Attribute::Orientation is not in Qt 6.8–6.9; gate it for Qt 6.10+.
# Write a patched TU into the build tree so vendored lib_ui stays unmodified.
function(zarya_write_lib_ui_qt68_accessible_patch out_path)
    set(_orig "${desktop_app_loc}/lib_ui/ui/accessible/ui_accessible_widget.cpp")
    if(NOT EXISTS "${_orig}")
        message(FATAL_ERROR "Missing ${_orig}")
    endif()
    file(READ "${_orig}" _src)
    # Normalize any existing 6.9 gate up to 6.10 (Orientation is not in 6.9.x).
    string(REPLACE
        "QT_VERSION_CHECK(6, 9, 0)"
        "QT_VERSION_CHECK(6, 10, 0)"
        _src "${_src}")
    if(NOT _src MATCHES "QT_VERSION_CHECK\\(6, 10, 0\\)")
        string(REPLACE
            "QList<QAccessible::Attribute> Widget::attributeKeys() const {\n\tauto result = QList<QAccessible::Attribute>();\n\tif (rp()->accessibilityOrientation().has_value()) {\n\t\tresult.append(QAccessible::Attribute::Orientation);\n\t}\n\treturn result;\n}"
            "QList<QAccessible::Attribute> Widget::attributeKeys() const {\n\tauto result = QList<QAccessible::Attribute>();\n#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)\n\tif (rp()->accessibilityOrientation().has_value()) {\n\t\tresult.append(QAccessible::Attribute::Orientation);\n\t}\n#else\n\tQ_UNUSED(result);\n#endif\n\treturn result;\n}"
            _src "${_src}")
        string(REPLACE
            "QVariant Widget::attributeValue(QAccessible::Attribute key) const {\n\tif (key == QAccessible::Attribute::Orientation) {\n\t\tif (const auto orientation = rp()->accessibilityOrientation()) {\n\t\t\t// Plain int by design: the UIA bridge reads this back with\n\t\t\t// QVariant::toInt(), and Qt::Orientation isn't a registered\n\t\t\t// metatype here - QVariant::fromValue() of it wouldn't round-trip.\n\t\t\treturn int(*orientation);\n\t\t}\n\t}\n\treturn QVariant();\n}"
            "QVariant Widget::attributeValue(QAccessible::Attribute key) const {\n#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)\n\tif (key == QAccessible::Attribute::Orientation) {\n\t\tif (const auto orientation = rp()->accessibilityOrientation()) {\n\t\t\t// Plain int by design: the UIA bridge reads this back with\n\t\t\t// QVariant::toInt(), and Qt::Orientation isn't a registered\n\t\t\t// metatype here - QVariant::fromValue() of it wouldn't round-trip.\n\t\t\treturn int(*orientation);\n\t\t}\n\t}\n#else\n\tQ_UNUSED(key);\n#endif\n\treturn QVariant();\n}"
            _src "${_src}")
    endif()
    if(NOT _src MATCHES "QT_VERSION_CHECK\\(6, 10, 0\\)")
        message(FATAL_ERROR
            "Failed to patch ui_accessible_widget.cpp for Qt < 6.10 "
            "(QAccessible::Attribute::Orientation). Upstream lib_ui may have changed.")
    endif()
    file(WRITE "${out_path}" "${_src}")
endfunction()

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
# Prefer system ZLIB/JPEG when present; otherwise Externals fall back to Qt bundled.
# QUIET: static Qt has no system libs — avoid noisy "Could NOT find" configure spam.
find_package(ZLIB QUIET)
find_package(JPEG QUIET)

# Qt modules required by toolkit.
find_package(Qt6 REQUIRED COMPONENTS OpenGL OpenGLWidgets)
if(LINUX)
    find_package(Qt6 REQUIRED COMPONENTS DBus)
endif()

# macOS 15+ SDKs dropped AGL; strip it from OpenGL imported targets and provide
# a tiny stub framework so any remaining -framework AGL resolves at link time.
if(APPLE)
    foreach(_zarya_gl_tgt OpenGL::GL OpenGL::OpenGL Qt6::OpenGL Qt6::OpenGLWidgets)
        if(TARGET ${_zarya_gl_tgt})
            get_target_property(_zarya_gl_libs ${_zarya_gl_tgt} INTERFACE_LINK_LIBRARIES)
            if(_zarya_gl_libs)
                list(FILTER _zarya_gl_libs EXCLUDE REGEX "[Aa][Gg][Ll]")
                set_property(TARGET ${_zarya_gl_tgt} PROPERTY INTERFACE_LINK_LIBRARIES "${_zarya_gl_libs}")
            endif()
            get_target_property(_zarya_gl_opts ${_zarya_gl_tgt} INTERFACE_LINK_OPTIONS)
            if(_zarya_gl_opts)
                list(FILTER _zarya_gl_opts EXCLUDE REGEX "[Aa][Gg][Ll]")
                set_property(TARGET ${_zarya_gl_tgt} PROPERTY INTERFACE_LINK_OPTIONS "${_zarya_gl_opts}")
            endif()
        endif()
    endforeach()
    set(_zarya_agl_stub_root "${CMAKE_BINARY_DIR}/macos-stubs")
    set(_zarya_agl_fw "${_zarya_agl_stub_root}/AGL.framework")
    file(MAKE_DIRECTORY "${_zarya_agl_fw}/Versions/A")
    file(WRITE "${_zarya_agl_stub_root}/agl_stub.c" "void zarya_agl_stub(void) {}\n")
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} -dynamiclib
            -install_name "/System/Library/Frameworks/AGL.framework/Versions/A/AGL"
            -o "${_zarya_agl_fw}/Versions/A/AGL"
            "${_zarya_agl_stub_root}/agl_stub.c"
        RESULT_VARIABLE _zarya_agl_stub_rc
        OUTPUT_VARIABLE _zarya_agl_stub_out
        ERROR_VARIABLE _zarya_agl_stub_err)
    if(NOT _zarya_agl_stub_rc EQUAL 0)
        message(WARNING "Failed to build AGL stub framework: ${_zarya_agl_stub_err}")
    else()
        file(CREATE_LINK "A" "${_zarya_agl_fw}/Versions/Current" SYMBOLIC)
        file(CREATE_LINK "Versions/Current/AGL" "${_zarya_agl_fw}/AGL" SYMBOLIC)
        add_link_options("LINKER:-F${_zarya_agl_stub_root}")
        message(STATUS "Using stub AGL.framework at ${_zarya_agl_fw}")
    endif()
endif()

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
# Qt 6.10+ ships GuiPrivate/WidgetsPrivate as separate CMake packages.
# Older Qt still exposes private headers via *_PRIVATE_INCLUDE_DIRS (see Externals).
find_package(Qt6 QUIET COMPONENTS GuiPrivate WidgetsPrivate)
if(QT_VERSION VERSION_GREATER_EQUAL 6.10)
    find_package(Qt6 REQUIRED COMPONENTS GuiPrivate WidgetsPrivate)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/ZaryaDesktopAppExternals.cmake)

# Linux lib_base needs glib/cppgir D-Bus codegen and kcoreaddons (packaged KF or bundled).
if(LINUX)
    add_subdirectory(${cmake_helpers_loc}/external/glib
        ${CMAKE_BINARY_DIR}/desktop_app/external_glib)
    add_subdirectory(${cmake_helpers_loc}/external/kcoreaddons
        ${CMAKE_BINARY_DIR}/desktop_app/external_kcoreaddons)
endif()

add_subdirectory(${desktop_app_loc}/lib_crl ${CMAKE_BINARY_DIR}/desktop_app/lib_crl)
add_subdirectory(${desktop_app_loc}/lib_rpl ${CMAKE_BINARY_DIR}/desktop_app/lib_rpl)
add_subdirectory(${desktop_app_loc}/lib_base ${CMAKE_BINARY_DIR}/desktop_app/lib_base)

# Codegen tools (style + emoji generators).
add_subdirectory(${desktop_app_loc}/codegen/codegen/common
    ${CMAKE_BINARY_DIR}/desktop_app/codegen_common)
add_subdirectory(${desktop_app_loc}/codegen/codegen/style
    ${CMAKE_BINARY_DIR}/desktop_app/codegen_style)
add_subdirectory(${desktop_app_loc}/codegen/codegen/emoji
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

add_subdirectory(${desktop_app_loc}/lib_ui ${CMAKE_BINARY_DIR}/desktop_app/lib_ui)

# Swap toolkit TUs for older Qt using build-tree copies.
if(TARGET lib_ui)
    set(_zarya_patched_dir "${CMAKE_BINARY_DIR}/desktop_app/patched")
    file(MAKE_DIRECTORY "${_zarya_patched_dir}")
    set(_zarya_lib_ui_need_rescan FALSE)

    if(APPLE AND QT_VERSION VERSION_LESS 6.9.0)
        set(_zarya_mac_window_patched
            "${_zarya_patched_dir}/ui_window_mac.mm")
        zarya_write_lib_ui_qt69_window_mac_patch("${_zarya_mac_window_patched}")
        set_source_files_properties("${_zarya_mac_window_patched}" PROPERTIES
            LANGUAGE OBJCXX)
        set(_zarya_lib_ui_need_rescan TRUE)
        message(STATUS "lib_ui: using Qt < 6.9-safe ui_window_mac.mm from build tree")
    endif()

    if(QT_VERSION VERSION_LESS 6.10.0)
        set(_zarya_a11y_patched
            "${_zarya_patched_dir}/ui_accessible_widget.cpp")
        zarya_write_lib_ui_qt68_accessible_patch("${_zarya_a11y_patched}")
        set(_zarya_lib_ui_need_rescan TRUE)
        message(STATUS "lib_ui: using Qt < 6.10-safe ui_accessible_widget.cpp from build tree")
    endif()

    if(_zarya_lib_ui_need_rescan)
        get_target_property(_zarya_lib_ui_sources lib_ui SOURCES)
        set(_zarya_lib_ui_new_sources)
        foreach(_zarya_src IN LISTS _zarya_lib_ui_sources)
            if(DEFINED _zarya_a11y_patched AND _zarya_src MATCHES "ui_accessible_widget\\.cpp$")
                list(APPEND _zarya_lib_ui_new_sources "${_zarya_a11y_patched}")
            elseif(DEFINED _zarya_mac_window_patched AND _zarya_src MATCHES "ui_window_mac\\.mm$")
                list(APPEND _zarya_lib_ui_new_sources "${_zarya_mac_window_patched}")
            else()
                list(APPEND _zarya_lib_ui_new_sources "${_zarya_src}")
            endif()
        endforeach()
        set_property(TARGET lib_ui PROPERTY SOURCES ${_zarya_lib_ui_new_sources})
    endif()
endif()

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
    src/ui/desktopapp/ZaryaPalette.cpp
    src/ui/desktopapp/ZaryaControls.cpp
    src/ui/desktopapp/ZaryaSelector.cpp
    src/ui/desktopapp/ProfileActionStrip.cpp
    src/ui/desktopapp/UiMessagePresenter.cpp
    src/ui/desktopapp/ProfileEmptyStatePanel.cpp
    src/ui/desktopapp/StatusBadgeLibUiEmbed.cpp
    src/ui/desktopapp/StatusConfiguredStrip.cpp
    src/ui/desktopapp/StatusUnconfiguredPanel.cpp)


# Zarya toolkit sources consume headers that assume QT_NO_KEYWORDS + the lib_ui PCH.
# MSVC: /FI; GCC/Clang: -include (never pass /FI to non-MSVC).
set(_zarya_ui_pch "${desktop_app_loc}/lib_ui/ui/ui_pch.h")
if(MSVC)
    set_source_files_properties(${ZARYA_DESKTOP_APP_UI_SOURCES} PROPERTIES
        COMPILE_DEFINITIONS "QT_NO_KEYWORDS"
        COMPILE_OPTIONS "/FI${_zarya_ui_pch}")
else()
    set_source_files_properties(${ZARYA_DESKTOP_APP_UI_SOURCES} PROPERTIES
        COMPILE_DEFINITIONS "QT_NO_KEYWORDS"
        COMPILE_OPTIONS "-include;${_zarya_ui_pch}")
endif()

# lz4 is PRIVATE on lib_ui; static link requires the final exe to pull it in.
set(ZARYA_DESKTOP_APP_UI_LIBS
    desktop-app::lib_ui
    desktop-app::external_lz4
    desktop-app::lib_crl)
