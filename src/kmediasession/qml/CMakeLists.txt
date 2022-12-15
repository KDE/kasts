# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>

add_library(kmediasession-qmlplugin kmediasession-qmlplugin.cpp)
target_link_libraries(kmediasession-qmlplugin
    Qt::Qml
    KMediaSession
)

# make examples work without installation
if (NOT CMAKE_VERSION VERSION_LESS 3.14)
    set_property(TARGET kmediasession-qmlplugin PROPERTY LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/org/kde/kmediasession)
    file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/qmldir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/org/kde/kmediasession/qmldir COPY_ON_ERROR SYMBOLIC)
else()
    message(WARNING "CMake is too old, cannot setup QML plugins to work without installation")
endif()

install(TARGETS kmediasession-qmlplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/kmediasession)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/kmediasession)
