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

ecm_qt_declare_logging_category(SRCS_base
    HEADER "audiologging.h"
    IDENTIFIER "kastsAudio"
    CATEGORY_NAME "org.kde.kasts.audio"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(SRCS_base
    HEADER "datamanagerlogging.h"
    IDENTIFIER "kastsDataManager"
    CATEGORY_NAME "org.kde.kasts.datamanager"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(SRCS_base
    HEADER "downloadprogressmodellogging.h"
    IDENTIFIER "kastsDownloadProgressModel"
    CATEGORY_NAME "org.kde.kasts.downloadprogressmodel"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(SRCS_base
    HEADER "enclosurelogging.h"
    IDENTIFIER "kastsEnclosure"
    CATEGORY_NAME "org.kde.kasts.enclosure"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(SRCS_base
    HEADER "feedlogging.h"
    IDENTIFIER "kastsFeed"
    CATEGORY_NAME "org.kde.kasts.feed"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(SRCS_base
    HEADER "fetcherlogging.h"
    IDENTIFIER "kastsFetcher"
    CATEGORY_NAME "org.kde.kasts.fetcher"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(SRCS_base
    HEADER "powermanagementinterfacelogging.h"
    IDENTIFIER "kastsPowerManagementInterface"
    CATEGORY_NAME "org.kde.kasts.powermanagementinterface"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(SRCS_base
    HEADER "queuemodellogging.h"
    IDENTIFIER "kastsQueueModel"
    CATEGORY_NAME "org.kde.kasts.queuemodel"
    DEFAULT_SEVERITY Info
)

if(ANDROID)
    set (SRCS ${SRCS_base}
         androidlogging.h)
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
target_link_libraries(kasts PRIVATE Qt::Core Qt::Qml Qt::Quick Qt::QuickControls2 Qt::Sql Qt::Multimedia KF5::Syndication KF5::CoreAddons KF5::ConfigGui KF5::I18n)

if(ANDROID)
    target_link_libraries(kasts PRIVATE
        KF5::Kirigami2
        Qt::Svg
        OpenSSL::SSL
        log
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
        checkbox
    )
else()
    target_link_libraries(kasts PRIVATE Qt::Widgets Qt::DBus)
endif()

install(TARGETS kasts ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
