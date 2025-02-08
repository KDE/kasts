/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls

import org.kde.kirigami as Kirigami
import org.kde.kasts

Controls.Menu {
    id: root

    title: i18nc("@title:window", "Select Playback Rate")

    Controls.ButtonGroup {
        id: playbackRateGroup
    }

    // Using instantiator because using Repeater made the separator show up in
    // the wrong place
    Instantiator {
        model: playbackRateModel

        delegate: Controls.MenuItem {
            required property string value
            text: value
            checkable: true
            checked: value === AudioManager.playbackRate.toFixed(2)
            Controls.ButtonGroup.group: playbackRateGroup

            onTriggered: {
                if (checked) {
                    AudioManager.playbackRate = value;
                }
                root.dismiss();
            }
        }

        onObjectAdded: (index, object) => root.insertItem(index, object)
        onObjectRemoved: (index, object) => root.removeItem(object)
    }

    Controls.MenuSeparator {
        padding: Kirigami.Units.smallSpacing
    }

    Kirigami.Action {
        text: i18nc("@action:button", "Customize")
        icon.name: "settings-configure"
        onTriggered: (customizeDialog.createObject(parent) as PlaybackRateCustomizerDialog).open()
    }

    ListModel {
        id: playbackRateModel

        function resetModel(): void {
            playbackRateModel.clear();
            for (var rate in SettingsManager.playbackRates) {
                playbackRateModel.append({
                    value: (SettingsManager.playbackRates[rate] / 100.0).toFixed(2),
                    name: (SettingsManager.playbackRates[rate] / 100.0).toFixed(2) + "x"
                });
            }
        }

        Component.onCompleted: {
            resetModel();
        }
    }

    Component {
        id: customizeDialog
        PlaybackRateCustomizerDialog {
            onAccepted: {
                var newRates = getRateList();
                SettingsManager.playbackRates = newRates;
                SettingsManager.save();
                playbackRateModel.resetModel();
            }
        }
    }
}
