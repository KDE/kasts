/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.15 as Kirigami
import org.kde.kasts 1.0

Kirigami.ScrollablePage {
    id: page
    title: i18n("Discover")
    property var feedModel: ""
    header: RowLayout {
        width: parent.width
        anchors.topMargin: Kirigami.Units.smallSpacing
        Kirigami.SearchField {
            id: textField
            placeholderText: i18n("Search podcastindex.org")
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.smallSpacing
        }
        Controls.Button {
            id: searchButton
            text: isWidescreen ? i18n("Search") : ""
            icon.name: "search"
            Layout.rightMargin: Kirigami.Units.smallSpacing
            onClicked: {
                podcastSearchModel.search(textField.text);
            }
        }
    }
    Component.onCompleted: {
        textField.forceActiveFocus();
    }
    Component {
        id: delegateComponent
        Kirigami.SwipeListItem {
            id: listItem
            contentItem: RowLayout {
                Kirigami.Icon {
                    source: model.image
                    Layout.fillHeight: true
                    Layout.maximumHeight: Kirigami.Units.iconSizes.huge
                    Layout.preferredWidth: height
                }
                Controls.Label {
                    Layout.fillWidth: true
                    height: Math.max(implicitHeight, Kirigami.Units.iconSizes.smallMedium)
                    text: model.title
                    elide: Text.ElideRight
                    color: listItem.checked || (listItem.pressed && !listItem.checked && !listItem.sectionDelegate) ? listItem.activeTextColor : listItem.textColor
                }
            }
            actions: [
                Kirigami.Action {
                    text: enabled ? i18n("Subscribe") : i18n("Subscribed")
                    icon.name: "kt-add-feeds"
                    enabled: !DataManager.isFeedExists(model.url)
                    onTriggered: {
                        DataManager.addFeed(model.url)
                    }
                }
            ]
            onClicked: {
                feedModel = model
                detailDrawer.open();
            }
        }
    }
    ListView {
        anchors.fill: parent
        model: PodcastSearchModel {
            id: podcastSearchModel
        }
        spacing: 5
        clip: true
        delegate: Kirigami.DelegateRecycler {
            width: parent ? parent.width : implicitWidth
            sourceComponent: delegateComponent
        }
    }
    Kirigami.OverlaySheet {
        id: detailDrawer
        showCloseButton: true
        contentItem: ColumnLayout {
            Layout.preferredWidth:  Kirigami.Units.gridUnit * 25
            GenericHeader {
                image: feedModel.image
                title: feedModel.title
                subtitle: feedModel.author
                Controls.Button {
                    text: enabled ? "Subscribe" : "Subscribed"
                    icon.name: "kt-add-feeds"
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.rightMargin: Kirigami.Units.largeSpacing
                    anchors.topMargin: Kirigami.Units.largeSpacing
                    onClicked: {
                        DataManager.addFeed(feedModel.url)
                    }
                    enabled: !DataManager.isFeedExists(feedModel.url)
                }
            }
            Controls.Label {
                text: i18n("%1 \n\nAuthor: %2 \nOwner: %3", feedModel.description, feedModel.author,  feedModel.ownerName)
                Layout.margins: Kirigami.Units.gridUnit
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                onLinkActivated: Qt.openUrlExternally(link)
                font.pointSize: SettingsManager && !(SettingsManager.articleFontUseSystem) ? SettingsManager.articleFontSize : Kirigami.Units.fontMetrics.font.pointSize
            }
        }
    }
}
