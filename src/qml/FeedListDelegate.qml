/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.15
import QtQml.Models 2.15

import org.kde.kirigami 2.19 as Kirigami

import org.kde.kasts 1.0

Controls.ItemDelegate {
    id: feedDelegate

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
    function updateIsSelected() {
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
        Rectangle {
            id: background
            anchors.fill: parent
            color: feedDelegate.checked || feedDelegate.highlighted || (feedDelegate.supportsMouseEvents && feedDelegate.pressed)
                ? feedDelegate.activeBackgroundColor
                : Kirigami.Theme.backgroundColor

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
            anchors.leftMargin: cardMargin
            color: Kirigami.Theme.backgroundColor

            radius: Kirigami.Units.smallSpacing

            shadow.size: Kirigami.Units.largeSpacing
            shadow.color: Qt.rgba(0.0, 0.0, 0.0, 0.15)
            shadow.yOffset: borderWidth * 2

            border.width: borderWidth
            border.color: Qt.tint(Kirigami.Theme.textColor, Qt.rgba(color.r, color.g, color.b, 0.6))
        }
    }

    contentItem: MouseArea {
        id: mouseArea
        anchors.fill: parent
        anchors.margins: cardMargin + borderWidth
        anchors.leftMargin: cardMargin + borderWidth
        implicitWidth:  cardSize - 2 * borderWidth
        implicitHeight: cardSize  - 2 * borderWidth

        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            // Keep track of (currently) selected items
            var modelIndex = feedDelegate.listView.model.index(index, 0);

            if (feedDelegate.listView.selectionModel.isSelected(modelIndex) && mouse.button == Qt.RightButton) {
                feedDelegate.listView.contextMenu.popup(null, mouse.x+1, mouse.y+1);
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
                feedDelegate.listView.contextMenu.popup(null, mouse.x+1, mouse.y+1);
            }
        }

        onPressAndHold: {
            var modelIndex = feedDelegate.listView.model.index(index, 0);
            feedDelegate.listView.selectionModel.select(modelIndex, ItemSelectionModel.Toggle | ItemSelectionModel.Rows);
        }

        Connections {
            target: listView.selectionModel
            function onSelectionChanged() {
                updateIsSelected();
            }
        }

        Connections {
            target: listView.model
            function onLayoutAboutToBeChanged() {
                if (feedList.currentItem === feedDelegate) {
                    isCurrentItem = true;
                    currentItemUrl = feed.url;
                } else {
                    isCurrentItem = false;
                    currentItemUrl = "";
                }
            }
            function onLayoutChanged() {
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
        }

        Rectangle {
            id: countRectangle
            visible: feed.unreadEntryCount > 0
            anchors.top: img.top
            anchors.right: img.right
            width: actionsButton.width
            height: actionsButton.height
            color: feedDelegate.activeBackgroundColor
            radius: Kirigami.Units.smallSpacing - 2 * borderWidth
        }

        Controls.Label {
            id: countLabel
            visible: feed.unreadEntryCount > 0
            anchors.centerIn: countRectangle
            anchors.margins: Kirigami.Units.smallSpacing
            text: feed.unreadEntryCount
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

        // Rounded edges
        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: Item {
                width: img.width
                height: img.height
                Rectangle {
                    anchors.centerIn: parent
                    width: img.adapt ? img.width : Math.min(img.width, img.height)
                    height: img.adapt ? img.height : width
                    radius: Kirigami.Units.smallSpacing - borderWidth
                }
            }
        }
    }

    onClicked: {
        lastFeed = feed.url
        if (pageStack.depth >  1)
            pageStack.pop();
        pageStack.push("qrc:/EntryListPage.qml", {"feed": feed})
    }

    Controls.ToolTip {
        text: feed.name
        delay: Qt.styleHints.mousePressAndHoldInterval
        y: cardSize + cardMargin
    }

    Kirigami.MenuDialog {
        id: actionOverlay
        // parent: applicationWindow().overlay
        showCloseButton: true

        title: feed.name

        actions: [
            Kirigami.Action {
                onTriggered: {
                    while (pageStack.depth > 1)
                        pageStack.pop()
                    pageStack.push("qrc:/FeedDetailsPage.qml", {"feed": feed});
                    actionOverlay.close();
                }
                iconName: "help-about-symbolic"
                text: i18n("Podcast Details")
            },
            Kirigami.Action {
                onTriggered: {
                    if (feed.url === lastFeed)
                        while(pageStack.depth > 1)
                            pageStack.pop()
                    DataManager.removeFeed(feed)
                    actionOverlay.close();
                }
                iconName: "delete"
                text: i18n("Remove Podcast")
            }
        ]
    }
}
