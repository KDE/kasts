/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQml.Models

import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kasts

Controls.ItemDelegate {
    id: root

    required property QtObject listView
    required property Feed feed
    required property int index
    required property int cardSize
    required property int cardMargin
    required property int row

    property int feedSorting: (Controls.ApplicationWindow.window as Main) ? (Controls.ApplicationWindow.window as Main).feedSorting : 0 // need to do this check because the window becomes null just before delegate destruction
    property var countProperty: (feedSorting === FeedsProxyModel.UnreadDescending || feedSorting === FeedsProxyModel.UnreadAscending) ? feed.unreadEntryCount : ((feedSorting === FeedsProxyModel.NewDescending || feedSorting === FeedsProxyModel.NewAscending) ? feed.newEntryCount : ((feedSorting === FeedsProxyModel.FavoriteDescending || feedSorting === FeedsProxyModel.FavoriteAscending) ? feed.favoriteEntryCount : 0))
    property int borderWidth: 1
    implicitWidth: root.cardSize + 2 * root.cardMargin
    implicitHeight: root.cardSize + 2 * root.cardMargin

    property bool selected: false
    property bool isCurrentItem: false // to restore currentItem when model is resorted
    property string currentItemUrl: ""
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
        selected = (root.listView as FeedListGridView).selectionModel.rowIntersectsSelection(row);
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
            color: root.checked || root.highlighted ? root.activeBackgroundColor : Kirigami.Theme.backgroundColor

            Rectangle {
                id: internal
                property bool indicateActiveFocus: root.pressed || Kirigami.Settings.tabletMode || root.activeFocus || (root.ListView.view ? root.ListView.view.activeFocus : false)
                anchors.fill: parent
                visible: !Kirigami.Settings.tabletMode && root.hoverEnabled
                color: root.activeBackgroundColor
                opacity: (root.hovered || root.highlighted || root.activeFocus) && !root.pressed ? 0.5 : 0
            }
        }

        Kirigami.ShadowedRectangle {
            anchors.fill: parent
            anchors.margins: root.cardMargin
            color: Kirigami.Theme.backgroundColor

            radius: Kirigami.Units.smallSpacing

            shadow.size: root.cardMargin
            shadow.color: Qt.rgba(0.0, 0.0, 0.0, 0.25)
            shadow.yOffset: root.borderWidth * 2

            border.width: root.borderWidth
            border.color: Qt.tint(Kirigami.Theme.textColor, Qt.rgba(color.r, color.g, color.b, 0.6))
        }
    }

    contentItem: MouseArea {
        id: mouseArea
        anchors.fill: parent
        anchors.margins: root.cardMargin + root.borderWidth
        implicitWidth: root.cardSize - 2 * root.borderWidth
        implicitHeight: root.cardSize - 2 * root.borderWidth

        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: mouse => {
            // Keep track of (currently) selected items
            var modelIndex = ((root.listView as FeedListGridView).model as FeedsProxyModel).index(root.index, 0);

            if ((root.listView as FeedListGridView).selectionModel.isSelected(modelIndex) && mouse.button == Qt.RightButton) {
                (root.listView as FeedListGridView).contextMenu.popup(null, mouse.x + 1, mouse.y + 1);
            } else if (mouse.modifiers & Qt.ShiftModifier) {
                // Have to take a detour through c++ since selecting large sets
                // in QML is extremely slow
                (root.listView as FeedListGridView).selectionModel.select((root.listView as FeedListGridView).model.createSelection(modelIndex.row, (root.listView as FeedListGridView).selectionModel.currentIndex.row), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
            } else if (mouse.modifiers & Qt.ControlModifier) {
                (root.listView as FeedListGridView).selectionModel.select(modelIndex, ItemSelectionModel.Toggle | ItemSelectionModel.Rows);
            } else if (mouse.button == Qt.LeftButton) {
                (root.listView as FeedListGridView).currentIndex = root.index;
                (root.listView as FeedListGridView).selectionModel.setCurrentIndex(modelIndex, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
                root.clicked();
            } else if (mouse.button == Qt.RightButton) {
                // This item is right-clicked, but isn't selected
                (root.listView as FeedListGridView).selectionForContextMenu = [modelIndex];
                (root.listView as FeedListGridView).contextMenu.popup(null, mouse.x + 1, mouse.y + 1);
            }
        }

        onPressAndHold: {
            var modelIndex = ((root.listView as FeedListGridView).model as FeedsProxyModel).index(root.index, 0);
            (root.listView as FeedListGridView).selectionModel.select(modelIndex, ItemSelectionModel.Toggle | ItemSelectionModel.Rows);
        }

        Connections {
            target: (root.listView as FeedListGridView).selectionModel
            function onSelectionChanged(): void {
                root.updateIsSelected();
            }
        }

        Connections {
            target: (root.listView as FeedListGridView).model as FeedsProxyModel
            function onLayoutAboutToBeChanged(): void {
                if (root.GridView.view.currentItem === root) {
                    root.isCurrentItem = true;
                    root.currentItemUrl = root.feed.url;
                } else {
                    root.isCurrentItem = false;
                    root.currentItemUrl = "";
                }
            }
            function onLayoutChanged(): void {
                root.updateIsSelected();
                if (root.isCurrentItem) {
                    // yet another hack because "index" is still giving the old
                    // value here; so we have to manually find the new index.
                    var mymodel = ((root.listView as FeedListGridView).model as FeedsProxyModel);
                    for (var i = 0; i < mymodel.rowCount(); i++) {
                        if (mymodel.data(mymodel.index(i, 0), FeedsModel.UrlRole) == root.currentItemUrl) {
                            (root.listView as FeedListGridView).currentIndex = i;
                        }
                    }
                }
            }
        }

        ImageWithFallback {
            id: img
            anchors.fill: parent
            imageSource: root.feed.cachedImage
            imageTitle: root.feed.name
            isLoading: root.feed.refreshing
            absoluteRadius: Kirigami.Units.smallSpacing - root.borderWidth
        }

        Rectangle {
            id: countRectangle
            visible: root.countProperty > 0
            anchors.top: img.top
            anchors.right: img.right
            width: Math.max(actionsButton.width, countLabel.width)
            height: actionsButton.height
            color: root.activeBackgroundColor
            radius: Kirigami.Units.smallSpacing - 2 * root.borderWidth
        }

        Controls.Label {
            id: countLabel
            visible: root.countProperty > 0
            anchors.centerIn: countRectangle
            anchors.margins: Kirigami.Units.smallSpacing
            text: root.countProperty
            font.bold: true
            color: Kirigami.Theme.highlightedTextColor
        }

        Rectangle {
            id: actionsRectangle
            anchors.fill: actionsButton
            color: "black"
            opacity: 0.5
            radius: Kirigami.Units.smallSpacing - 2 * root.borderWidth
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
        (Controls.ApplicationWindow.window as Main).lastFeed = root.feed.url;
        var appPageStack = (Controls.ApplicationWindow.window as Kirigami.ApplicationWindow).pageStack;
        if (appPageStack.depth > 1)
            appPageStack.pop();
        appPageStack.push(Qt.createComponent("org.kde.kasts", "FeedDetailsPage"), {
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

        title: root.feed.name

        actions: [
            Kirigami.Action {
                onTriggered: {
                    var appPageStack = (root.Controls.ApplicationWindow.window as Kirigami.ApplicationWindow).pageStack;
                    while (appPageStack.depth > 1)
                        appPageStack.pop();
                    appPageStack.push(Qt.createComponent("org.kde.kasts", "FeedDetailsPage"), {
                        feed: root.feed
                    });
                }
                icon.name: "documentinfo"
                text: KI18n.i18n("Podcast Details")
            },
            Kirigami.Action {
                onTriggered: {
                    var appPageStack = (root.Controls.ApplicationWindow.window as Kirigami.ApplicationWindow).pageStack;
                    if (root.feed.url === (root.Controls.ApplicationWindow.window as Main).lastFeed)
                        while (appPageStack.depth > 1)
                            appPageStack.pop();
                    DataManager.removeFeed(root.feed);
                }
                icon.name: "delete"
                text: KI18n.i18n("Remove Podcast")
            }
        ]
    }
}
