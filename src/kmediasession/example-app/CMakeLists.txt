# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Core Qml Gui Multimedia)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS CoreAddons I18n)

if (NOT ANDROID)
    find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS DBus Widgets)
endif()

###########################################################

add_executable(kmediasession-example main.cpp resources.qrc)

target_link_libraries(kmediasession-example PRIVATE
    Qt::Core
    Qt::Qml
    Qt::Gui
    KF5::CoreAddons
    KF5::I18n
    KMediaSession
)

if (NOT ANDROID)
    target_link_libraries(kmediasession-example PRIVATE Qt::Widgets)
endif()

install(TARGETS kmediasession-example ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

