/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.15

import org.kde.kirigami 2.19 as Kirigami

import org.kde.kasts 1.0

Kirigami.Dialog {
    id: errorOverlay
    preferredWidth: Kirigami.Units.gridUnit * 25
    preferredHeight: Kirigami.Units.gridUnit * 16

    showCloseButton: true

    title: i18n("Error Log")
    standardButtons: Kirigami.Dialog.NoButton

    customFooterActions: Kirigami.Action {
        text: i18n("Clear All Errors")
        iconName: "edit-clear-all"
        onTriggered: ErrorLogModel.clearAll()
        enabled: errorList.count > 0
    }

    ListView {
        id: errorList
        reuseItems: true

        model: ErrorLogModel

        Kirigami.PlaceholderMessage {
            id: placeholder
            visible: errorList.count == 0
            anchors.centerIn: parent

            text: i18n("No Errors Logged")
        }

        Component {
            id: errorListDelegate
            Kirigami.SwipeListItem {
                // workaround to get rid of "_swipeFilter" errors
                alwaysVisibleActions: true
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
                            text: i18n("Error Code: ") + error.code + (error.message ? "  ·  " + error.message : "")
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
}
