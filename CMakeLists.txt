# SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
# SPDX-FileCopyrightText: 2020 Nicolas Fella <nicolas.fella@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.16)

project(Kasts)
set(PROJECT_VERSION "22.11")

# be c++17 compliant
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(KF5_MIN_VERSION "5.101.0")
set(QT_MIN_VERSION "5.15.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)

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
if(NOT ANDROID)
    include(KDEClangFormat)
endif()

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX KASTS
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/kasts-version.h
)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Core Quick Test Gui QuickControls2 Sql Multimedia Svg)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS CoreAddons Kirigami2 Syndication Config I18n ThreadWeaver)
find_package(KF5KirigamiAddons 0.6 REQUIRED)
find_package(Taglib REQUIRED)
find_package(Qt${QT_MAJOR_VERSION}Keychain)
set_package_properties(Qt${QT_MAJOR_VERSION}Keychain PROPERTIES
    TYPE REQUIRED
    PURPOSE "Secure storage of account secrets"
)

find_package(KF5 ${KF5_MIN_VERSION} OPTIONAL_COMPONENTS NetworkManagerQt)

if (ANDROID)
    find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS Svg AndroidExtras)
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
                -DQT_DISABLE_DEPRECATED_BEFORE=0x050d00
)

ki18n_install(po)

install(PROGRAMS org.kde.kasts.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kasts.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES kasts.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/android/version.gradle.in ${CMAKE_BINARY_DIR}/version.gradle)

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)


if (NOT ANDROID)
    # inside if-statement to work around problems with gitlab Android CI
    file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h)
    kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
    kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
endif()
