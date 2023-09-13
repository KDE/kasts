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

Controls.Menu {
    id: playbackRateMenu

    required property QtObject parentButton

    title: i18nc("@title:window", "Select Playback Rate")

    Controls.ButtonGroup { id: playbackRateGroup }

    // Using instantiator because using Repeater made the separator show up in
    // the wrong place
    Instantiator {
        model: playbackRateModel

        Controls.MenuItem {
            text: model.value
            checkable: true
            checked: model.value === AudioManager.playbackRate.toFixed(2)
            Controls.ButtonGroup.group: playbackRateGroup

            onTriggered: {
                if (checked) {
                    AudioManager.playbackRate = value;
                }
                playbackRateMenu.dismiss();
            }
        }

        onObjectAdded: (index, object) => playbackRateMenu.insertItem(index, object)
        onObjectRemoved: (object) => playbackRateMenu.removeItem(object)
    }

    Controls.MenuSeparator {
        padding: Kirigami.Units.smallSpacing
    }

    Kirigami.Action {
        text: i18nc("@action:button", "Customize")
        icon.name: "settings-configure"
        onTriggered: {
            const dialog = customizeDialog.createObject(parent);
            dialog.open();
        }
    }

    ListModel {
        id: playbackRateModel

        function resetModel() {
            playbackRateModel.clear();
            for (var rate in SettingsManager.playbackRates) {
                playbackRateModel.append({"value": (SettingsManager.playbackRates[rate] / 100.0).toFixed(2),
                                          "name": (SettingsManager.playbackRates[rate] / 100.0).toFixed(2) + "x"});
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
