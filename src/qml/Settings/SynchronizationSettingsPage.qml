/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.delegates as Delegates
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.ki18n

import org.kde.kasts

import ".."

Kirigami.ScrollablePage {
    id: root

    leftPadding: 0
    rightPadding: 0

    ColumnLayout {
        spacing: 0

        FormCard.FormCard {
            Layout.fillWidth: true

            FormCard.FormTextDelegate {
                id: accountStatus
                text: KI18n.i18nc("@label", "Account")
                description: Sync.syncEnabled ? KI18n.i18nc("@info:status Shows which sync account and sync server the user is logged into", "Logged into account \"%1\" on server \"%2\"", Sync.username, (Sync.provider == SyncUtils.GPodderNet && Sync.hostname == "") ? "gpodder.net" : Sync.hostname) : KI18n.i18nc("@info:status", "Syncing disabled")

                trailing: Controls.Button {
                    text: Sync.syncEnabled ? KI18n.i18nc("@action:button", "Logout") : KI18n.i18nc("@action:button", "Login")
                    onClicked: {
                        Sync.syncEnabled ? Sync.logout() : syncProviderOverlay.open();
                    }
                }
            }

            FormCard.FormDelegateSeparator {}

            FormCard.FormTextDelegate {
                id: manualSync
                text: KI18n.i18nc("@label", "Manually sync")

                trailing: Controls.Button {
                    text: KI18n.i18nc("@action:button", "Sync Now")
                    enabled: Sync.syncEnabled
                    onClicked: {
                        syncFeedsAndEpisodes.run();
                    }
                }
            }

            FormCard.FormDelegateSeparator {}

            FormCard.FormTextDelegate {
                id: lastFullSync
                text: KI18n.i18nc("@info:status", "Last full sync with server")
                description: Sync.lastSuccessfulDownloadSync
            }

            FormCard.FormDelegateSeparator {}

            FormCard.FormTextDelegate {
                id: lastQuickUpload
                text: KI18n.i18nc("@info:status", "Last quick upload to sync server")
                description: Sync.lastSuccessfulUploadSync
            }
        }

        FormCard.FormHeader {
            title: KI18n.i18nc("@title Form header for settings related to automatic syncing", "Automatic syncing")
            Layout.fillWidth: true
        }

        FormCard.FormCard {
            Layout.fillWidth: true

            FormCard.FormSwitchDelegate {
                enabled: Sync.syncEnabled
                checked: SettingsManager.refreshOnStartup
                text: KI18n.i18nc("@option:check", "Do full sync on startup")
                onToggled: {
                    SettingsManager.refreshOnStartup = checked;
                    SettingsManager.save();
                }
            }

            FormCard.FormSwitchDelegate {
                enabled: Sync.syncEnabled
                checked: SettingsManager.syncWhenUpdatingFeeds
                text: KI18n.i18nc("@option:check", "Do full sync when fetching podcasts")
                onToggled: {
                    SettingsManager.syncWhenUpdatingFeeds = checked;
                    SettingsManager.save();
                }
            }

            FormCard.FormSwitchDelegate {
                enabled: Sync.syncEnabled
                checked: SettingsManager.syncWhenPlayerstateChanges
                text: KI18n.i18nc("@option:check", "Upload episode play positions on play/pause toggle")
                onToggled: {
                    SettingsManager.syncWhenPlayerstateChanges = checked;
                    SettingsManager.save();
                }
            }
        }

        FormCard.FormHeader {
            title: KI18n.i18nc("@title Form header for advanced settings related to syncing", "Advanced options")
            Layout.fillWidth: true
        }

        FormCard.FormCard {
            Layout.fillWidth: true

            FormCard.FormTextDelegate {
                id: fetchAllEpisodeStates
                text: KI18n.i18nc("@label", "Fetch all episode states from server")

                trailing: Controls.Button {
                    text: KI18n.i18nc("@action:button", "Fetch")
                    enabled: Sync.syncEnabled
                    onClicked: {
                        forceSyncFeedsAndEpisodes.run();
                    }
                }
            }

            FormCard.FormDelegateSeparator {}

            FormCard.FormTextDelegate {
                id: fetchLocalEpisodeStates
                text: KI18n.i18nc("@label", "Push all local episode states to server")

                trailing: Controls.Button {
                    enabled: Sync.syncEnabled
                    text: KI18n.i18nc("@action:button", "Push")
                    onClicked: {
                        syncPushAllStatesDialog.open();
                    }
                }
            }
        }
    }

    // This item can be used to trigger an update of all feeds; it will open an
    // overlay with options in case the operation is not allowed by the settings
    ConnectionCheckAction {
        id: syncFeedsAndEpisodes

        function action(): void {
            Sync.doRegularSync();
        }
    }

    // This item can be used to trigger an update of all feeds; it will open an
    // overlay with options in case the operation is not allowed by the settings
    ConnectionCheckAction {
        id: forceSyncFeedsAndEpisodes

        function action(): void {
            Sync.doForceSync();
        }
    }

    Kirigami.Dialog {
        id: syncPushAllStatesDialog
        preferredWidth: Kirigami.Units.gridUnit * 25
        padding: Kirigami.Units.largeSpacing

        showCloseButton: true
        standardButtons: Controls.DialogButtonBox.Ok | Controls.DialogButtonBox.Cancel
        closePolicy: Kirigami.Dialog.CloseOnEscape | Kirigami.Dialog.CloseOnPressOutside

        title: KI18n.i18nc("@label", "Push all local episode states to server?")

        Component.onCompleted: {
            parent = WindowUtils.focusedWindowItem();
        }

        onAccepted: {
            syncPushAllStatesDialog.close();
            syncPushAllStates.run();
        }
        onRejected: syncPushAllStatesDialog.close()

        RowLayout {
            spacing: Kirigami.Units.largeSpacing
            Kirigami.Icon {
                Layout.preferredHeight: Kirigami.Units.gridUnit * 4
                Layout.preferredWidth: Kirigami.Units.gridUnit * 4
                source: Sync.provider === SyncUtils.GPodderNextcloud ? "kaccounts-nextcloud" : "gpodder"
            }
            TextEdit {
                Layout.fillWidth: true
                Layout.fillHeight: true
                readOnly: true
                wrapMode: Text.WordWrap
                text: KI18n.i18nc("@label", "Please note that pushing the playback state of all local episodes to the server might take a very long time and/or might overload the server. Also note that this action will overwrite all existing episode states on the server.\n\nContinue?")
                color: Kirigami.Theme.textColor
                Keys.onReturnPressed: syncPushAllStatesDialog.accepted()
            }
        }
    }

    // This item can be used to trigger a push of all episode states to the server;
    // it will open an overlay with options in case the operation is not allowed by the settings
    ConnectionCheckAction {
        id: syncPushAllStates

        function action(): void {
            Sync.doSyncPushAll();
        }
    }

    Kirigami.Dialog {
        id: syncProviderOverlay
        preferredWidth: Kirigami.Units.gridUnit * 20
        standardButtons: Kirigami.Dialog.NoButton

        showCloseButton: true

        title: KI18n.i18nc("@label", "Select Sync Provider")

        Component.onCompleted: {
            parent = WindowUtils.focusedWindowItem();
        }

        property list<var> providerModel: [
            {
                name: KI18n.i18nc("@label", "gpodder.net"),
                subtitle: KI18n.i18nc("@label", "Synchronize with official gpodder.net server"),
                iconName: "gpodder",
                provider: SyncUtils.GPodderNet
            },
            {
                name: KI18n.i18nc("@label", "GPodder Nextcloud"),
                subtitle: KI18n.i18nc("@label", "Synchronize with GPodder Nextcloud app"),
                iconName: "kaccounts-nextcloud",
                provider: SyncUtils.GPodderNextcloud
            }
        ]

        ColumnLayout {
            spacing: 0

            Repeater {
                focus: syncProviderOverlay.visible

                model: syncProviderOverlay.providerModel

                delegate: Delegates.RoundedItemDelegate {
                    id: syncProviderRepeaterDelegate
                    required property string name
                    required property string iconName
                    required property string subtitle
                    required property var provider
                    Layout.fillWidth: true
                    text: name
                    icon.name: iconName
                    contentItem: Delegates.SubtitleContentItem {
                        itemDelegate: syncProviderRepeaterDelegate
                        subtitle: subtitle
                    }
                    Keys.onReturnPressed: clicked()
                    onClicked: {
                        Sync.provider = provider;
                        syncProviderOverlay.close();
                        syncLoginOverlay.open();
                    }
                }
            }
        }
    }

    Kirigami.Dialog {
        id: syncLoginOverlay
        maximumWidth: Kirigami.Units.gridUnit * 30
        padding: Kirigami.Units.largeSpacing

        showCloseButton: true
        standardButtons: Controls.DialogButtonBox.Ok | Controls.DialogButtonBox.Cancel
        closePolicy: Kirigami.Dialog.CloseOnEscape | Kirigami.Dialog.CloseOnPressOutside

        title: KI18n.i18nc("@title of dialog box", "Sync Login Credentials")

        Component.onCompleted: {
            parent = WindowUtils.focusedWindowItem();
        }

        onAccepted: {
            if (Sync.provider === SyncUtils.GPodderNextcloud || customServerCheckBox.checked) {
                Sync.hostname = hostnameField.text;
            } else {
                Sync.hostname = "";
            }
            Sync.login(usernameField.text, passwordField.text);
            syncLoginOverlay.close();
        }
        onRejected: syncLoginOverlay.close()

        ColumnLayout {
            spacing: Kirigami.Units.largeSpacing
            RowLayout {
                width: parent.width
                spacing: Kirigami.Units.largeSpacing
                Kirigami.Icon {
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 4
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 4
                    source: Sync.provider === SyncUtils.GPodderNextcloud ? "kaccounts-nextcloud" : "gpodder"
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Kirigami.Heading {
                        clip: true
                        level: 2
                        text: Sync.provider === SyncUtils.GPodderNextcloud ? KI18n.i18nc("@label", "Sync with GPodder Nextcloud app") : KI18n.i18nc("@label", "Sync with gpodder.net service")
                    }
                    TextEdit {
                        Layout.fillWidth: true
                        readOnly: true
                        wrapMode: Text.WordWrap
                        textFormat: Text.RichText
                        onLinkActivated: link => {
                            Qt.openUrlExternally(link);
                        }
                        text: Sync.provider === SyncUtils.GPodderNextcloud ? KI18n.i18nc("@label argument is a weblink", "Sync with a Nextcloud server that has the GPodder Sync app installed: %1.<br/>It is advised to manually create an app password for Kasts through the web interface and use those credentials.", "<a href=\"https://apps.nextcloud.com/apps/gpoddersync\">https://apps.nextcloud.com/apps/gpoddersync</a>") : KI18n.i18nc("@label argument is a weblink", "If you don't already have an account, you should first create one at %1", "<a href=\"https://gpodder.net\">https://gpodder.net</a>")
                        color: Kirigami.Theme.textColor
                    }
                }
            }
            GridLayout {
                width: parent.width
                columns: 2
                rowSpacing: Kirigami.Units.smallSpacing
                columnSpacing: Kirigami.Units.smallSpacing
                Controls.Label {
                    Layout.alignment: Qt.AlignRight
                    text: KI18n.i18nc("@label:textbox", "Username:")
                }
                Controls.TextField {
                    id: usernameField
                    Layout.fillWidth: true
                    text: Sync.username
                    Keys.onReturnPressed: syncLoginOverlay.accepted()
                    // focus: syncLoginOverlay.visible // disabled for now since it causes problem with virtual keyboard appearing at the same time as the overlay
                }
                Controls.Label {
                    Layout.alignment: Qt.AlignRight
                    text: KI18n.i18nc("@label:textbox", "Password:")
                }
                Controls.TextField {
                    id: passwordField
                    Layout.fillWidth: true
                    echoMode: TextInput.Password
                    text: Sync.password
                    Keys.onReturnPressed: syncLoginOverlay.accepted()
                }
                Controls.CheckBox {
                    id: customServerCheckBox
                    Layout.row: 2
                    Layout.column: 1
                    visible: Sync.provider === SyncUtils.GPodderNet
                    checked: false
                    text: KI18n.i18nc("@option:check", "Use custom server")
                }
                Controls.Label {
                    visible: Sync.provider === SyncUtils.GPodderNextcloud || customServerCheckBox.checked
                    Layout.alignment: Qt.AlignRight
                    text: KI18n.i18nc("@label:textbox", "Hostname:")
                }
                Controls.TextField {
                    id: hostnameField
                    visible: Sync.provider === SyncUtils.GPodderNextcloud || customServerCheckBox.checked
                    Layout.fillWidth: true
                    placeholderText: Sync.provider === SyncUtils.GPodderNet ? "https://gpodder.net" : "https://nextcloud.mydomain.org"
                    text: Sync.hostname
                    Keys.onReturnPressed: syncLoginOverlay.accepted()
                }
            }
        }
    }

    Connections {
        target: Sync
        function onDeviceListReceived(): void {
            syncDeviceOverlay.open();
            syncDeviceOverlay.update();
        }
        function onLoginSucceeded(): void {
            if (Sync.provider === SyncUtils.GPodderNextcloud) {
                firstSyncOverlay.open();
            }
        }
    }

    Kirigami.Dialog {
        id: syncDeviceOverlay
        maximumWidth: Kirigami.Units.gridUnit * 30
        preferredWidth: Kirigami.Units.gridUnit * 30
        padding: Kirigami.Units.largeSpacing

        showCloseButton: true

        title: KI18n.i18nc("@title", "Sync Device Settings")

        Component.onCompleted: {
            parent = WindowUtils.focusedWindowItem();
        }

        Column {
            spacing: Kirigami.Units.largeSpacing * 2
            Kirigami.Heading {
                level: 2
                text: KI18n.i18nc("@action:button", "Create a new device")
            }
            GridLayout {
                columns: 2
                width: parent.width
                Controls.Label {
                    text: KI18n.i18nc("@label:textbox", "Device Name:")
                }
                Controls.TextField {
                    id: deviceField
                    Layout.fillWidth: true
                    text: Sync.suggestedDevice
                    Keys.onReturnPressed: createDeviceButton.clicked()
                    // focus: syncDeviceOverlay.visible // disabled for now since it causes problem with virtual keyboard appearing at the same time as the overlay
                }
                Controls.Label {
                    text: KI18n.i18nc("@label:textbox", "Device Description:")
                }
                Controls.TextField {
                    id: deviceNameField
                    Layout.fillWidth: true
                    text: Sync.suggestedDeviceName
                    Keys.onReturnPressed: createDeviceButton.clicked()
                }
                Controls.Label {
                    text: KI18n.i18nc("@label:listbox", "Device Type:")
                }
                Controls.ComboBox {
                    id: deviceTypeField
                    textRole: "text"
                    valueRole: "value"
                    popup.z: 102 // popup has to go in front of OverlaySheet
                    model: [
                        {
                            text: KI18n.i18nc("@item:inlistbox type of device", "other"),
                            value: "other"
                        },
                        {
                            text: KI18n.i18nc("@item:inlistbox type of device", "desktop"),
                            value: "desktop"
                        },
                        {
                            text: KI18n.i18nc("@item:inlistbox type of device", "laptop"),
                            value: "laptop"
                        },
                        {
                            text: KI18n.i18nc("@item:inlistbox type of device", "server"),
                            value: "server"
                        },
                        {
                            text: KI18n.i18nc("@item:inlistbox type of device", "mobile"),
                            value: "mobile"
                        }
                    ]
                }
            }
            Controls.Button {
                id: createDeviceButton
                text: KI18n.i18nc("@action:button", "Create Device")
                icon.name: "list-add"
                onClicked: {
                    Sync.registerNewDevice(deviceField.text, deviceNameField.text, deviceTypeField.currentValue);
                    syncDeviceOverlay.close();
                }
            }
            ListView {
                id: deviceList
                width: parent.width
                height: contentItem.childrenRect.height
                visible: deviceListModel.count !== 0

                header: Kirigami.Heading {
                    topPadding: Kirigami.Units.gridUnit
                    bottomPadding: Kirigami.Units.largeSpacing
                    level: 2
                    text: KI18n.i18nc("@label", "or select an existing device")
                }
                model: ListModel {
                    id: deviceListModel
                }

                delegate: Delegates.RoundedItemDelegate {
                    required property var device
                    text: device.caption
                    icon.name: device.type == "desktop" ? "computer" : device.type == "laptop" ? "computer-laptop" : device.type == "server" ? "network-server-database" : device.type == "mobile" ? "smartphone" : "emblem-music-symbolic"
                    onClicked: {
                        syncDeviceOverlay.close();
                        Sync.device = device.id;
                        Sync.deviceName = device.caption;
                        Sync.syncEnabled = true;
                        syncGroupOverlay.open();
                    }
                }
            }
        }

        function update(): void {
            deviceListModel.clear();
            for (var index in Sync.deviceList) {
                deviceListModel.append({
                    device: Sync.deviceList[index]
                });
            }
        }
    }

    Connections {
        target: Sync
        function onDeviceCreated(): void {
            syncGroupOverlay.open();
        }
    }

    Kirigami.Dialog {
        id: syncGroupOverlay
        preferredWidth: Kirigami.Units.gridUnit * 25
        padding: Kirigami.Units.largeSpacing

        showCloseButton: true
        standardButtons: Controls.DialogButtonBox.Ok | Controls.DialogButtonBox.Cancel
        closePolicy: Kirigami.Dialog.CloseOnEscape | Kirigami.Dialog.CloseOnPressOutside

        title: KI18n.i18nc("@title of dialog box", "Device Sync Settings")

        Component.onCompleted: {
            parent = WindowUtils.focusedWindowItem();
        }

        onAccepted: {
            Sync.linkUpAllDevices();
            syncGroupOverlay.close();
        }
        onRejected: {
            syncGroupOverlay.close();
        }

        RowLayout {
            spacing: Kirigami.Units.largeSpacing
            Kirigami.Icon {
                Layout.preferredHeight: Kirigami.Units.gridUnit * 4
                Layout.preferredWidth: Kirigami.Units.gridUnit * 4
                source: "gpodder"
            }
            TextEdit {
                Layout.fillWidth: true
                Layout.fillHeight: true
                readOnly: true
                wrapMode: Text.WordWrap
                text: KI18n.i18nc("@label", "Should all podcast subscriptions on this gpodder.net account be synced across all devices?\nIf you don't know what this means, you should probably select \"Ok\".")
                color: Kirigami.Theme.textColor
                Keys.onReturnPressed: syncGroupOverlay.accepted()
            }
        }

        onVisibleChanged: {
            if (!visible) {
                firstSyncOverlay.open();
            }
        }
    }

    Kirigami.Dialog {
        id: firstSyncOverlay
        preferredWidth: Kirigami.Units.gridUnit * 16
        padding: Kirigami.Units.largeSpacing

        showCloseButton: true
        standardButtons: Controls.DialogButtonBox.Ok | Controls.DialogButtonBox.Cancel
        closePolicy: Kirigami.Dialog.CloseOnEscape | Kirigami.Dialog.CloseOnPressOutside

        title: KI18n.i18nc("@title of dialog box", "Sync Now?")

        Component.onCompleted: {
            parent = WindowUtils.focusedWindowItem();
        }

        onAccepted: {
            firstSyncOverlay.close();
            Sync.doRegularSync();
        }
        onRejected: firstSyncOverlay.close()

        RowLayout {
            spacing: Kirigami.Units.largeSpacing
            Kirigami.Icon {
                Layout.preferredHeight: Kirigami.Units.gridUnit * 4
                Layout.preferredWidth: Kirigami.Units.gridUnit * 4
                source: Sync.provider === SyncUtils.GPodderNextcloud ? "kaccounts-nextcloud" : "gpodder"
            }
            TextEdit {
                Layout.fillWidth: true
                Layout.fillHeight: true
                readOnly: true
                wrapMode: Text.WordWrap
                text: KI18n.i18nc("@label", "Perform a first sync now?")
                color: Kirigami.Theme.textColor
                Keys.onReturnPressed: firstSyncOverlay.accepted()
            }
        }
    }
}
