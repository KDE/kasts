/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.delegates as AddonDelegates

import org.kde.kasts

Kirigami.ScrollablePage {
    id: page
    title: i18nc("@title of page allowing to search for new podcasts online", "Discover")
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

    ListView {
        id: listView
        reuseItems: true

        model: PodcastSearchModel {
            id: podcastSearchModel
        }

        delegate: AddonDelegates.RoundedItemDelegate {
            id: listItem

            required property string title
            required property string image
            required property string url
            required property var model

            text: title

            contentItem: RowLayout {
                ImageWithFallback {
                    imageSource: listItem.image
                    Layout.fillHeight: true
                    Layout.maximumHeight: Kirigami.Units.iconSizes.huge
                    Layout.preferredWidth: height
                    absoluteRadius: Kirigami.Units.smallSpacing
                }

                AddonDelegates.DefaultContentItem {
                    itemDelegate: listItem
                }

                Controls.ToolButton {
                    id: subscribeAction
                    text: enabled ? i18n("Subscribe") : i18n("Subscribed")
                    icon.name: "kt-add-feeds"
                    display: Controls.ToolButton.IconOnly
                    enabled: !DataManager.feedExists(listItem.url)

                    Controls.ToolTip.text: text
                    Controls.ToolTip.visible: hovered
                    Controls.ToolTip.delay: Kirigami.Units.toolTipDelay

                    onClicked: {
                        DataManager.addFeed(listItem.url);
                        enabled = false;
                    }
                }
            }

            Keys.onReturnPressed: clicked()

            onClicked: {
                pageStack.push("qrc:/qt/qml/org/kde/kasts/qml/FeedDetailsPage.qml", {
                    feed: subscribeAction.enabled ? listItem.model : DataManager.getFeed(listItem.url),
                    isSubscribed: !subscribeAction.enabled,
                    subscribeAction: subscribeAction,
                    showMoreInfo: true
                })
            }
        }
    }
}
