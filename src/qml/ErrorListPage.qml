/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.15
import QtMultimedia 5.15
import org.kde.kirigami 2.15 as Kirigami

import org.kde.alligator 1.0


Kirigami.ScrollablePage {

    padding: 0

    Kirigami.PlaceholderMessage {
        visible: errorList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: i18n("No errors logged")
    }

    Component {
        id: errorListDelegate
        Kirigami.SwipeListItem {
            contentItem: RowLayout {
                Kirigami.Icon {
                    source: "data-error"
                    property int size: Kirigami.Units.iconSizes.medium
                    Layout.minimumHeight: size
                    Layout.maximumHeight: size
                    Layout.minimumWidth: size
                    Layout.maximumWidth: size
                }
                ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    Controls.Label {
                        text: ( (error.id) ? i18n("Media download") : i18n("Feed update error") )  + " ·  " + error.date.toLocaleDateString(Qt.locale(), Locale.NarrowFormat) + " ·  " + error.date.toLocaleTimeString(Qt.locale(), Locale.NarrowFormat)
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                    Controls.Label {
                        text: error.title
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        font.weight: Font.Normal
                        opacity: 1
                    }
                    Controls.Label {
                        text: i18n("Error code: ") + error.code + " ·  " + error.string
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                }
            }
        }
    }

    ListView {
        anchors.fill: parent
        id: errorList
        anchors.fill: parent
        visible: count !== 0
        model: ErrorLogModel

        delegate: Kirigami.DelegateRecycler {
            width: errorList.width
            sourceComponent: errorListDelegate
        }
    }
    actions.main: Kirigami.Action {
        text: i18n("Clear all errors")
        iconName: "edit-clear-all"
        visible: errorList.count > 0
        onTriggered: ErrorLogModel.clearAll()
    }
}
