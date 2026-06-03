cmake_minimum_required(VERSION 3.21)

project(zarya VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

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
    src/ui/models/ProfileTableModel.cpp
    ${ZARYA_CORE_SOURCES}
    src/storage/ProfileStore.cpp
    src/platform/Platform.cpp
    src/storage/AppPaths.cpp
    ${ZARYA_DOMAIN_SOURCES}
)

set(ZARYA_HEADERS
    src/app/Application.h
    src/ui/MainWindow.h
    src/ui/ProfileDialog.h
    src/ui/models/ProfileTableModel.h
    src/core/ICoreAdapter.h
    src/core/XrayAdapter.h
    src/core/XrayVlessGenerator.h
    src/core/XrayConfigTestHelpers.h
    src/core/SingBoxAdapter.h
    src/core/CoreManager.h
    src/storage/ProfileStore.h
    src/platform/Platform.h
    src/storage/AppPaths.h
    src/domain/Profile.h
    src/domain/CoreType.h
    src/domain/ProtocolType.h
    src/domain/ProfileValidation.h
)

add_executable(zarya ${ZARYA_SOURCES} ${ZARYA_HEADERS})

target_include_directories(zarya PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(zarya PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

if(WIN32)
    set_target_properties(zarya PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

install(TARGETS zarya RUNTIME DESTINATION bin)

# Config generation checks (unit-like; no Qt Test framework required)
add_executable(zarya_xray_config_test
    tests/xray_config_test.cpp
    ${ZARYA_CORE_SOURCES}
    ${ZARYA_DOMAIN_SOURCES}
    src/storage/ProfileStore.cpp
)

target_include_directories(zarya_xray_config_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(zarya_xray_config_test PRIVATE
    ZARYA_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/examples"
)

target_link_libraries(zarya_xray_config_test PRIVATE Qt6::Core)
