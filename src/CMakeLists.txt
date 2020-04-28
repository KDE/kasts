set(alligator_SRCS
    main.cpp
    feedListModel.cpp
    entryListModel.cpp
    fetcher.cpp
    database.cpp
    resources.qrc
)

kconfig_add_kcfg_files(alligator_SRCS alligatorsettings.kcfgc GENERATE_MOC)

add_executable(alligator ${alligator_SRCS})
target_link_libraries(alligator PRIVATE Qt5::Core Qt5::Qml Qt5::Quick Qt5::Sql KF5::Syndication KF5::CoreAddons KF5::ConfigGui KF5::I18n)

if(ANDROID)
    target_link_libraries(alligator PRIVATE
        KF5::Kirigami2
        Qt5::Svg
        OpenSSL::SSL
    )

    kirigami_package_breeze_icons(ICONS
        bookmark-remove
        bookmark-new
        delete
        settings-configure
        help-about
        list-add
        view-refresh
    )
else()
    target_link_libraries(alligator PRIVATE Qt5::Widgets)
endif()

install(TARGETS alligator ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
