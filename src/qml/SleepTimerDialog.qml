/**
 * SPDX-FileCopyrightText: 2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kasts

Kirigami.Dialog {
    id: root

    title: i18n("Sleep Timer")
    padding: Kirigami.Units.largeSpacing
    closePolicy: Kirigami.Dialog.CloseOnEscape | Kirigami.Dialog.CloseOnPressOutside
    standardButtons: Kirigami.Dialog.NoButton

    property bool timerActive: AudioManager.remainingSleepTime > 0

    customFooterActions: [
        Kirigami.Action {
            enabled: !root.timerActive
            text: i18n("Start")
            icon.name: "dialog-ok"
            onTriggered: {
                root.close();
                var sleepTimeSeconds = sleepTimerValueBox.value * sleepTimerUnitsBox.model[sleepTimerUnitsBox.currentIndex]["secs"];
                if (sleepTimeSeconds > 0) {
                    SettingsManager.sleepTimerValue = sleepTimerValueBox.value;
                    SettingsManager.sleepTimerUnits = sleepTimerUnitsBox.currentValue;
                    SettingsManager.save();
                    AudioManager.sleepTime = sleepTimeSeconds;
                }
            }
        },
        Kirigami.Action {
            enabled: root.timerActive
            text: i18n("Stop")
            icon.name: "dialog-cancel"
            onTriggered: {
                root.close();
                AudioManager.sleepTime = undefined; // make use of RESET
            }
        }
    ]

    ColumnLayout {
        id: content
        Controls.Label {
            text: root.timerActive ? i18n("Status: Active") : i18n("Status: Inactive")
        }

        Controls.Label {
            opacity: root.timerActive ? 1 : 0.5
            Layout.bottomMargin: Kirigami.Units.largeSpacing
            text: i18n("Remaining time: %1", AudioManager.formattedRemainingSleepTime)
        }

        RowLayout {
            Controls.SpinBox {
                id: sleepTimerValueBox
                enabled: !root.timerActive
                value: SettingsManager.sleepTimerValue
                from: 1
                to: 24 * 60 * 60
            }

            Controls.ComboBox {
                id: sleepTimerUnitsBox
                enabled: !root.timerActive
                textRole: "text"
                valueRole: "value"
                model: [
                    {
                        text: i18n("Seconds"),
                        value: 0,
                        secs: 1,
                        max: 24 * 60 * 60
                    },
                    {
                        text: i18n("Minutes"),
                        value: 1,
                        secs: 60,
                        max: 24 * 60
                    },
                    {
                        text: i18n("Hours"),
                        value: 2,
                        secs: 60 * 60,
                        max: 24
                    }
                ]
                Component.onCompleted: {
                    currentIndex = indexOfValue(SettingsManager.sleepTimerUnits);
                    sleepTimerValueBox.to = sleepTimerUnitsBox.model[currentIndex]["max"];
                    if (sleepTimerValueBox.value > sleepTimerUnitsBox.model[currentIndex]["max"]) {
                        sleepTimerValueBox.value = sleepTimerUnitsBox.model[currentIndex]["max"];
                    }
                }
                onActivated: {
                    SettingsManager.sleepTimerUnits = currentValue;
                    if (sleepTimerValueBox.value > sleepTimerUnitsBox.model[currentIndex]["max"]) {
                        sleepTimerValueBox.value = sleepTimerUnitsBox.model[currentIndex]["max"];
                    }
                    sleepTimerValueBox.to = sleepTimerUnitsBox.model[currentIndex]["max"];
                }
            }
        }
    }
}
