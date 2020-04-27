project(Alligator)

cmake_minimum_required(VERSION 2.8.12)

set(KF5_MIN_VERSION "5.70.0")
set(QT_MIN_VERSION "5.10.0")

include(FeatureSummary)

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(KDEInstallDirs)
include(KDEClangFormat)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Core Quick Test Gui QuickControls2 Sql)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS CoreAddons Syndication Config I18n)

if (ANDROID)
    find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Svg)
    find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Kirigami2)
else()
    find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Widgets)
endif()

add_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_URL_CAST_FROM_STRING)
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)
add_definitions(-DQT_USE_QSTRINGBUILDER)
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050d00)

install(PROGRAMS org.kde.mobile.alligator.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.mobile.alligator.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES alligator.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
