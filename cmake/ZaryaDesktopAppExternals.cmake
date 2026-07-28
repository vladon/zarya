# Minimal desktop-app::external_* targets for Zarya (packaged / local deps).
# Avoids pulling the full cmake_helpers/external tree (ffmpeg, webrtc, …).

# ---- Qt ----
add_library(external_qt INTERFACE IMPORTED GLOBAL)
add_library(desktop-app::external_qt ALIAS external_qt)
target_link_libraries(external_qt INTERFACE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)
if(TARGET Qt6::DBus)
    target_link_libraries(external_qt INTERFACE Qt6::DBus)
endif()
if(TARGET Qt6::GuiPrivate)
    target_link_libraries(external_qt INTERFACE Qt6::GuiPrivate)
elseif(DEFINED Qt6Gui_PRIVATE_INCLUDE_DIRS)
    # Qt < 6.10: private headers via Gui's PRIVATE_INCLUDE_DIRS (needed by lib_ui).
    target_include_directories(external_qt SYSTEM INTERFACE ${Qt6Gui_PRIVATE_INCLUDE_DIRS})
endif()
if(TARGET Qt6::WidgetsPrivate)
    target_link_libraries(external_qt INTERFACE Qt6::WidgetsPrivate)
elseif(DEFINED Qt6Widgets_PRIVATE_INCLUDE_DIRS)
    target_include_directories(external_qt SYSTEM INTERFACE ${Qt6Widgets_PRIVATE_INCLUDE_DIRS})
endif()
if(TARGET Qt6::CorePrivate)
    target_link_libraries(external_qt INTERFACE Qt6::CorePrivate)
elseif(DEFINED Qt6Core_PRIVATE_INCLUDE_DIRS)
    target_include_directories(external_qt SYSTEM INTERFACE ${Qt6Core_PRIVATE_INCLUDE_DIRS})
endif()
if(ZARYA_HAS_QT_SVG AND TARGET Qt6::Svg)
    target_link_libraries(external_qt INTERFACE Qt6::Svg)
endif()

# Empty plugins stub expected by some helpers.
add_library(external_qt_static_plugins INTERFACE IMPORTED GLOBAL)
add_library(desktop-app::external_qt_static_plugins ALIAS external_qt_static_plugins)

# ---- OpenSSL ----
# Shining Light OpenSSL-Win64 stores libs under lib/VC/x64/{MT,MD}/.
# Prefer /MT static libs when building static Zarya.
add_library(external_openssl_common INTERFACE IMPORTED GLOBAL)
add_library(desktop-app::external_openssl_common ALIAS external_openssl_common)
if(OPENSSL_INCLUDE_DIR)
    target_include_directories(external_openssl_common SYSTEM INTERFACE ${OPENSSL_INCLUDE_DIR})
elseif(OPENSSL_ROOT_DIR)
    target_include_directories(external_openssl_common SYSTEM INTERFACE ${OPENSSL_ROOT_DIR}/include)
endif()

function(zarya_openssl_pick_lib out_var lib_basename)
    set(_candidates)
    if(ZARYA_STATIC_QT AND WIN32 AND OPENSSL_ROOT_DIR)
        list(APPEND _candidates
            "${OPENSSL_ROOT_DIR}/lib/VC/x64/MT/${lib_basename}_static.lib"
            "${OPENSSL_ROOT_DIR}/lib/VC/x64/MT/${lib_basename}.lib")
    endif()
    if(WIN32 AND OPENSSL_ROOT_DIR)
        list(APPEND _candidates
            "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD/${lib_basename}_static.lib"
            "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD/${lib_basename}.lib"
            "${OPENSSL_ROOT_DIR}/lib/${lib_basename}.lib")
    endif()
    foreach(_c ${_candidates})
        if(EXISTS "${_c}")
            set(${out_var} "${_c}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${out_var} "" PARENT_SCOPE)
endfunction()

zarya_openssl_pick_lib(_zarya_libssl libssl)
zarya_openssl_pick_lib(_zarya_libcrypto libcrypto)
if(_zarya_libssl)
    message(STATUS "Desktop App OpenSSL SSL lib: ${_zarya_libssl}")
endif()
if(_zarya_libcrypto)
    message(STATUS "Desktop App OpenSSL Crypto lib: ${_zarya_libcrypto}")
endif()

add_library(external_openssl_ssl INTERFACE IMPORTED GLOBAL)
add_library(desktop-app::external_openssl_ssl ALIAS external_openssl_ssl)
if(_zarya_libssl)
    target_link_libraries(external_openssl_ssl INTERFACE
        desktop-app::external_openssl_common
        ${_zarya_libssl})
elseif(TARGET OpenSSL::SSL)
    target_link_libraries(external_openssl_ssl INTERFACE OpenSSL::SSL)
else()
    message(FATAL_ERROR "OpenSSL SSL library not found (needed for ZARYA_DESKTOP_APP_UI)")
endif()

add_library(external_openssl_crypto INTERFACE IMPORTED GLOBAL)
add_library(desktop-app::external_openssl_crypto ALIAS external_openssl_crypto)
if(_zarya_libcrypto)
    target_link_libraries(external_openssl_crypto INTERFACE
        desktop-app::external_openssl_common
        ${_zarya_libcrypto})
elseif(TARGET OpenSSL::Crypto)
    target_link_libraries(external_openssl_crypto INTERFACE OpenSSL::Crypto)
else()
    message(FATAL_ERROR "OpenSSL Crypto library not found (needed for ZARYA_DESKTOP_APP_UI)")
endif()

add_library(external_openssl INTERFACE IMPORTED GLOBAL)
add_library(desktop-app::external_openssl ALIAS external_openssl)
target_link_libraries(external_openssl INTERFACE
    desktop-app::external_openssl_ssl
    desktop-app::external_openssl_crypto
)
if(WIN32)
    target_link_libraries(external_openssl INTERFACE ws2_32 crypt32)
endif()

# ---- Crash reports (disabled) ----
add_library(external_crash_reports INTERFACE IMPORTED GLOBAL)
add_library(desktop-app::external_crash_reports ALIAS external_crash_reports)

# ---- GSL / ranges / expected (headers from submodules) ----
add_library(external_gsl INTERFACE IMPORTED GLOBAL)
add_library(desktop-app::external_gsl ALIAS external_gsl)
target_include_directories(external_gsl SYSTEM INTERFACE ${third_party_loc}/GSL/include)

add_library(external_ranges INTERFACE IMPORTED GLOBAL)
add_library(desktop-app::external_ranges ALIAS external_ranges)
target_include_directories(external_ranges SYSTEM INTERFACE ${third_party_loc}/range-v3/include)

add_library(external_expected INTERFACE IMPORTED GLOBAL)
add_library(desktop-app::external_expected ALIAS external_expected)
target_include_directories(external_expected SYSTEM INTERFACE ${third_party_loc}/expected/include)

# ---- xxHash (header-only inline) ----
add_library(external_xxhash INTERFACE IMPORTED GLOBAL)
add_library(desktop-app::external_xxhash ALIAS external_xxhash)
target_include_directories(external_xxhash SYSTEM INTERFACE ${third_party_loc}/xxHash)
target_compile_definitions(external_xxhash INTERFACE XXH_INLINE_ALL)

# ---- lz4 (build from submodule; skip bundled xxhash.c — use XXH_INLINE_ALL) ----
add_library(external_lz4_bundled STATIC)
init_target(external_lz4_bundled "(external)")
set(lz4_loc ${third_party_loc}/lz4/lib)
target_sources(external_lz4_bundled PRIVATE
    ${lz4_loc}/lz4.c
    ${lz4_loc}/lz4.h
    ${lz4_loc}/lz4frame.c
    ${lz4_loc}/lz4frame.h
    ${lz4_loc}/lz4frame_static.h
    ${lz4_loc}/lz4hc.c
    ${lz4_loc}/lz4hc.h
)
target_include_directories(external_lz4_bundled PUBLIC ${lz4_loc})
target_compile_definitions(external_lz4_bundled PRIVATE XXH_INLINE_ALL LZ4_DISABLE_DEPRECATE_WARNINGS)
target_link_libraries(external_lz4_bundled PUBLIC desktop-app::external_xxhash)
set_target_properties(external_lz4_bundled PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
add_library(external_lz4 INTERFACE IMPORTED GLOBAL)
add_library(desktop-app::external_lz4 ALIAS external_lz4)
target_link_libraries(external_lz4 INTERFACE external_lz4_bundled)

# ---- zlib ----
add_library(external_zlib INTERFACE IMPORTED GLOBAL)
add_library(desktop-app::external_zlib ALIAS external_zlib)
if(DEFINED Qt6_DIR)
    get_filename_component(_zarya_qt_prefix "${Qt6_DIR}/../../../" ABSOLUTE)
elseif(DEFINED Qt6Core_DIR)
    get_filename_component(_zarya_qt_prefix "${Qt6Core_DIR}/../../../" ABSOLUTE)
endif()
if(ZLIB_FOUND)
    target_link_libraries(external_zlib INTERFACE ZLIB::ZLIB)
elseif(TARGET Qt6::BundledZLIB)
    target_link_libraries(external_zlib INTERFACE Qt6::BundledZLIB)
    # Static Qt ships zlib.h under include/QtZlib.
    if(_zarya_qt_prefix AND EXISTS "${_zarya_qt_prefix}/include/QtZlib/zlib.h")
        target_include_directories(external_zlib INTERFACE "${_zarya_qt_prefix}/include/QtZlib")
    endif()
elseif(TARGET Qt6::ZlibPrivate)
    target_link_libraries(external_zlib INTERFACE Qt6::ZlibPrivate)
else()
    message(FATAL_ERROR "ZARYA_DESKTOP_APP_UI requires ZLIB (system or Qt bundled)")
endif()

# ---- jpeg ----
add_library(external_jpeg INTERFACE IMPORTED GLOBAL)
add_library(desktop-app::external_jpeg ALIAS external_jpeg)
if(JPEG_FOUND)
    target_link_libraries(external_jpeg INTERFACE JPEG::JPEG)
elseif(TARGET Qt6::BundledLibjpeg)
    target_link_libraries(external_jpeg INTERFACE Qt6::BundledLibjpeg)
    if(_zarya_qt_prefix AND EXISTS "${_zarya_qt_prefix}/include/QtJpeg/jpeglib.h")
        target_include_directories(external_jpeg INTERFACE "${_zarya_qt_prefix}/include/QtJpeg")
    endif()
else()
    message(FATAL_ERROR "ZARYA_DESKTOP_APP_UI requires JPEG (system or Qt BundledLibjpeg)")
endif()
