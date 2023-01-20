/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.15

import org.kde.kirigami 2.12 as Kirigami

import org.kde.kasts 1.0

Kirigami.ScrollablePage {
    id: page

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    property QtObject feed;
    property bool isSubscribed: true
    property var subscribeAction: undefined // this is only used if instantiated from the discoverpage

    property string author: isSubscribed ? (page.feed.authors.length === 0 ? "" : page.feed.authors[0].name) : feed.author
    property bool showMoreInfo: false

    title: i18n("Podcast Details")

    supportsRefreshing: true

    onRefreshingChanged: {
        if (refreshing) {
            updateFeed.run()
        }
    }

    // Overlay dialog box showing options what to do on metered connections
    ConnectionCheckAction {
        id: updateFeed

        function action() {
            feed.refresh()
        }

        function abortAction() {
            page.refreshing = false
        }
    }

    // Make sure that this feed is also showing as "refreshing" on FeedListPage
    Connections {
        target: feed
        function onRefreshingChanged(refreshing) {
            if(!refreshing)
                page.refreshing = refreshing
        }
    }

    // add the default actions through onCompleted to add them to the ones
    // defined above
    Component.onCompleted: {
        for (var i in entryList.defaultActionList) {
            contextualActions.push(entryList.defaultActionList[i]);
        }
    }

    Component {
        id: entryListDelegate
        GenericEntryDelegate {
            listView: entryList
        }
    }

    ListModel {
        id: emptyListModel
    }

    GenericEntryListView {
        id: entryList
        reuseItems: true
        currentIndex: -1

        model: page.feed.entries ? page.feed.entries : emptyListModel
        delegate: entryListDelegate

        // OverlayHeader looks nicer, but seems completely broken when flicking the list
        //headerPositioning: ListView.OverlayHeader

        header: ColumnLayout {
            id: headerColumn
            height: (isSubscribed && entryList.count > 0) ? implicitHeight : entryList.height
            width: entryList.width
            spacing: 0

            property real headerOverlayProgress: Math.min(1, Math.abs(entryList.contentY) / headerColumn.height)

            Kirigami.Theme.inherit: false
            Kirigami.Theme.colorSet: Kirigami.Theme.Window

            GenericHeader {
                id: headerImage
                Layout.fillWidth: true

                image: isSubscribed ? feed.cachedImage : feed.image
                title: isSubscribed ? feed.name : feed.title
                subtitle: (!page.feed.authors || page.feed.authors.length === 0) ? "" : i18nc("by <author(s)>", "by %1", page.feed.authors[0].name)
            }

            // header actions
            Controls.Control {
                Layout.fillWidth: true

                leftPadding: Kirigami.Units.largeSpacing
                rightPadding: Kirigami.Units.largeSpacing
                bottomPadding: Kirigami.Units.smallSpacing
                topPadding: Kirigami.Units.smallSpacing

                background: Rectangle {
                    color: Kirigami.Theme.alternateBackgroundColor
                }

                contentItem: Kirigami.ActionToolBar {
                    alignment: Qt.AlignLeft
                    background: Item {}

                    // HACK: ActionToolBar loads buttons dynamically, and so the height calculation
                    // changes the position
                    onHeightChanged: entryList.contentY = entryList.originY

                    actions: [
                        Kirigami.Action {
                            visible: isSubscribed
                            iconName: "view-refresh"
                            text: i18n("Refresh Podcast")
                            onTriggered: page.refreshing = true
                        },
                        Kirigami.Action {
                            iconName: "kt-add-feeds"
                            text: enabled ? i18n("Subscribe") : i18n("Subscribed")
                            enabled: !DataManager.feedExists(feed.url)
                            visible: !isSubscribed
                            onTriggered: {
                                DataManager.addFeed(feed.url);
                                enabled = false;
                                // Also disable button on discoverpage
                                if (subscribeAction !== undefined) {
                                    subscribeAction.enabled = false;
                                }
                            }
                        },
                        Kirigami.Action {
                            iconName: "documentinfo"
                            text: i18n("Show Details")
                            checkable: true
                            onCheckedChanged: {
                                showMoreInfo = checked;
                            }
                        }
                    ]
                }
            }

            Kirigami.Separator { Layout.fillWidth: true }

            // podcast description
            Controls.Control {
                Layout.fillHeight: !isSubscribed
                Layout.fillWidth: true
                leftPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
                rightPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
                topPadding: Kirigami.Units.largeSpacing
                bottomPadding: Kirigami.Units.largeSpacing

                // HACK: opening more info changes the position of the header
                onHeightChanged: entryList.contentY = entryList.originY

                background: Rectangle {
                    color: Kirigami.Theme.backgroundColor
                }

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Controls.Label {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        textFormat: page.showMoreInfo ? TextEdit.RichText : Text.StyledText
                        maximumLineCount: page.showMoreInfo ? undefined : 2
                        elide: Text.ElideRight
                        text: feed.description
                        font.pointSize: Kirigami.Theme.defaultFont.pointSize
                        wrapMode: Text.WordWrap
                        color: Kirigami.Theme.textColor
                        lineHeight: 1.2
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredHeight: Math.max(feedUrlLayout.height, feedUrlCopyButton.width)
                        visible: page.showMoreInfo
                        height: visible ? implicitHeight : 0
                        RowLayout {
                            id: feedUrlLayout
                            anchors.left: parent.left
                            anchors.right: feedUrlCopyButton.left
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: Kirigami.Units.smallSpacing
                            TextEdit {
                                Layout.alignment: Qt.AlignTop
                                readOnly: true
                                textFormat:TextEdit.RichText
                                text: i18n("Podcast URL:")
                                wrapMode: TextEdit.Wrap
                                color: Kirigami.Theme.textColor
                            }
                            TextEdit {
                                id: feedUrl
                                Layout.alignment: Qt.AlignTop
                                readOnly: true
                                selectByMouse: !Kirigami.Settings.isMobile
                                textFormat:TextEdit.RichText
                                text: "<a href='%1'>%1</a>".arg(feed.url)
                                wrapMode: TextEdit.Wrap
                                Layout.fillWidth: true
                                color: Kirigami.Theme.textColor
                            }
                        }
                        Controls.Button {
                            id: feedUrlCopyButton
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.leftMargin: Kirigami.Units.smallSpacing
                            icon.name: "edit-copy"

                            onClicked: {
                                feedUrl.selectAll();
                                feedUrl.copy();
                                feedUrl.deselect();
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        visible: page.showMoreInfo
                        height: visible ? implicitHeight : 0
                        spacing: Kirigami.Units.smallSpacing
                        TextEdit {
                            Layout.alignment: Qt.AlignTop
                            readOnly: true
                            textFormat: TextEdit.RichText
                            text: i18n("Weblink:")
                            wrapMode: TextEdit.Wrap
                            color: Kirigami.Theme.textColor
                        }

                        TextEdit {
                            readOnly: true
                            Layout.alignment: Qt.AlignTop
                            selectByMouse: !Kirigami.Settings.isMobile
                            textFormat:TextEdit.RichText
                            text: "<a href='%1'>%1</a>".arg(feed.link)
                            onLinkActivated: Qt.openUrlExternally(link)
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            color: Kirigami.Theme.textColor
                        }
                    }
                    TextEdit {
                        Layout.alignment: Qt.AlignTop
                        Layout.fillWidth: true
                        visible: isSubscribed && page.showMoreInfo
                        height: visible ? implicitHeight : 0

                        readOnly: true
                        selectByMouse: !Kirigami.Settings.isMobile
                        textFormat:TextEdit.RichText
                        text: isSubscribed ? i18n("Subscribed since: %1", feed.subscribed.toLocaleString(Qt.locale(), Locale.ShortFormat)) : ""
                        wrapMode: Text.WordWrap
                        color: Kirigami.Theme.textColor
                    }
                    TextEdit {
                        Layout.alignment: Qt.AlignTop
                        Layout.fillWidth: true
                        visible: isSubscribed && page.showMoreInfo
                        height: visible ? implicitHeight : 0

                        readOnly: true
                        selectByMouse: !Kirigami.Settings.isMobile
                        textFormat:TextEdit.RichText
                        text: isSubscribed ? i18n("Last Updated: %1", feed.lastUpdated.toLocaleString(Qt.locale(), Locale.ShortFormat)) : ""
                        wrapMode: Text.WordWrap
                        color: Kirigami.Theme.textColor
                    }
                    TextEdit {
                        Layout.alignment: Qt.AlignTop
                        Layout.fillWidth: true
                        visible: isSubscribed && page.showMoreInfo
                        height: visible ? implicitHeight : 0

                        readOnly: true
                        selectByMouse: !Kirigami.Settings.isMobile
                        textFormat:TextEdit.RichText
                        text: i18np("1 Episode", "%1 Episodes", feed.entryCount) + ", " + i18np("1 Unplayed", "%1 Unplayed", feed.unreadEntryCount)
                        wrapMode: Text.WordWrap
                        color: Kirigami.Theme.textColor
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            Kirigami.Separator { Layout.fillWidth: true }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
                height: visible ? implicitHeight : 0
                visible: entryList.count === 0 && isSubscribed

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent

                    width: Kirigami.Units.gridUnit * 20

                    text: feed.errorId === 0 ? i18n("No Episodes Available") : i18n("Error (%1): %2", feed.errorId, feed.errorString)
                    icon.name: feed.errorId === 0 ? "" : "data-error"
                }
            }
        }
    }
}
