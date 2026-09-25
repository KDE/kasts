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
import org.kde.ki18n

import org.kde.kasts

Kirigami.ScrollablePage {
    id: root

    title: KI18n.i18nc("@title of page allowing to search for new podcasts online", "Discover")

    header: Controls.Control {
        padding: Kirigami.Units.largeSpacing

        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        Kirigami.Theme.inherit: false

        background: Rectangle {
            color: Kirigami.Theme.backgroundColor

            Kirigami.Separator {
                anchors {
                    left: parent.left
                    bottom: parent.bottom
                    right: parent.right
                }
            }
        }

        contentItem: RowLayout {
            Kirigami.SearchField {
                id: textField
                placeholderText: KI18n.i18nc("@label:textbox Placeholder text for search field", "Search podcastindex.org…")
                Layout.fillWidth: true
                Keys.onReturnPressed: {
                    searchButton.clicked();
                }
            }

            Controls.Button {
                id: searchButton
                text: Utils.isWidescreen ? KI18n.i18n("Search") : ""
                icon.name: "search"
                onClicked: {
                    podcastSearchModel.search(textField.text);
                }
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

            required property string url
            required property string name
            required property string image
            required property string link
            required property string description
            required property string author
            required property date lastUpdated
            required property var model

            text: name

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
                    text: enabled ? KI18n.i18n("Subscribe") : KI18n.i18n("Subscribed")
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
                (Controls.ApplicationWindow.window as Main).pageStack.push(Qt.createComponent("org.kde.kasts", "FeedDetailsPage"), {
                    feed: subscribeAction.enabled ? DataManager.getFeed(url, name, image, link, description, author, lastUpdated) : DataManager.getFeed(DataManager.findFeeduids([listItem.url])[0]),
                    subscribeAction: subscribeAction,
                    showMoreInfo: true
                });
            }
        }
    }
}
