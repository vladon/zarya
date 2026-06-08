cmake_minimum_required(VERSION 3.21)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ZaryaVersion.cmake)

project(zarya VERSION ${ZARYA_VERSION_MAJOR}.${ZARYA_VERSION_MINOR}.${ZARYA_VERSION_PATCH} LANGUAGES CXX)

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
    src/domain/DnsProfileMode.cpp
    src/domain/DnsQueryStrategy.cpp
    src/domain/DnsServer.cpp
    src/domain/DnsProfile.cpp
)

set(ZARYA_ROUTING_SOURCES
    src/routing/RoutingManager.cpp
    src/routing/RoutingGeoUtils.cpp
    src/routing/XrayRoutingGenerator.cpp
    src/routing/RoutingProfileValidator.cpp
)

set(ZARYA_DNS_SOURCES
    src/dns/DnsManager.cpp
    src/dns/DnsGeoUtils.cpp
    src/dns/XrayDnsGenerator.cpp
    src/dns/DnsValidator.cpp
)

set(ZARYA_IPC_SOURCES
    src/ipc/IpcMessage.cpp
    src/ipc/IpcTransport.cpp
    src/ipc/IpcServer.cpp
    src/ipc/IpcClient.cpp
)

set(ZARYA_HELPERCLIENT_SOURCES
    src/helperclient/HelperProcessManager.cpp
    src/storage/HelperSession.cpp
)

set(ZARYA_KILLSWITCH_COMMON
    src/killswitch/KillSwitchMode.cpp
    src/killswitch/KillSwitchState.cpp
    src/killswitch/KillSwitchMarker.cpp
    src/killswitch/KillSwitchLog.cpp
    src/killswitch/KillSwitchManager.cpp
)

if(UNIX AND NOT APPLE)
    set(ZARYA_KILLSWITCH_PLATFORM src/killswitch/linux/LinuxNftKillSwitchManager.cpp)
elseif(WIN32)
    set(ZARYA_KILLSWITCH_PLATFORM src/killswitch/windows/WindowsWfpKillSwitchManager_stub.cpp)
    set(ZARYA_KILLSWITCH_WINDOWS_WFP_SOURCES
        src/killswitch/windows/WindowsWfpKillSwitchManager.cpp
        src/killswitch/windows/WindowsWfpSession.cpp
        src/killswitch/windows/WindowsWfpFilterBuilder.cpp
        src/killswitch/windows/WindowsWfpGuids.cpp
        src/killswitch/windows/WindowsWfpUtils.cpp
    )
elseif(APPLE)
    set(ZARYA_KILLSWITCH_PLATFORM src/killswitch/macos/MacKillSwitchManager.cpp)
else()
    set(ZARYA_KILLSWITCH_PLATFORM src/killswitch/stub/StubKillSwitchManager.cpp)
endif()

set(ZARYA_KILLSWITCH_SOURCES ${ZARYA_KILLSWITCH_COMMON} ${ZARYA_KILLSWITCH_PLATFORM})

set(ZARYA_KILLSWITCH_GUI_SOURCES
    src/killswitch/KillSwitchPayloadBuilder.cpp
)

set(ZARYA_RULESET_SOURCES
    src/rulesets/RuleSetKind.cpp
    src/rulesets/RuleSetStatus.cpp
    src/rulesets/RuleSetNormalizer.cpp
    src/rulesets/RuleSetCatalog.cpp
    src/rulesets/RuleSetVerifier.cpp
    src/rulesets/RuleSetCompiler.cpp
    src/rulesets/RuleSetDownloader.cpp
    src/rulesets/RequiredRuleSetDetector.cpp
    src/rulesets/RuleSetManager.cpp
    src/storage/RuleSetStore.cpp
    src/runtime/singbox/SingBoxRuleSetContext.cpp
)

set(ZARYA_RUNTIME_SOURCES
    src/runtime/ConfigWarning.cpp
    src/runtime/IRuntimeBackend.cpp
    src/runtime/RuntimeBackendType.cpp
    src/runtime/RuntimeBackendFactory.cpp
    src/runtime/xray/XraySystemProxyRuntimeBackend.cpp
    src/runtime/singbox/SingBoxTunSupportChecker.cpp
    src/runtime/singbox/SingBoxRouteGenerator.cpp
    src/runtime/singbox/SingBoxDnsGenerator.cpp
    src/runtime/singbox/SingBoxRuleSetManager.cpp
    src/runtime/singbox/SingBoxConfigGenerator.cpp
    src/runtime/singbox/SingBoxTunRuntimeBackend.cpp
    ${ZARYA_RULESET_SOURCES}
)

set(ZARYA_GEODATA_SOURCES
    src/geodata/GeoDataFileStatus.cpp
    src/geodata/GeoDataSource.cpp
    src/geodata/GeoDataVerifier.cpp
    src/geodata/GeoDataDownloader.cpp
    src/geodata/GeoDataManager.cpp
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
    src/platform/PlatformPrivilege.cpp
    src/platform/PlatformProcessUtils.cpp
    src/platform/SystemProxyManagerFactory.cpp
    src/platform/SystemProxyController.cpp
    src/platform/SystemProxyDebug.cpp
    src/platform/stub/StubSystemProxyManager.cpp
)

set(ZARYA_AUTOSTART_SOURCES
    src/platform/AutostartManagerFactory.cpp
    src/platform/stub/StubAutostartManager.cpp
)

if(WIN32)
    list(APPEND ZARYA_PLATFORM_SOURCES src/platform/windows/WindowsSystemProxyManager.cpp)
    list(APPEND ZARYA_AUTOSTART_SOURCES src/platform/windows/WindowsAutostartManager.cpp)
elseif(APPLE)
    list(APPEND ZARYA_PLATFORM_SOURCES src/platform/macos/MacSystemProxyManager.cpp)
    list(APPEND ZARYA_AUTOSTART_SOURCES src/platform/macos/MacAutostartManager.cpp)
elseif(UNIX)
    list(APPEND ZARYA_PLATFORM_SOURCES
        src/platform/linux/LinuxDesktopEnvironment.cpp
        src/platform/linux/GnomeSystemProxyManager.cpp
        src/platform/linux/KdeSystemProxyManager.cpp
        src/platform/linux/LinuxSystemProxyManager.cpp
    )
    list(APPEND ZARYA_AUTOSTART_SOURCES src/platform/linux/LinuxAutostartManager.cpp)
endif()

set(ZARYA_SOURCES
    src/app/main.cpp
    src/app/Application.cpp
    src/app/StartupOptions.cpp
    src/app/AppController.cpp
    src/packaging/PackagingInfo.cpp
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
    src/ui/SingBoxConfigPreviewDialog.cpp
    src/ui/RuleSetManagerDialog.cpp
    src/ui/GeoDataManagerDialog.cpp
    src/ui/DnsManagerDialog.cpp
    src/ui/DnsProfileDialog.cpp
    src/ui/DnsServerEditorDialog.cpp
    src/ui/models/ProfileTableModel.cpp
    src/ui/models/SubscriptionTableModel.cpp
    ${ZARYA_CORE_SOURCES}
    ${ZARYA_SUBSCRIPTION_SOURCES}
    ${ZARYA_TESTING_SOURCES}
    src/storage/ProfileStore.cpp
    src/storage/SubscriptionStore.cpp
    src/storage/RoutingStore.cpp
    src/storage/AppSettings.cpp
    src/storage/GeoDataSettingsStore.cpp
    src/storage/DnsStore.cpp
    ${ZARYA_ROUTING_SOURCES}
    ${ZARYA_DNS_SOURCES}
    ${ZARYA_RUNTIME_SOURCES}
    ${ZARYA_IPC_SOURCES}
    ${ZARYA_HELPERCLIENT_SOURCES}
    ${ZARYA_KILLSWITCH_COMMON}
    ${ZARYA_KILLSWITCH_PLATFORM}
    ${ZARYA_KILLSWITCH_GUI_SOURCES}
    ${ZARYA_GEODATA_SOURCES}
    src/import/VlessUriParser.cpp
    ${ZARYA_PLATFORM_SOURCES}
    ${ZARYA_AUTOSTART_SOURCES}
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

target_compile_definitions(zarya PRIVATE ZARYA_VERSION_STRING="${ZARYA_VERSION_STRING}")

zarya_link_qt_widgets_app(zarya)
target_link_libraries(zarya PRIVATE Qt6::Network)

if(WIN32)
    set_target_properties(zarya PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_libraries(zarya PRIVATE wininet)
elseif(APPLE)
    set_target_properties(zarya PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER dev.vladon.zarya
        MACOSX_BUNDLE_BUNDLE_NAME Zarya
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${ZARYA_VERSION_STRING}
        MACOSX_BUNDLE_BUNDLE_VERSION ${ZARYA_VERSION_STRING}
    )
endif()

if(ZARYA_STATIC_QT AND WIN32)
    message(STATUS "zarya Release will be a single portable executable (Qt linked statically, /MT runtime)")
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ZaryaDeploy.cmake)

set(ZARYA_HELPER_SOURCES
    src/helper/main.cpp
    src/helper/HelperApplication.cpp
    src/helper/HelperCommandServer.cpp
    src/helper/HelperRuntimeManager.cpp
    src/helper/HelperAuth.cpp
    src/helper/HelperPathPolicy.cpp
    ${ZARYA_IPC_SOURCES}
    src/runtime/singbox/SingBoxTunSupportChecker.cpp
    src/platform/PlatformPrivilege.cpp
    src/platform/Platform.cpp
    src/storage/AppPaths.cpp
    src/storage/AppSettings.cpp
    src/runtime/RuntimeBackendType.cpp
    src/packaging/PackagingInfo.cpp
    ${ZARYA_KILLSWITCH_SOURCES}
)

if(WIN32)
    list(APPEND ZARYA_HELPER_SOURCES ${ZARYA_KILLSWITCH_WINDOWS_WFP_SOURCES})
    list(REMOVE_ITEM ZARYA_HELPER_SOURCES src/killswitch/windows/WindowsWfpKillSwitchManager_stub.cpp)
endif()

add_executable(zarya-helper ${ZARYA_HELPER_SOURCES})
target_include_directories(zarya-helper PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(zarya-helper PRIVATE ZARYA_VERSION_STRING="${ZARYA_VERSION_STRING}")
zarya_link_qt_core(zarya-helper)
target_link_libraries(zarya-helper PRIVATE Qt6::Network)
if(WIN32)
    target_link_libraries(zarya-helper PRIVATE Fwpuclnt Ws2_32 iphlpapi)
endif()

# Config generation checks (unit-like; uses shared or static Qt Core)
add_executable(zarya_xray_config_test
    tests/xray_config_test.cpp
    ${ZARYA_CORE_SOURCES}
    ${ZARYA_ROUTING_SOURCES}
    ${ZARYA_DNS_SOURCES}
    ${ZARYA_DOMAIN_SOURCES}
    src/storage/RoutingStore.cpp
    src/storage/DnsStore.cpp
    src/testing/TestStatus.cpp
    src/storage/ProfileStore.cpp
    src/storage/AppSettings.cpp
    src/storage/AppPaths.cpp
    src/runtime/RuntimeBackendType.cpp
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

add_executable(zarya_singbox_config_test
    tests/singbox_config_test.cpp
    src/runtime/ConfigWarning.cpp
    src/runtime/singbox/SingBoxRouteGenerator.cpp
    src/runtime/singbox/SingBoxDnsGenerator.cpp
    src/runtime/singbox/SingBoxRuleSetManager.cpp
    src/runtime/singbox/SingBoxConfigGenerator.cpp
    src/routing/RoutingGeoUtils.cpp
    src/dns/DnsGeoUtils.cpp
    ${ZARYA_DOMAIN_SOURCES}
    src/storage/AppPaths.cpp
    src/storage/AppSettings.cpp
    src/runtime/RuntimeBackendType.cpp
    src/platform/Platform.cpp
    src/killswitch/KillSwitchMode.cpp
    src/geodata/GeoDataDownloader.cpp
    src/packaging/PackagingInfo.cpp
    ${ZARYA_RULESET_SOURCES}
)

target_include_directories(zarya_singbox_config_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(zarya_singbox_config_test PRIVATE
    ZARYA_VERSION_STRING="${ZARYA_VERSION_STRING}"
)

zarya_link_qt_core(zarya_singbox_config_test)
target_link_libraries(zarya_singbox_config_test PRIVATE Qt6::Network)

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
    src/storage/AppSettings.cpp
    src/storage/AppPaths.cpp
    src/runtime/RuntimeBackendType.cpp
    src/platform/Platform.cpp
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
    ${ZARYA_DNS_SOURCES}
    ${ZARYA_DOMAIN_SOURCES}
    src/storage/RoutingStore.cpp
    src/storage/DnsStore.cpp
    src/storage/AppSettings.cpp
    src/storage/AppPaths.cpp
    src/runtime/RuntimeBackendType.cpp
    src/platform/Platform.cpp
)

target_include_directories(zarya_testing_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

zarya_link_qt_core(zarya_testing_test)
target_link_libraries(zarya_testing_test PRIVATE Qt6::Network)

add_executable(zarya_geodata_verifier_test
    tests/geodata_verifier_test.cpp
    src/geodata/GeoDataVerifier.cpp
    src/geodata/GeoDataFileStatus.cpp
)

target_include_directories(zarya_geodata_verifier_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

zarya_link_qt_core(zarya_geodata_verifier_test)
