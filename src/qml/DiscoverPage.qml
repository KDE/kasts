/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.delegates as AddonDelegates
import org.kde.kirigamiaddons.components as Addons
import org.kde.ki18n

import org.kde.kasts

Kirigami.ScrollablePage {
    id: root

    title: KI18n.i18nc("@title of page allowing to search for new podcasts online", "Discover")

    readonly property list<AbstractPodcastSearchModel> searchModels: [
        PodcastIndexSearchModel {},
        ItunesSearchModel {}
    ]

    property AbstractPodcastSearchModel podcastSearchModel: searchModels[0]

    readonly property Component searchProviderMenuComponent: Component {
        Addons.ConvergentContextMenu {
            id: searchProviderMenu

            // property Controls.ActionGroup searchProviderGroup: Controls.ActionGroup {}

            readonly property Instantiator instantiator: Instantiator {
                model: root.searchModels

                delegate: Kirigami.Action {
                    required property AbstractPodcastSearchModel modelData

                    text: modelData.name
                    checkable: true
                    autoExclusive: true
                    checked: modelData.name == root.podcastSearchModel.name
                    onTriggered: {
                        root.podcastSearchModel = Qt.binding(() => {
                            return modelData;
                        });
                    }
                }

                onObjectAdded: (index, object) => {
                    searchProviderMenu.actions.push(object as Kirigami.Action);
                }
            }
        }
    }

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

        contentItem: Kirigami.SearchField {
            id: textField
            Layout.fillWidth: true
            placeholderText: root.podcastSearchModel.placeholderText
            focus: true
            autoAccept: false
            onAccepted: {
                root.podcastSearchModel.search(textField.text);
            }

            Kirigami.Action {
                id: searchSettingsButton
                icon.name: "settings-configure"
                text: KI18n.i18nc("@action:intoolbar", "Select Search Provider")

                onTriggered: {
                    const item = root.searchProviderMenuComponent.createObject(root.Controls.Overlay.overlay);
                    (item as Addons.ConvergentContextMenu).popup();
                }
            }

            Component.onCompleted: {
                var origAction = rightActions[0];
                rightActions[0] = searchSettingsButton;
                rightActions.push(origAction);
            }
        }
    }

    Component.onCompleted: {
        textField.forceActiveFocus();
    }

    ListView {
        id: listView
        reuseItems: true
        model: root.podcastSearchModel

        delegate: AddonDelegates.RoundedItemDelegate {
            id: listItem

            required property string url
            required property string name
            required property string image
            required property string link
            required property string description
            required property string author
            required property date lastUpdated

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
