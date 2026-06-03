cmake_minimum_required(VERSION 3.21)

project(zarya VERSION 0.8.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(ZARYA_STATIC_QT "Link Qt statically into zarya (requires a static Qt build)" OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
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
    src/domain/ProfileSourceType.cpp
    src/domain/Subscription.cpp
    src/domain/SubscriptionStatus.cpp
    src/domain/RoutingMode.cpp
    src/domain/RoutingRule.cpp
    src/domain/RoutingProfile.cpp
)

set(ZARYA_ROUTING_SOURCES
    src/routing/RoutingManager.cpp
    src/routing/XrayRoutingGenerator.cpp
    src/routing/RoutingProfileValidator.cpp
)

set(ZARYA_TESTING_SOURCES
    src/testing/TestStatus.cpp
    src/testing/PortAllocator.cpp
    src/testing/TcpPingTester.cpp
    src/testing/RealDelayTester.cpp
    src/testing/TestManager.cpp
)

set(ZARYA_SUBSCRIPTION_SOURCES
    src/subscription/ShareLinkParser.cpp
    src/subscription/SubscriptionParser.cpp
    src/subscription/SubscriptionDownloader.cpp
    src/subscription/SubscriptionManager.cpp
)

set(ZARYA_CORE_SOURCES
    src/core/XrayAdapter.cpp
    src/core/XrayVlessGenerator.cpp
    src/core/XrayStreamSettings.cpp
    src/core/XrayConfigBuilder.cpp
    src/core/XrayConfigTestHelpers.cpp
    src/core/SingBoxAdapter.cpp
    src/core/CoreManager.cpp
)

set(ZARYA_PLATFORM_SOURCES
    src/platform/Platform.cpp
    src/platform/SystemProxyManagerFactory.cpp
    src/platform/SystemProxyController.cpp
    src/platform/SystemProxyDebug.cpp
)

if(WIN32)
    list(APPEND ZARYA_PLATFORM_SOURCES src/platform/windows/WindowsSystemProxyManager.cpp)
else()
    list(APPEND ZARYA_PLATFORM_SOURCES src/platform/stub/StubSystemProxyManager.cpp)
endif()

set(ZARYA_SOURCES
    src/app/main.cpp
    src/app/Application.cpp
    src/app/AppController.cpp
    src/ui/MainWindow.cpp
    src/ui/TrayController.cpp
    src/ui/ProfileDialog.cpp
    src/ui/SettingsDialog.cpp
    src/ui/ImportVlessDialog.cpp
    src/ui/SubscriptionDialog.cpp
    src/ui/SubscriptionManagerDialog.cpp
    src/ui/RoutingManagerDialog.cpp
    src/ui/RoutingProfileDialog.cpp
    src/ui/RoutingRuleEditorDialog.cpp
    src/ui/RoutingJsonPreviewDialog.cpp
    src/ui/models/ProfileTableModel.cpp
    src/ui/models/SubscriptionTableModel.cpp
    ${ZARYA_CORE_SOURCES}
    ${ZARYA_SUBSCRIPTION_SOURCES}
    ${ZARYA_TESTING_SOURCES}
    src/storage/ProfileStore.cpp
    src/storage/SubscriptionStore.cpp
    src/storage/RoutingStore.cpp
    src/storage/AppSettings.cpp
    ${ZARYA_ROUTING_SOURCES}
    src/import/VlessUriParser.cpp
    ${ZARYA_PLATFORM_SOURCES}
    src/storage/AppPaths.cpp
    ${ZARYA_DOMAIN_SOURCES}
)

set(ZARYA_HEADERS
    src/app/Application.h
    src/app/AppController.h
    src/ui/MainWindow.h
    src/ui/TrayController.h
    src/ui/ProfileDialog.h
    src/ui/SettingsDialog.h
    src/ui/ImportVlessDialog.h
    src/ui/SubscriptionDialog.h
    src/ui/SubscriptionManagerDialog.h
    src/ui/models/ProfileTableModel.h
    src/ui/models/SubscriptionTableModel.h
    src/core/ICoreAdapter.h
    src/core/XrayAdapter.h
    src/core/XrayVlessGenerator.h
    src/core/XrayStreamSettings.h
    src/core/XrayConfigBuilder.h
    src/core/XrayConfigTestHelpers.h
    src/core/SingBoxAdapter.h
    src/core/CoreManager.h
    src/storage/ProfileStore.h
    src/storage/SubscriptionStore.h
    src/storage/AppSettings.h
    src/import/VlessUriParser.h
    src/subscription/ShareLinkParser.h
    src/subscription/SubscriptionParser.h
    src/subscription/SubscriptionDownloader.h
    src/subscription/SubscriptionManager.h
    src/testing/TestStatus.h
    src/testing/TestResult.h
    src/testing/PortAllocator.h
    src/testing/TcpPingTester.h
    src/testing/RealDelayTester.h
    src/testing/TestManager.h
    src/domain/ProfileSourceType.h
    src/domain/Subscription.h
    src/domain/SubscriptionStatus.h
    src/platform/Platform.h
    src/platform/ISystemProxyManager.h
    src/platform/SystemProxyState.h
    src/platform/SystemProxyManagerFactory.h
    src/platform/SystemProxyController.h
    src/platform/SystemProxyDebug.h
    src/storage/AppPaths.h
    src/domain/Profile.h
    src/domain/CoreType.h
    src/domain/ProtocolType.h
    src/domain/ProfileValidation.h
)

add_executable(zarya ${ZARYA_SOURCES} ${ZARYA_HEADERS})

target_include_directories(zarya PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

zarya_link_qt_widgets_app(zarya)
target_link_libraries(zarya PRIVATE Qt6::Network)

if(WIN32)
    set_target_properties(zarya PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_libraries(zarya PRIVATE wininet)
endif()

if(ZARYA_STATIC_QT AND WIN32)
    message(STATUS "zarya Release will be a single portable executable (Qt linked statically, /MT runtime)")
endif()

install(TARGETS zarya RUNTIME DESTINATION bin)

# Config generation checks (unit-like; uses shared or static Qt Core)
add_executable(zarya_xray_config_test
    tests/xray_config_test.cpp
    ${ZARYA_CORE_SOURCES}
    ${ZARYA_ROUTING_SOURCES}
    ${ZARYA_DOMAIN_SOURCES}
    src/storage/RoutingStore.cpp
    src/testing/TestStatus.cpp
    src/storage/ProfileStore.cpp
    src/storage/AppSettings.cpp
    src/storage/AppPaths.cpp
    src/platform/Platform.cpp
    src/import/VlessUriParser.cpp
    src/subscription/ShareLinkParser.cpp
    src/subscription/SubscriptionParser.cpp
)

target_include_directories(zarya_xray_config_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(zarya_xray_config_test PRIVATE
    ZARYA_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/examples"
)

zarya_link_qt_core(zarya_xray_config_test)

if(WIN32)
    add_executable(zarya_system_proxy_test
        tests/system_proxy_test.cpp
        src/platform/SystemProxyManagerFactory.cpp
        src/platform/SystemProxyDebug.cpp
        src/platform/windows/WindowsSystemProxyManager.cpp
    )

    target_include_directories(zarya_system_proxy_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

    zarya_link_qt_core(zarya_system_proxy_test)
    target_link_libraries(zarya_system_proxy_test PRIVATE wininet)
endif()

add_executable(zarya_subscription_test
    tests/subscription_test.cpp
    ${ZARYA_SUBSCRIPTION_SOURCES}
    ${ZARYA_DOMAIN_SOURCES}
    src/testing/TestStatus.cpp
    src/storage/ProfileStore.cpp
    src/storage/SubscriptionStore.cpp
    src/storage/AppPaths.cpp
    src/import/VlessUriParser.cpp
)

target_include_directories(zarya_subscription_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

zarya_link_qt_core(zarya_subscription_test)
target_link_libraries(zarya_subscription_test PRIVATE Qt6::Network)

add_executable(zarya_testing_test
    tests/testing_test.cpp
    ${ZARYA_TESTING_SOURCES}
    ${ZARYA_CORE_SOURCES}
    ${ZARYA_ROUTING_SOURCES}
    ${ZARYA_DOMAIN_SOURCES}
    src/storage/RoutingStore.cpp
    src/storage/AppSettings.cpp
    src/storage/AppPaths.cpp
    src/platform/Platform.cpp
)

target_include_directories(zarya_testing_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

zarya_link_qt_core(zarya_testing_test)
target_link_libraries(zarya_testing_test PRIVATE Qt6::Network)
