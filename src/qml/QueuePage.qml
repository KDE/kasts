/*
   SPDX-FileCopyrightText: 2021 (c) Bart De Vries <bart@mogwai.be>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

// Includes relevant modules used by the QML
import QtQuick 2.6
import QtQuick.Controls 2.0 as Controls
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.13 as Kirigami
import QtMultimedia 5.15
import org.kde.alligator 1.0

Kirigami.ScrollablePage {
    id: queuepage
    title: "Alligator Play Queue"
    Component {
        id: delegateComponent
        Kirigami.SwipeListItem {
            id: listItem
            contentItem: RowLayout {
                Kirigami.ListItemDragHandle {
                    listItem: listItem
                    listView: mainList
                    onMoveRequested: queueModel.moveRows(oldIndex, newIndex, 1)
                }

                Controls.Label {
                    Layout.fillWidth: true
                    height: Math.max(implicitHeight, Kirigami.Units.iconSizes.smallMedium)
                    text: model.entry.title
                    color: listItem.checked || (listItem.pressed && !listItem.checked && !listItem.sectionDelegate) ? listItem.activeTextColor : listItem.textColor
                }
            }
            actions: [
                Kirigami.Action {
                    iconName: "media-playback-start"
                    text: "Play"
                    onTriggered: {
                        audio.entry = model.entry
                        audio.play()
                    }
                },
                Kirigami.Action {
                    iconName: "delete"
                    text: i18n("Delete download")
                    onTriggered: model.entry.enclosure.deleteFile()
                    visible: model.entry.enclosure && model.entry.enclosure.status === Enclosure.Downloaded
                }
            ]
            onClicked: {
                pageStack.push("qrc:/EntryPage.qml", {"entry": model.entry})
            }
        }
    }

    ListView {
        id: mainList
        Timer {
            id: refreshRequestTimer
            interval: 3000
            onTriggered: page.refreshing = false
        }

        /*model: ListModel {
            id: listModel
            Component.onCompleted: {
                for (var i = 0; i < 200; ++i) {
                    listModel.append({"title": "Item " + i,
                        "actions": [{text: "Action 1", icon: "document-decrypt"},
                                    {text: "Action 2", icon: "mail-reply-sender"}]
                    })
                }
            }
        }*/
        model: QueueModel { id: queueModel }

        moveDisplaced: Transition {
            YAnimator {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }
        delegate: Kirigami.DelegateRecycler {
            width: parent ? parent.width : implicitWidth
            sourceComponent: delegateComponent
        }
    }
}

