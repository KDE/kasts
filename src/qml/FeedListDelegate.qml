/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQuick.Effects
import QtQml.Models

import org.kde.kirigami as Kirigami

import org.kde.kasts

Controls.ItemDelegate {
    id: feedDelegate

    property var countProperty: (kastsMainWindow.feedSorting === FeedsProxyModel.UnreadDescending || kastsMainWindow.feedSorting === FeedsProxyModel.UnreadAscending) ? feed.unreadEntryCount : ((kastsMainWindow.feedSorting === FeedsProxyModel.NewDescending || kastsMainWindow.feedSorting === FeedsProxyModel.NewAscending) ? feed.newEntryCount : ((kastsMainWindow.feedSorting === FeedsProxyModel.FavoriteDescending || kastsMainWindow.feedSorting === FeedsProxyModel.FavoriteAscending) ? feed.favoriteEntryCount : 0))
    property int cardSize: 0
    property int cardMargin: 0
    property int borderWidth: 1
    implicitWidth: cardSize + 2 * cardMargin
    implicitHeight: cardSize + 2 * cardMargin

    property QtObject listView: undefined
    property bool selected: false
    property bool isCurrentItem: false // to restore currentItem when model is resorted
    property string currentItemUrl: ""
    property int row: model ? model.row : -1
    property var activeBackgroundColor: Qt.lighter(Kirigami.Theme.highlightColor, 1.3)
    highlighted: selected

    Accessible.role: Accessible.Button
    Accessible.name: feed.name
    Accessible.onPressAction: {
        clicked();
    }
    Keys.onReturnPressed: clicked()

    // We need to update the "selected" status:
    // - if the selected indexes changes
    // - if our delegate moves
    // - if the model moves and the delegate stays in the same place
    function updateIsSelected(): void {
        selected = listView.selectionModel.rowIntersectsSelection(row);
    }

    onRowChanged: {
        updateIsSelected();
    }

    Component.onCompleted: {
        updateIsSelected();
    }

    background: Rectangle {
        // Background for highlighted / hovered / active items
        anchors.fill: parent

        Rectangle {
            id: background
            anchors.fill: parent
            color: feedDelegate.checked || feedDelegate.highlighted || (feedDelegate.supportsMouseEvents && feedDelegate.pressed) ? feedDelegate.activeBackgroundColor : Kirigami.Theme.backgroundColor

            Rectangle {
                id: internal
                property bool indicateActiveFocus: feedDelegate.pressed || Kirigami.Settings.tabletMode || feedDelegate.activeFocus || (feedDelegate.ListView.view ? feedDelegate.ListView.view.activeFocus : false)
                anchors.fill: parent
                visible: !Kirigami.Settings.tabletMode && feedDelegate.hoverEnabled
                color: feedDelegate.activeBackgroundColor
                opacity: (feedDelegate.hovered || feedDelegate.highlighted || feedDelegate.activeFocus) && !feedDelegate.pressed ? 0.5 : 0
            }
        }

        Kirigami.ShadowedRectangle {
            anchors.fill: parent
            anchors.margins: cardMargin
            color: Kirigami.Theme.backgroundColor

            radius: Kirigami.Units.smallSpacing

            shadow.size: cardMargin
            shadow.color: Qt.rgba(0.0, 0.0, 0.0, 0.25)
            shadow.yOffset: borderWidth * 2

            border.width: borderWidth
            border.color: Qt.tint(Kirigami.Theme.textColor, Qt.rgba(color.r, color.g, color.b, 0.6))
        }
    }

    contentItem: MouseArea {
        id: mouseArea
        anchors.fill: parent
        anchors.margins: cardMargin + borderWidth
        implicitWidth: cardSize - 2 * borderWidth
        implicitHeight: cardSize - 2 * borderWidth

        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: mouse => {
            // Keep track of (currently) selected items
            var modelIndex = feedDelegate.listView.model.index(index, 0);

            if (feedDelegate.listView.selectionModel.isSelected(modelIndex) && mouse.button == Qt.RightButton) {
                feedDelegate.listView.contextMenu.popup(null, mouse.x + 1, mouse.y + 1);
            } else if (mouse.modifiers & Qt.ShiftModifier) {
                // Have to take a detour through c++ since selecting large sets
                // in QML is extremely slow
                feedDelegate.listView.selectionModel.select(feedDelegate.listView.model.createSelection(modelIndex.row, feedDelegate.listView.selectionModel.currentIndex.row), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
            } else if (mouse.modifiers & Qt.ControlModifier) {
                feedDelegate.listView.selectionModel.select(modelIndex, ItemSelectionModel.Toggle | ItemSelectionModel.Rows);
            } else if (mouse.button == Qt.LeftButton) {
                feedDelegate.listView.currentIndex = index;
                feedDelegate.listView.selectionModel.setCurrentIndex(modelIndex, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
                feedDelegate.clicked();
            } else if (mouse.button == Qt.RightButton) {
                // This item is right-clicked, but isn't selected
                feedDelegate.listView.selectionForContextMenu = [modelIndex];
                feedDelegate.listView.contextMenu.popup(null, mouse.x + 1, mouse.y + 1);
            }
        }

        onPressAndHold: {
            var modelIndex = feedDelegate.listView.model.index(index, 0);
            feedDelegate.listView.selectionModel.select(modelIndex, ItemSelectionModel.Toggle | ItemSelectionModel.Rows);
        }

        Connections {
            target: listView.selectionModel
            function onSelectionChanged(): void {
                updateIsSelected();
            }
        }

        Connections {
            target: listView.model
            function onLayoutAboutToBeChanged(): void {
                if (feedList.currentItem === feedDelegate) {
                    isCurrentItem = true;
                    currentItemUrl = feed.url;
                } else {
                    isCurrentItem = false;
                    currentItemUrl = "";
                }
            }
            function onLayoutChanged(): void {
                updateIsSelected();
                if (isCurrentItem) {
                    // yet another hack because "index" is still giving the old
                    // value here; so we have to manually find the new index.
                    for (var i = 0; i < feedList.model.rowCount(); i++) {
                        if (feedList.model.data(feedList.model.index(i, 0), FeedsModel.UrlRole) == currentItemUrl) {
                            feedList.currentIndex = i;
                        }
                    }
                }
            }
        }

        ImageWithFallback {
            id: img
            anchors.fill: parent
            imageSource: feed.cachedImage
            imageTitle: feed.name
            isLoading: feed.refreshing
            absoluteRadius: Kirigami.Units.smallSpacing - borderWidth
        }

        Rectangle {
            id: countRectangle
            visible: countProperty > 0
            anchors.top: img.top
            anchors.right: img.right
            width: Math.max(actionsButton.width, countLabel.width)
            height: actionsButton.height
            color: feedDelegate.activeBackgroundColor
            radius: Kirigami.Units.smallSpacing - 2 * borderWidth

        }

        Controls.Label {
            id: countLabel
            visible: countProperty > 0
            anchors.centerIn: countRectangle
            anchors.margins: Kirigami.Units.smallSpacing
            text: countProperty
            font.bold: true
            color: Kirigami.Theme.highlightedTextColor
        }

        Rectangle {
            id: actionsRectangle
            anchors.fill: actionsButton
            color: "black"
            opacity: 0.5
            radius: Kirigami.Units.smallSpacing - 2 * borderWidth
        }

        Controls.Button {
            id: actionsButton
            anchors.right: img.right
            anchors.bottom: img.bottom
            anchors.margins: 0
            padding: 0
            flat: true
            icon.name: "overflow-menu"
            icon.color: "white"
            onClicked: actionOverlay.open()
        }
    }

    onClicked: {
        lastFeed = feed.url;
        if (pageStack.depth > 1)
            pageStack.pop();
        pageStack.push(Qt.createComponent("org.kde.kasts", "FeedDetailsPage"), {
            feed: feed
        });
    }

    Controls.ToolTip.visible: hovered
    Controls.ToolTip.text: feed.name
    Controls.ToolTip.delay: Kirigami.Units.toolTipDelay

    Kirigami.MenuDialog {
        id: actionOverlay
        // parent: applicationWindow().overlay
        showCloseButton: true

        title: feed.name

        actions: [
            Kirigami.Action {
                onTriggered: {
                    while (pageStack.depth > 1)
                        pageStack.pop();
                    pageStack.push(Qt.createComponent("org.kde.kasts", "FeedDetailsPage"), {
                        feed: feed
                    });
                }
                icon.name: "documentinfo"
                text: i18n("Podcast Details")
            },
            Kirigami.Action {
                onTriggered: {
                    if (feed.url === lastFeed)
                        while (pageStack.depth > 1)
                            pageStack.pop();
                    DataManager.removeFeed(feed);
                }
                icon.name: "delete"
                text: i18n("Remove Podcast")
            }
        ]
    }
}
