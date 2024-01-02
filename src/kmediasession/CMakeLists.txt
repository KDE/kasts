# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${ECM_MODULE_PATH})

option(BUILD_EXAMPLE_PLAYER "Build and install example audio backend player" OFF)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Core Qml Multimedia)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS CoreAddons I18n)

if (NOT ANDROID)
    find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS DBus Widgets)
endif()

find_package(LIBVLC)
set_package_properties(LIBVLC PROPERTIES
    DESCRIPTION "Allows to play audio using libVLC as backend"
    URL "https://www.videolan.org/vlc/libvlc.html"
    TYPE RECOMMENDED)

option(BUILD_GSTREAMER_BACKEND "Build gstreamer audio backend" ON)
if (BUILD_GSTREAMER_BACKEND)
    find_package(PkgConfig REQUIRED) #finding pkg-config is a helper tool
    #using pkg-config to getting Gstreamer
    pkg_check_modules(GSTREAMER IMPORTED_TARGET gstreamer-1.0)
endif()

#########################################

add_library(KMediaSession
    kmediasession.cpp
    metadata.cpp
    mediabackends/abstractmediabackend.h
    mediabackends/qtmediabackend.cpp
    powermanagement/powermanagementinterface.cpp
    mpris2/mpris2.cpp
)

qt_policy(SET QTP0001 NEW)
qt_add_qml_module(KMediaSession URI org.kde.kmediasession
    OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/org/kde/kmediasession
)

ecm_qt_declare_logging_category(KMediaSession
    HEADER "kmediasessionlogging.h"
    IDENTIFIER "KMediaSessionLog"
    CATEGORY_NAME "org.kde.kmediasession.kmediasession"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(KMediaSession
    HEADER "powermanagementinterfacelogging.h"
    IDENTIFIER "KMediaSessionPowerManagementInterface"
    CATEGORY_NAME "org.kde.kmediasession.powermanagementinterface"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(KMediaSession
    HEADER "qtmediabackendlogging.h"
    IDENTIFIER "QtMediaBackendLog"
    CATEGORY_NAME "org.kde.kmediasession.qtmediabackend"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(KMediaSession
    HEADER "mpris2logging.h"
    IDENTIFIER "Mpris2Log"
    CATEGORY_NAME "org.kde.kmediasession.mpris2"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(KMediaSession
    HEADER "metadatalogging.h"
    IDENTIFIER "MetaDataLog"
    CATEGORY_NAME "org.kde.kmediasession.metadata"
    DEFAULT_SEVERITY Info
)

if(ANDROID)
    target_sources(KMediaSession PRIVATE androidlogging.h)
else()
    set(KMediaSession_dbus_srcs "")
    qt_add_dbus_interface(KMediaSession_dbus_srcs dbus-interfaces/org.freedesktop.PowerManagement.Inhibit.xml inhibitinterface)
    qt_add_dbus_interface(KMediaSession_dbus_srcs dbus-interfaces/org.gnome.SessionManager.xml gnomesessioninterface)
    target_sources(KMediaSession PRIVATE
        mpris2/mediaplayer2.cpp
        mpris2/mediaplayer2player.cpp
        ${KMediaSession_dbus_srcs}
    )
endif()

if (LIBVLC_FOUND)
    ecm_qt_declare_logging_category(KMediaSession
        HEADER "vlcmediabackendlogging.h"
        IDENTIFIER "VlcMediaBackendLog"
        CATEGORY_NAME "org.kde.kmediasession.vlcmediabackend"
        DEFAULT_SEVERITY Info
    )
    ecm_qt_declare_logging_category(KMediaSession
        HEADER "vlcsignalslogging.h"
        IDENTIFIER "VlcSignalsLog"
        CATEGORY_NAME "org.kde.kmediasession.vlcsignals"
        DEFAULT_SEVERITY Info
    )
    target_sources(KMediaSession PRIVATE
        mediabackends/vlcmediabackend.cpp
    )
    set(HAVE_LIBVLC ON)
endif()

if (GSTREAMER_FOUND)
    ecm_qt_declare_logging_category(KMediaSession
        HEADER "gstmediabackendlogging.h"
        IDENTIFIER "GstMediaBackendLog"
        CATEGORY_NAME "org.kde.kmediasession.gstmediabackend"
        DEFAULT_SEVERITY Info
    )
    ecm_qt_declare_logging_category(KMediaSession
        HEADER "gstsignalslogging.h"
        IDENTIFIER "GstSignalsLog"
        CATEGORY_NAME "org.kde.kmediasession.gstsignals"
        DEFAULT_SEVERITY Info
    )
    target_sources(KMediaSession PRIVATE
        mediabackends/gstmediabackend.cpp
    )
    set(HAVE_GST ON)
endif()

configure_file(config-kmediasession.h.in ${CMAKE_CURRENT_BINARY_DIR}/config-kmediasession.h)

generate_export_header(KMediaSession BASE_NAME KMediaSession)

target_include_directories(KMediaSession PUBLIC ${CMAKE_BINARY_DIR})

target_link_libraries(KMediaSession PUBLIC Qt::Core)
target_link_libraries(KMediaSession PRIVATE
    Qt::Multimedia
    KF6::CoreAddons
    KF6::I18n
)

if (LIBVLC_FOUND)
    target_link_libraries(KMediaSession PRIVATE LIBVLC::LIBVLC)
endif()

if (GSTREAMER_FOUND)
    target_link_libraries(KMediaSession PRIVATE PkgConfig::GSTREAMER)
endif()

if (ANDROID)
else()
    target_link_libraries(KMediaSession PRIVATE Qt::DBus)
endif()

install(TARGETS KMediaSession ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

if (BUILD_EXAMPLE_PLAYER)
    add_subdirectory(example-app)
endif()
