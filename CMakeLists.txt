# SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
# SPDX-FileCopyrightText: 2020 Nicolas Fella <nicolas.fella@gmx.de>
# SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.16)

# KDE Applications version, managed by release script.
set(RELEASE_SERVICE_VERSION_MAJOR "23")
set(RELEASE_SERVICE_VERSION_MINOR "04")
set(RELEASE_SERVICE_VERSION_MICRO "0")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(kasts VERSION ${RELEASE_SERVICE_VERSION})

# be c++17 compliant
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(KF_MIN_VERSION "5.102.0")
set(QT_MIN_VERSION "5.15.2")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(FeatureSummary)
include(ECMSetupVersion)
include(ECMQtDeclareLoggingCategory)
include(ECMGenerateExportHeader)
include(KDEInstallDirs)
include(KDEGitCommitHooks)
include(ECMFindQmlModule)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMAddAppIcon)
include(ECMDeprecationSettings)
if(NOT ANDROID)
    include(KDEClangFormat)
endif()

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX KASTS
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/kasts-version.h
)

ecm_set_disabled_deprecation_versions(
    QT 5.15.2
    KF 5.102.0
)

if (QT_MAJOR_VERSION EQUAL "6")
    set(KF_MAJOR_VERSION "6")
else()
    set(KF_MAJOR_VERSION "5")
endif()

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Core Quick Test Gui QuickControls2 Sql Svg Xml)
find_package(KF${KF_MAJOR_VERSION} ${KF_MIN_VERSION} REQUIRED COMPONENTS I18n CoreAddons Kirigami2 Syndication Config ThreadWeaver)
if (NOT KF_MAJOR_VERSION EQUAL "6") # TODO temporary workaround until kirigami-addons is switched to KF6
find_package(KF5KirigamiAddons 0.7 REQUIRED)
endif()
find_package(Taglib REQUIRED)
find_package(Qt${QT_MAJOR_VERSION}Keychain)
set_package_properties(Qt${QT_MAJOR_VERSION}Keychain PROPERTIES
    TYPE REQUIRED
    PURPOSE "Secure storage of account secrets"
)

find_package(KF${KF_MAJOR_VERSION} ${KF_MIN_VERSION} OPTIONAL_COMPONENTS NetworkManagerQt)

if (ANDROID)
    find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS Svg)
    if (QT_MAJOR_VERSION EQUAL "5")
        find_package(Qt5 REQUIRED COMPONENTS AndroidExtras)
    endif()
    find_package(OpenSSL REQUIRED)
    find_package(SQLite3)
    find_package(ZLIB)
    find_package(Gradle)
else()
    find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS Widgets DBus)
endif()

add_definitions(-DQT_NO_CAST_FROM_ASCII
                -DQT_NO_CAST_TO_ASCII
                -DQT_NO_URL_CAST_FROM_STRING
                -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
                -DQT_USE_QSTRINGBUILDER
)

ki18n_install(po)

install(PROGRAMS org.kde.kasts.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kasts.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES kasts.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)
install(FILES icons/kasts-tray-dark.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)
install(FILES icons/kasts-tray-light.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)
install(FILES icons/media-playback-start-cloud.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/actions)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/android/version.gradle.in ${CMAKE_BINARY_DIR}/version.gradle)

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

if (NOT ANDROID)
    # inside if-statement to work around problems with gitlab Android CI
    file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h)
    kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
    kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
endif()
