project(Alligator)

cmake_minimum_required(VERSION 2.8.12)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
   message(FATAL_ERROR "This application requires an out of source build. Please create a separate build directory.")
endif()

include(FeatureSummary)

find_package(ECM 5.67.0 REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMPoQmTools)
include(KDECompilerSettings NO_POLICY_SCOPE)

find_package(Qt5 REQUIRED NO_MODULE COMPONENTS Core Quick Test Gui Svg QuickControls2 Sql)
find_package(KF5 REQUIRED COMPONENTS CoreAddons Syndication Config I18n)

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
