cmake_minimum_required(VERSION 3.14)

project(Alligator)

set(KF5_MIN_VERSION "5.75.0")
set(QT_MIN_VERSION "5.15.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(FeatureSummary)
include(ECMSetupVersion)
include(KDEInstallDirs)
include(KDEClangFormat)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

ecm_setup_version(0.1.0
    VARIABLE_PREFIX ALLIGATOR
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/alligator-version.h
)

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Core Quick Test Gui QuickControls2 Sql)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS CoreAddons Syndication Config I18n)

if (ANDROID)
    find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Svg)
    find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Kirigami2)
    find_package(OpenSSL REQUIRED)
else()
    find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Widgets)
endif()

add_definitions(-DQT_NO_CAST_FROM_ASCII
                -DQT_NO_CAST_TO_ASCII
                -DQT_NO_URL_CAST_FROM_STRING
                -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
                -DQT_USE_QSTRINGBUILDER
                -DQT_DISABLE_DEPRECATED_BEFORE=0x050d00
)

install(PROGRAMS org.kde.alligator.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.alligator.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES alligator.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
