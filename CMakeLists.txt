cmake_minimum_required(VERSION 3.21)
project(WinRoute VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network LinguistTools)

set(NETFILTER_SDK_DIR ${CMAKE_SOURCE_DIR}/netfilter/nfsdk CACHE PATH "NetFilter SDK root (nfsdk folder)")
# Several drivers ship, the installer lets the user pick one: the NetFilter SDK
# demo driver (default) and the release drivers "A", "B", each as
# <driver>/netfilter2.sys under NETFILTER_DRIVER_DIR. All are WFP x64 builds,
# which serve every Windows version WinRoute runs on. They live only under
# drivers/<driver>/; there is no stray copy next to WinRoute.exe.
set(NETFILTER_DRIVER_DIR ${CMAKE_SOURCE_DIR}/netfilter/driver CACHE PATH "Drivers, as <driver>/netfilter2.sys")
set(NETFILTER_DRIVERS demo a b)
set(NETFILTER_API_DLL ${NETFILTER_SDK_DIR}/wfp/bin/release_c_api/x64/nfapi.dll)

# WinDivert is the alternative backend: its headers and x64 binaries are not in
# the repository (see README); place them under windivert/ or point here. Only
# the headers are needed to build; the DLL and .sys are copied next to the exe
# and loaded at run time (the backend is optional).
set(WINDIVERT_DIR ${CMAKE_SOURCE_DIR}/windivert CACHE PATH "WinDivert root (include/ and x64/)")

set(CORE_SOURCES
    src/core/Profile.h            src/core/Profile.cpp
    src/core/RuleEngine.h         src/core/RuleEngine.cpp
    src/core/ProcessInfo.h        src/core/ProcessInfo.cpp
    src/core/FlowTable.h          src/core/FlowTable.cpp
    src/core/DnsMessage.h         src/core/DnsMessage.cpp
    src/core/FakeDns.h            src/core/FakeDns.cpp
    src/core/DnsClient.h          src/core/DnsClient.cpp
    src/core/ProxyConnector.h     src/core/ProxyConnector.cpp
    src/core/Relay.h              src/core/Relay.cpp
    src/core/UdpRelaySession.h    src/core/UdpRelaySession.cpp
    src/core/Interceptor.h
    src/core/NetFilterApi.h       src/core/NetFilterApi.cpp
    src/core/NetFilterInterceptor.h src/core/NetFilterInterceptor.cpp
    src/core/WinDivertApi.h       src/core/WinDivertApi.cpp
    src/core/WinDivertPacket.h    src/core/WinDivertPacket.cpp
    src/core/WinDivertInterceptor.h src/core/WinDivertInterceptor.cpp
    src/core/CodeSignature.h      src/core/CodeSignature.cpp
    src/core/Engine.h             src/core/Engine.cpp
    src/core/Logger.h             src/core/Logger.cpp
    src/core/Utils.h              src/core/Utils.cpp
)

set(UI_SOURCES
    src/ui/MainWindow.h           src/ui/MainWindow.cpp
    src/ui/ConnectionsModel.h     src/ui/ConnectionsModel.cpp
    src/ui/TrafficWidget.h        src/ui/TrafficWidget.cpp
    src/ui/StatisticsWidget.h     src/ui/StatisticsWidget.cpp
    src/ui/ProxyServersDialog.h   src/ui/ProxyServersDialog.cpp
    src/ui/ProxyServerDialog.h    src/ui/ProxyServerDialog.cpp
    src/ui/ChainTypeDialog.h      src/ui/ChainTypeDialog.cpp
    src/ui/RulesDialog.h          src/ui/RulesDialog.cpp
    src/ui/RuleDialog.h           src/ui/RuleDialog.cpp
    src/ui/NameResolutionDialog.h src/ui/NameResolutionDialog.cpp
    src/ui/AdvancedDialogs.h      src/ui/AdvancedDialogs.cpp
    src/ui/ProxyCheckerDialog.h   src/ui/ProxyCheckerDialog.cpp
    src/ui/Icons.h                src/ui/Icons.cpp
    src/ui/Language.h             src/ui/Language.cpp
    src/ui/Updater.h              src/ui/Updater.cpp
    src/ui/DriverDialog.h         src/ui/DriverDialog.cpp
)

set(WINROUTE_HOMEPAGE_URL "https://pages.yanlei.org/kaguya/winroute/" CACHE STRING "WinRoute's homepage")
# The update information the homepage's web server serves.
set(WINROUTE_UPDATE_URL "${WINROUTE_HOMEPAGE_URL}update.json" CACHE STRING "WinRoute's update information")

set(TS_FILES
    translations/winroute_zh_CN.ts
    translations/winroute_ja.ts
)

add_library(winroute_core STATIC ${CORE_SOURCES})
target_include_directories(winroute_core PUBLIC src ${NETFILTER_SDK_DIR}/wfp/include ${NETFILTER_SDK_DIR}/include ${WINDIVERT_DIR}/include)
target_compile_definitions(winroute_core PUBLIC UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)
target_link_libraries(winroute_core PUBLIC Qt6::Core Qt6::Network ws2_32 iphlpapi psapi advapi32 shell32 crypt32 version)

option(WINROUTE_BUILD_GUI "Build the WinRoute application" ON)
# Development builds can drop the requireAdministrator manifest to test the UI unelevated.
option(WINROUTE_REQUIRE_ADMIN "Embed a manifest that requires administrator rights" ON)
if(WINROUTE_BUILD_GUI)
qt_add_executable(WinRoute WIN32
    src/main.cpp
    ${UI_SOURCES}
    resources/app.rc
)
target_link_libraries(WinRoute PRIVATE winroute_core Qt6::Widgets)
target_compile_definitions(WinRoute PRIVATE WINROUTE_VERSION="${PROJECT_VERSION}" WINROUTE_UPDATE_URL="${WINROUTE_UPDATE_URL}")
# Run by the updater once WinRoute has quit (src/ui/Updater.cpp).
qt_add_resources(WinRoute "update" PREFIX "/" BASE resources FILES resources/update.ps1)
if(WINROUTE_REQUIRE_ADMIN)
    target_compile_definitions(WinRoute PRIVATE WINROUTE_REQUIRE_ADMIN)
endif()

# Translations: "cmake --build build --target update_translations" refreshes the .ts files
# from the sources of both targets; the compiled .qm files are embedded under :/i18n.
qt_add_lupdate(TS_FILES ${TS_FILES} SOURCE_TARGETS WinRoute winroute_core LUPDATE_TARGET update_translations
               OPTIONS -no-obsolete -locations none)
qt_add_lrelease(TS_FILES ${TS_FILES} QM_FILES_OUTPUT_VARIABLE WINROUTE_QM_FILES)
set(QT_TRANSLATIONS_DIR ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_TRANSLATIONS})
set(QT_QM_FILES ${QT_TRANSLATIONS_DIR}/qtbase_zh_CN.qm ${QT_TRANSLATIONS_DIR}/qtbase_ja.qm)
foreach(qm ${WINROUTE_QM_FILES} ${QT_QM_FILES})
    get_filename_component(qm_name ${qm} NAME)
    set_source_files_properties(${qm} PROPERTIES QT_RESOURCE_ALIAS ${qm_name})
endforeach()
qt_add_resources(WinRoute "translations" PREFIX "/i18n" FILES ${WINROUTE_QM_FILES} ${QT_QM_FILES})

# Copy nfapi.dll next to the exe, and every driver to drivers/<key>/ so the app
# can install one and "Tools > Network Driver" can switch between them.
set(POSTBUILD_DRIVER_COMMANDS)
foreach(driver ${NETFILTER_DRIVERS})
    list(APPEND POSTBUILD_DRIVER_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:WinRoute>/drivers/${driver}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${NETFILTER_DRIVER_DIR}/${driver}/netfilter2.sys $<TARGET_FILE_DIR:WinRoute>/drivers/${driver})
endforeach()
add_custom_command(TARGET WinRoute POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${NETFILTER_API_DLL} $<TARGET_FILE_DIR:WinRoute>
    ${POSTBUILD_DRIVER_COMMANDS}
)

# WinDivert.dll and its driver are loaded at run time from next to the exe. Copy
# them when present; the backend is optional, so a missing WinDivert is not fatal.
if(EXISTS ${WINDIVERT_DIR}/x64/WinDivert.dll)
    add_custom_command(TARGET WinRoute POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WINDIVERT_DIR}/x64/WinDivert.dll
                ${WINDIVERT_DIR}/x64/WinDivert64.sys $<TARGET_FILE_DIR:WinRoute>)
endif()

# "cmake --build build --target installer" stages a deployed copy and builds
# build/WinRoute-<version>-setup.exe with Inno Setup 6.
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS})
find_program(ISCC_EXECUTABLE ISCC PATHS "C:/Program Files (x86)/Inno Setup 6" "C:/Program Files/Inno Setup 6")
if(WINDEPLOYQT_EXECUTABLE AND ISCC_EXECUTABLE)
    set(INSTALLER_STAGE ${CMAKE_BINARY_DIR}/stage)
    # Each driver goes to stage/drivers/<driver>/netfilter2.sys.
    set(STAGE_DRIVER_COMMANDS)
    foreach(driver ${NETFILTER_DRIVERS})
        list(APPEND STAGE_DRIVER_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E make_directory ${INSTALLER_STAGE}/drivers/${driver}
            COMMAND ${CMAKE_COMMAND} -E copy ${NETFILTER_DRIVER_DIR}/${driver}/netfilter2.sys ${INSTALLER_STAGE}/drivers/${driver})
    endforeach()
    add_custom_target(installer
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${INSTALLER_STAGE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${INSTALLER_STAGE}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:WinRoute> ${NETFILTER_API_DLL}
                ${WINDIVERT_DIR}/x64/WinDivert.dll ${WINDIVERT_DIR}/x64/WinDivert64.sys ${INSTALLER_STAGE}
        ${STAGE_DRIVER_COMMANDS}
        COMMAND ${WINDEPLOYQT_EXECUTABLE} --release --no-translations --no-system-d3d-compiler --no-opengl-sw
                ${INSTALLER_STAGE}/WinRoute.exe
        COMMAND ${ISCC_EXECUTABLE} /Q /DAppVersion=${PROJECT_VERSION}
                /DStageDir=${INSTALLER_STAGE} /DOutputDir=${CMAKE_BINARY_DIR}
                ${CMAKE_SOURCE_DIR}/installer/WinRoute.iss
        DEPENDS WinRoute
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/installer
        COMMENT "Building the WinRoute installer"
        VERBATIM)
else()
    message(STATUS "windeployqt or Inno Setup 6 (ISCC) not found: the installer target is not available")
endif()
endif()

# Tests for the platform-independent pieces (profile IO, rule matching, DNS, proxy protocols).
enable_testing()
add_executable(winroute_tests tests/tests.cpp)
target_link_libraries(winroute_tests PRIVATE winroute_core)
add_test(NAME winroute_tests COMMAND winroute_tests)
