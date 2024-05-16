# SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
# SPDX-FileCopyrightText: 2020 Nicolas Fella <nicolas.fella@gmx.de>
# SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.16)

# KDE Applications version, managed by release script.
set(RELEASE_SERVICE_VERSION_MAJOR "24")
set(RELEASE_SERVICE_VERSION_MINOR "05")
set(RELEASE_SERVICE_VERSION_MICRO "0")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(kasts VERSION ${RELEASE_SERVICE_VERSION})

# be c++17 compliant
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(KF_MIN_VERSION "6.0.0")
set(QT_MIN_VERSION "6.4.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

set(KDE_COMPILERSETTINGS_LEVEL 6.0)

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
include(ECMAddAndroidApk)
if(NOT ANDROID)
    include(KDEClangFormat)
endif()

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX KASTS
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/kasts-version.h
)

ecm_set_disabled_deprecation_versions(
    QT 6.0.0
    KF 5.240.0
)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Core Quick Test Gui QuickControls2 Sql Svg Xml Network)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS I18n CoreAddons Kirigami Syndication Config ThreadWeaver ColorScheme)

find_package(KF6 ${KF_MIN_VERSION} QUIET COMPONENTS BreezeIcons)

find_package(Taglib REQUIRED)
find_package(Qt6Keychain)
set_package_properties(Qt6Keychain PROPERTIES
    TYPE REQUIRED
    PURPOSE "Secure storage of account secrets"
)

if (ANDROID)
    find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Svg)
    find_package(OpenSSL REQUIRED)
    find_package(SQLite3)
    find_package(ZLIB)
    find_package(Gradle)
else()
    find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Widgets DBus)
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
install(FILES icons/media-playback-cloud.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/actions)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/android/version.gradle.in ${CMAKE_BINARY_DIR}/version.gradle)

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

if (NOT ANDROID)
    # inside if-statement to work around problems with gitlab Android CI
    file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h)
    kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
    kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
endif()
