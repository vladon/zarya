cmake_minimum_required(VERSION 3.21)

project(zarya VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(ZARYA_SOURCES
    src/app/main.cpp
    src/app/Application.cpp
    src/ui/MainWindow.cpp
    src/ui/ProfileDialog.cpp
    src/ui/models/ProfileTableModel.cpp
    src/core/XrayAdapter.cpp
    src/core/SingBoxAdapter.cpp
    src/core/CoreManager.cpp
    src/storage/ProfileStore.cpp
    src/storage/AppPaths.cpp
    src/domain/Profile.cpp
    src/domain/CoreType.cpp
    src/domain/ProtocolType.cpp
)

set(ZARYA_HEADERS
    src/app/Application.h
    src/ui/MainWindow.h
    src/ui/ProfileDialog.h
    src/ui/models/ProfileTableModel.h
    src/core/CoreManager.h
    src/storage/ProfileStore.h
    src/storage/AppPaths.h
    src/domain/Profile.h
    src/domain/CoreType.h
    src/domain/ProtocolType.h
)

add_executable(zarya ${ZARYA_SOURCES} ${ZARYA_HEADERS})

target_include_directories(zarya PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(zarya PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

if(WIN32)
    set_target_properties(zarya PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
