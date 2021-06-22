/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.15

import org.kde.kirigami 2.15 as Kirigami

import org.kde.kasts 1.0

Kirigami.OverlaySheet {
    id: errorOverlay

    showCloseButton: true

    header: Kirigami.Heading {
        text: i18n("Error Log")
        level: 2
        wrapMode: Text.Wrap
    }

    footer: Controls.DialogButtonBox {
        Controls.Button {
            text: i18n("Clear All Errors")
            icon.name: "edit-clear-all"
            onClicked: ErrorLogModel.clearAll()
            enabled: errorList.count > 0
        }
    }


    Kirigami.PlaceholderMessage {
        id: placeholder
        visible: errorList.count == 0

        text: i18n("No Errors Logged")
    }

    ListView {
        id: errorList
        visible: errorList.count > 0
        implicitWidth: Kirigami.Units.gridUnit * 20
        model: ErrorLogModel

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

        delegate: Kirigami.DelegateRecycler {
            width: errorList.width
            sourceComponent: errorListDelegate
        }
    }

    contentItem: errorList.count > 0 ? errorList : placeholder
}
