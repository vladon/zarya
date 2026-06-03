cmake_minimum_required(VERSION 3.21)

project(zarya VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(ZARYA_STATIC_QT "Link Qt statically into zarya (requires a static Qt build)" OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
include(ZaryaQt)

zarya_detect_static_qt(_zarya_qt_is_static)
if(_zarya_qt_is_static AND NOT ZARYA_STATIC_QT)
    message(STATUS "Static Qt detected at CMAKE_PREFIX_PATH; enabling ZARYA_STATIC_QT")
    set(ZARYA_STATIC_QT ON)
endif()

if(ZARYA_STATIC_QT AND NOT _zarya_qt_is_static)
    message(WARNING "ZARYA_STATIC_QT is ON but Qt6::Core does not appear to be static. "
                    "Point CMAKE_PREFIX_PATH to a static qtbase install, e.g. C:/Qt/Static/6.8.3/msvc2022_64")
endif()

qt_standard_project_setup()

set(ZARYA_DOMAIN_SOURCES
    src/domain/Profile.cpp
    src/domain/CoreType.cpp
    src/domain/ProtocolType.cpp
    src/domain/ProfileValidation.cpp
)

set(ZARYA_CORE_SOURCES
    src/core/XrayAdapter.cpp
    src/core/XrayVlessGenerator.cpp
    src/core/XrayConfigTestHelpers.cpp
    src/core/SingBoxAdapter.cpp
    src/core/CoreManager.cpp
)

set(ZARYA_SOURCES
    src/app/main.cpp
    src/app/Application.cpp
    src/ui/MainWindow.cpp
    src/ui/ProfileDialog.cpp
    src/ui/SettingsDialog.cpp
    src/ui/ImportVlessDialog.cpp
    src/ui/models/ProfileTableModel.cpp
    ${ZARYA_CORE_SOURCES}
    src/storage/ProfileStore.cpp
    src/storage/AppSettings.cpp
    src/import/VlessUriParser.cpp
    src/platform/Platform.cpp
    src/storage/AppPaths.cpp
    ${ZARYA_DOMAIN_SOURCES}
)

set(ZARYA_HEADERS
    src/app/Application.h
    src/ui/MainWindow.h
    src/ui/ProfileDialog.h
    src/ui/SettingsDialog.h
    src/ui/ImportVlessDialog.h
    src/ui/models/ProfileTableModel.h
    src/core/ICoreAdapter.h
    src/core/XrayAdapter.h
    src/core/XrayVlessGenerator.h
    src/core/XrayConfigTestHelpers.h
    src/core/SingBoxAdapter.h
    src/core/CoreManager.h
    src/storage/ProfileStore.h
    src/storage/AppSettings.h
    src/import/VlessUriParser.h
    src/platform/Platform.h
    src/storage/AppPaths.h
    src/domain/Profile.h
    src/domain/CoreType.h
    src/domain/ProtocolType.h
    src/domain/ProfileValidation.h
)

add_executable(zarya ${ZARYA_SOURCES} ${ZARYA_HEADERS})

target_include_directories(zarya PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

zarya_link_qt_widgets_app(zarya)

if(WIN32)
    set_target_properties(zarya PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(ZARYA_STATIC_QT AND WIN32)
    message(STATUS "zarya Release will be a single portable executable (Qt linked statically, /MT runtime)")
endif()

install(TARGETS zarya RUNTIME DESTINATION bin)

# Config generation checks (unit-like; uses shared or static Qt Core)
add_executable(zarya_xray_config_test
    tests/xray_config_test.cpp
    ${ZARYA_CORE_SOURCES}
    ${ZARYA_DOMAIN_SOURCES}
    src/storage/ProfileStore.cpp
    src/storage/AppSettings.cpp
    src/storage/AppPaths.cpp
    src/platform/Platform.cpp
    src/import/VlessUriParser.cpp
)

target_include_directories(zarya_xray_config_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(zarya_xray_config_test PRIVATE
    ZARYA_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/examples"
)

zarya_link_qt_core(zarya_xray_config_test)
