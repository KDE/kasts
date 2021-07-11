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
            Keys.onReturnPressed: {
                searchButton.clicked()
            }
            Controls.Popup {
                id: historyPopup
                x: 0
                y: parent.height + Kirigami.Units.smallSpacing
                width: parent.width
                visible: view.count > 0 && textField.activeFocus
                padding: Kirigami.Units.smallSpacing
                height: Math.min(Kirigami.Units.gridUnit * 10, view.contentHeight)
                contentItem: Controls.ScrollView {
                    Controls.ScrollBar.horizontal.policy: Controls.ScrollBar.AlwaysOff
                    ListView {
                        id: view
                        currentIndex: 0
                        keyNavigationWraps: true
                        model: SearchHistoryModel
                        delegate: Kirigami.BasicListItem {
                            text: search.searchTerm
                            onClicked: {
                                textField.text = text;
                            }
                        }
                    }
                }
            }
            Keys.onUpPressed: view.decrementCurrentIndex()
            Keys.onDownPressed: view.incrementCurrentIndex()
        }
        Controls.Button {
            id: searchButton
            text: isWidescreen ? i18n("Search") : ""
            icon.name: "search"
            Layout.rightMargin: Kirigami.Units.smallSpacing
            onClicked: {
                if(textField.text.length > 0) {
                    podcastSearchModel.search(textField.text);
                    SearchHistoryModel.insertSearchResult(textField.text);
                    historyPopup.close();
                }
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
                    text: enabled ? i18n("Subscribe") : i18n("Subscribed")
                    icon.name: "kt-add-feeds"
                    enabled: !DataManager.isFeedExists(model.url)
                    onTriggered: {
                        DataManager.addFeed(model.url)
                    }
                }
            ]
            onClicked: {
                pageStack.push("qrc:/FeedDetailsPage.qml", {"feed": model, isSubscribed: false})
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
}
