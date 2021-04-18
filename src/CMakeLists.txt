add_executable(alligator
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
    datamanager.cpp
    audiomanager.cpp
    powermanagementinterface.cpp
    mpris2/mpris2.cpp
    mpris2/mediaplayer2.cpp
    mpris2/mediaplayer2player.cpp
    resources.qrc
)

kconfig_add_kcfg_files(alligator settingsmanager.kcfgc GENERATE_MOC)

target_include_directories(alligator PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(alligator PRIVATE Qt5::Core Qt5::Qml Qt5::Quick Qt5::QuickControls2 Qt5::Sql Qt5::Multimedia Qt5::DBus KF5::Syndication KF5::CoreAddons KF5::ConfigGui KF5::I18n)

if(ANDROID)
    target_link_libraries(alligator PRIVATE
        KF5::Kirigami2
        Qt5::Svg
        OpenSSL::SSL
    )

    kirigami_package_breeze_icons(ICONS
        delete
        settings-configure
        help-about-symbolic
        list-add
        list-remove
        view-refresh
        alligator
        mail-sent
        globe
        data-error
        rss
        document-import
        document-export
        document-open-folder
        edit-delete-remove
        download
        media-playlist-append
        media-seek-backward
        media-seek-forward
        media-skip-forward
        media-playback-start
        media-playback-pause
        source-playlist
        arrow-down
    )
else()
    target_link_libraries(alligator PRIVATE Qt5::Widgets)
endif()

install(TARGETS alligator ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
