# SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
# SPDX-License-Identifier: BSD-2-Clause


set(SRCS_base
    main.cpp
    feedsmodel.cpp
    entriesmodel.cpp
    fetcher.cpp
    database.cpp
    entry.cpp
    feed.cpp
    author.cpp
    enclosure.cpp
    enclosuredownloadjob.cpp
    queuemodel.cpp
    episodemodel.cpp
    downloadprogressmodel.cpp
    datamanager.cpp
    audiomanager.cpp
    powermanagementinterface.cpp
    errorlogmodel.cpp
    error.h
    mpris2/mpris2.cpp
    resources.qrc
)

if(ANDROID)
    set (SRCS ${SRCS_base})
else()
    set (SRCS ${SRCS_base}
         mpris2/mediaplayer2.cpp
         mpris2/mediaplayer2player.cpp
    )

    qt5_add_dbus_interface(SRCS dbus-interfaces/org.freedesktop.PowerManagement.Inhibit.xml inhibitinterface)
    qt5_add_dbus_interface(SRCS dbus-interfaces/org.gnome.SessionManager.xml gnomesessioninterface)
endif()

add_executable(kasts ${SRCS})

kconfig_add_kcfg_files(kasts settingsmanager.kcfgc GENERATE_MOC)

target_include_directories(kasts PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(kasts PRIVATE Qt5::Core Qt5::Qml Qt5::Quick Qt5::QuickControls2 Qt5::Sql Qt5::Multimedia KF5::Syndication KF5::CoreAddons KF5::ConfigGui KF5::I18n)

if(ANDROID)
    target_link_libraries(kasts PRIVATE
        KF5::Kirigami2
        Qt5::Svg
        OpenSSL::SSL
    )

    kirigami_package_breeze_icons(ICONS
        dialog-close
        delete
        settings-configure
        help-about-symbolic
        list-add
        list-remove
        view-refresh
        kasts
        mail-sent
        globe
        data-error
        rss
        document-import
        document-export
        document-open-folder
        edit-delete-remove
        edit-clear-all
        download
        media-playlist-append
        media-seek-backward
        media-seek-forward
        media-skip-forward
        media-playback-start
        media-playback-pause
        source-playlist
        arrow-down
        overflow-menu
    )
else()
    target_link_libraries(kasts PRIVATE Qt5::Widgets Qt5::DBus)
endif()

install(TARGETS kasts ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
