/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kasts

Kirigami.Dialog {
    id: customizeRatesDialog
    title: i18nc("@title:window", "Playback Rate Presets")
    padding: Kirigami.Units.largeSpacing
    preferredWidth: Kirigami.Units.gridUnit * 16 + Kirigami.Units.smallSpacing

    closePolicy: Controls.Popup.CloseOnEscape
    standardButtons: Kirigami.Dialog.Save | Kirigami.Dialog.Cancel

    ListModel {
        id: rateModel

        Component.onCompleted: {
            for (var rate in SettingsManager.playbackRates) {
                rateModel.append({"value": SettingsManager.playbackRates[rate] / 100.0,
                                  "name": (SettingsManager.playbackRates[rate] / 100.0).toFixed(2) + "x"});
            }
        }
    }

    function getRateList() {
        var list = [];
        for (var i = 0; i < rateModel.count; i++) {
            list.push(Math.round(rateModel.get(i).value * 100));
        }
        return list;
    }

    ColumnLayout {
        id: playbackRateList

        implicitWidth: customizeRatesDialog.width
        spacing: Kirigami.Units.largeSpacing

        RowLayout {
            Layout.fillWidth: true
            Controls.Label {
                Layout.fillWidth: true
                text: i18nc("@info Label for controls to add new playback rate preset", "New Preset:")
            }

            Kirigami.Chip {
                text: (Math.round(rateSlider.value * 20) / 20.0).toFixed(2)
                closable: false
                checkable: false
            }

            Controls.Button {
                id: addButton
                icon.name: "list-add"
                text: i18nc("@action:button Add new playback rate value to list", "Add")
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: i18nc("@info:tooltip", "Add new playback rate value to list")
                onClicked: {
                    var found = false;
                    var insertIndex = 0;
                    var newValue = (Math.round(rateSlider.value * 20) / 20.0);
                    for (var i = 0; i < rateModel.count; i++) {
                        if (newValue == rateModel.get(i).value) {
                            found = true;
                            break;
                        }
                        if (newValue > rateModel.get(i).value) {
                            insertIndex = i + 1;
                        }
                    }
                    if (!found) {
                        rateModel.insert(insertIndex, {"value": newValue,
                                                       "name": newValue.toFixed(2) + "x"});
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: Kirigami.Units.gridUnit

            Controls.Button {
                text: "-"
                implicitWidth: addButton.height
                implicitHeight: addButton.height
                onClicked: rateSlider.value = Math.max(0.0, rateSlider.value - 0.05)
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: i18nc("@action:button", "Decrease playback rate")
            }

            Controls.Slider {
                id: rateSlider
                Layout.fillWidth: true
                from: 0
                to: 3
                value: 1
                handle.implicitWidth: implicitHeight // workaround to make slider handle position itself exactly at the location of the click
            }

            Controls.Button {
                text: "+"
                implicitWidth: addButton.height
                implicitHeight: addButton.height
                onClicked: rateSlider.value = Math.min(3.0, rateSlider.value + 0.05)
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: i18nc("@action:button", "Increase playback rate")
            }
        }

        Controls.Label {
            text: i18nc("@title:group List of custom playback rates", "Current Presets:")
        }

        GridLayout {
            columns: 4
            Layout.fillWidth: true
            Repeater {
                model: rateModel
                delegate: Kirigami.Chip {
                    Layout.fillWidth: true
                    text: model.value.toFixed(2)
                    closable: true
                    onRemoved: {
                        for (var i = 0; i < rateModel.count; i++) {
                            if (model.value == rateModel.get(i).value) {
                                rateModel.remove(i);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}
