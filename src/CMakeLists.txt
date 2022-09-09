# SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
# SPDX-License-Identifier: BSD-2-Clause

add_subdirectory(solidextras)

set(SRCS_base
    main.cpp
    about.cpp
    fetcher.cpp
    database.cpp
    entry.cpp
    feed.cpp
    author.cpp
    enclosure.cpp
    datamanager.cpp
    audiomanager.cpp
    error.cpp
    enclosuredownloadjob.cpp
    storagemanager.cpp
    storagemovejob.cpp
    updatefeedjob.cpp
    fetchfeedsjob.cpp
    models/chaptermodel.cpp
    models/feedsmodel.cpp
    models/feedsproxymodel.cpp
    models/entriesmodel.cpp
    models/queuemodel.cpp
    models/episodemodel.cpp
    models/episodeproxymodel.cpp
    models/downloadmodel.cpp
    models/errorlogmodel.cpp
    models/podcastsearchmodel.cpp
    mpris2/mpris2.cpp
    powermanagementinterface.cpp
    sync/sync.cpp
    sync/syncjob.cpp
    sync/syncutils.cpp
    sync/gpodder/gpodder.cpp
    sync/gpodder/genericrequest.cpp
    sync/gpodder/loginrequest.cpp
    sync/gpodder/logoutrequest.cpp
    sync/gpodder/devicerequest.cpp
    sync/gpodder/syncrequest.cpp
    sync/gpodder/updatesyncrequest.cpp
    sync/gpodder/updatedevicerequest.cpp
    sync/gpodder/subscriptionrequest.cpp
    sync/gpodder/uploadsubscriptionrequest.cpp
    sync/gpodder/episodeactionrequest.cpp
    sync/gpodder/uploadepisodeactionrequest.cpp
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
    HEADER "enclosurelogging.h"
    IDENTIFIER "kastsEnclosure"
    CATEGORY_NAME "org.kde.kasts.enclosure"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(SRCS_base
    HEADER "entrylogging.h"
    IDENTIFIER "kastsEntry"
    CATEGORY_NAME "org.kde.kasts.entry"
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
    HEADER "synclogging.h"
    IDENTIFIER "kastsSync"
    CATEGORY_NAME "org.kde.kasts.sync"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(SRCS_base
    HEADER "models/downloadmodellogging.h"
    IDENTIFIER "kastsDownloadModel"
    CATEGORY_NAME "org.kde.kasts.downloadmodel"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(SRCS_base
    HEADER "models/queuemodellogging.h"
    IDENTIFIER "kastsQueueModel"
    CATEGORY_NAME "org.kde.kasts.queuemodel"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(SRCS_base
    HEADER "powermanagementinterfacelogging.h"
    IDENTIFIER "kastsPowerManagementInterface"
    CATEGORY_NAME "org.kde.kasts.powermanagementinterface"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(SRCS_base
    HEADER "storagemanagerlogging.h"
    IDENTIFIER "kastsStorageManager"
    CATEGORY_NAME "org.kde.kasts.storagemanager"
    DEFAULT_SEVERITY Info
)

ecm_qt_declare_logging_category(SRCS_base
    HEADER "storagemovejoblogging.h"
    IDENTIFIER "kastsStorageMoveJob"
    CATEGORY_NAME "org.kde.kasts.storagemovejob"
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

    qt_add_dbus_interface(SRCS dbus-interfaces/org.freedesktop.PowerManagement.Inhibit.xml inhibitinterface)
    qt_add_dbus_interface(SRCS dbus-interfaces/org.gnome.SessionManager.xml gnomesessioninterface)
endif()

set(kasts_ICONS_PNG
    ../icons/16-apps-kasts.png
    ../icons/24-apps-kasts.png
    ../icons/32-apps-kasts.png
    ../icons/48-apps-kasts.png
    ../icons/64-apps-kasts.png
    ../icons/128-apps-kasts.png
)

# add icons to application sources, to have them bundled
ecm_add_app_icon(SRCS ICONS ${kasts_ICONS_PNG})

add_executable(kasts ${SRCS})

kconfig_add_kcfg_files(kasts settingsmanager.kcfgc GENERATE_MOC)

target_include_directories(kasts PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(kasts PRIVATE Qt::Core Qt::Qml Qt::Quick Qt::QuickControls2 Qt::Sql Qt::Multimedia KF5::Syndication KF5::CoreAddons KF5::ConfigGui KF5::I18n Taglib::Taglib KastsSolidExtras ${QTKEYCHAIN_LIBRARIES} KF5::ThreadWeaver)

if(ANDROID)
    target_link_libraries(kasts PRIVATE
        KF5::Kirigami2
        Qt::Svg
        OpenSSL::SSL
        log
    )
    if(SQLite3_FOUND)
        target_link_libraries(kasts PRIVATE SQLite::SQLite3)
    endif()

    if(ZLIB_FOUND)
        target_link_libraries(kasts PRIVATE ZLIB::ZLIB)
    endif()

    kirigami_package_breeze_icons(ICONS
        window-close
        window-close-symbolic
        delete
        settings-configure
        help-about-symbolic
        tools-report-bug
        list-add
        list-remove
        view-refresh
        view-filter
        kasts
        mail-sent
        globe
        data-error
        rss
        bookmarks
        document-import
        document-export
        document-open-folder
        edit-delete-remove
        edit-clear-all
        edit-select-all
        edit-select-none
        edit-copy
        download
        media-playlist-append
        media-seek-backward
        media-seek-forward
        media-skip-forward
        media-playback-start
        media-playback-pause
        view-media-playlist
        source-playlist
        arrow-down
        go-next
        overflow-menu
        checkbox
        error
        search
        kt-add-feeds
        state-sync
        network-connect
        drive-harddisk-symbolic
        dialog-ok
        dialog-cancel
        computer
        computer-laptop
        network-server-database
        smartphone
        emblem-music-symbolic
        gpodder
        kaccounts-nextcloud
        clock
        viewimage
    )
else()
    target_link_libraries(kasts PRIVATE Qt::Widgets Qt::DBus)
endif()

install(TARGETS kasts ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
