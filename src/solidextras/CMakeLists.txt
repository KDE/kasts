# SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(qml)

set(solidextras_srcs
    networkstatus.cpp
)

if (ANDROID)
    add_subdirectory(android)
    list(APPEND solidextras_srcs
        networkstatus_android.cpp
    )
else()
    list(APPEND solidextras_srcs
        networkstatus_dbus.cpp
    )
    qt_add_dbus_interface(solidextras_srcs org.freedesktop.portal.NetworkMonitor.xml portalnetworkmonitor)
endif()

add_library(KastsSolidExtras ${solidextras_srcs})
generate_export_header(KastsSolidExtras BASE_NAME KastsSolidExtras)

if (TARGET KF5::NetworkManagerQt)
    set(HAVE_NM ON)
endif()
configure_file(config-solid-extras.h.in ${CMAKE_CURRENT_BINARY_DIR}/config-solid-extras.h)

target_include_directories(KastsSolidExtras PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>")
target_link_libraries(KastsSolidExtras PUBLIC Qt::Core)

if (ANDROID)
    target_link_libraries(KastsSolidExtras PRIVATE Qt::AndroidExtras)
else ()
    target_link_libraries(KastsSolidExtras PRIVATE Qt::DBus)
    if (TARGET KF5::NetworkManagerQt)
        target_link_libraries(KastsSolidExtras PRIVATE KF5::NetworkManagerQt)
    endif()
endif()

install(TARGETS KastsSolidExtras ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
