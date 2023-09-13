/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kasts

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
            Keys.onReturnPressed: {
                searchButton.clicked()
            }
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
            alwaysVisibleActions: true
            separatorVisible: true
            contentItem: RowLayout {
                ImageWithFallback {
                    imageSource: model.image
                    Layout.fillHeight: true
                    Layout.maximumHeight: Kirigami.Units.iconSizes.huge
                    Layout.preferredWidth: height
                    fractionalRadius: 1.0 / 8.0
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
                    id: subscribeAction
                    text: enabled ? i18n("Subscribe") : i18n("Subscribed")
                    icon.name: "kt-add-feeds"
                    enabled: !DataManager.feedExists(model.url)
                    onTriggered: {
                        DataManager.addFeed(model.url);
                        enabled = false;
                    }
                }
            ]
            onClicked: {
                pageStack.push("qrc:/FeedDetailsPage.qml", {"feed": subscribeAction.enabled ? model : DataManager.getFeed(model.url), "isSubscribed": !subscribeAction.enabled, "subscribeAction": subscribeAction, "showMoreInfo": true})
            }
        }
    }
    ListView {
        id: listView
        anchors.fill: parent
        reuseItems: true

        model: PodcastSearchModel {
            id: podcastSearchModel
        }
        spacing: 5
        clip: true
        delegate: delegateComponent
    }
}
