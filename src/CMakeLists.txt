set(alligator_SRCS
    main.cpp
    feedListModel.cpp
    entryListModel.cpp
    fetcher.cpp
    feed.cpp
    entry.cpp
    resources.qrc
    )

add_executable(alligator ${alligator_SRCS})
target_link_libraries(alligator Qt5::Core Qt5::Qml Qt5::Quick Qt5::Svg Qt5::Sql KF5::Syndication)

install(TARGETS alligator ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
