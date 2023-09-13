/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

import org.kde.kasts

ListView {
    id: errorList
    reuseItems: true

    model: ErrorLogModel
    implicitHeight: errorList.count > 0 ? errorList.contentHeight : placeholder.height
    currentIndex: -1

    Kirigami.PlaceholderMessage {
        id: placeholder
        height: 3.0 * Kirigami.Units.gridUnit
        visible: errorList.count == 0
        anchors.centerIn: parent

        text: i18n("No errors logged")
    }

    Component {
        id: errorListDelegate
        Kirigami.SwipeListItem {
            alwaysVisibleActions: true
            separatorVisible: true
            highlighted: false
            hoverEnabled: false
            contentItem: RowLayout {
                Kirigami.Icon {
                    source: "data-error"
                    property int size: Kirigami.Units.iconSizes.medium
                    Layout.preferredHeight: size
                    Layout.preferredWidth: size
                }
                ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    Controls.Label {
                        text: error.description  + "  ·  " + error.date.toLocaleDateString(Qt.locale(), Locale.NarrowFormat) + "  ·  " + error.date.toLocaleTimeString(Qt.locale(), Locale.NarrowFormat)
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
                        text: i18n("Error code:") + " " + error.code + (error.message ? "  ·  " + error.message : "")
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                }
            }
        }
    }

    delegate: errorListDelegate
}
