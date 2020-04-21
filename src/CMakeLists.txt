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
target_link_libraries(alligator Qt5::Core Qt5::Qml Qt5::Quick Qt5::Svg Qt5::Sql KF5::Syndication KF5::CoreAddons KF5::ConfigGui KF5::I18n)

install(TARGETS alligator ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
