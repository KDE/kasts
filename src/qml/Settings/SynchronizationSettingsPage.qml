/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import org.kde.kasts 1.0

Kirigami.ScrollablePage {
    title: i18n("Synchronization Settings")

    leftPadding: 0
    rightPadding: 0
    topPadding: Kirigami.Units.gridUnit
    bottomPadding: Kirigami.Units.gridUnit

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    ColumnLayout {
        spacing: 0

        MobileForm.FormCard {
            Layout.fillWidth: true

            contentItem: ColumnLayout {
                spacing: 0

                MobileForm.FormTextDelegate {
                    id: accountStatus
                    text: i18n("Account")
                    description: Sync.syncEnabled ? i18n("Logged into account \"%1\" on server \"%2\"", Sync.username, (Sync.provider == SyncUtils.GPodderNet && Sync.hostname == "") ? "gpodder.net" : Sync.hostname) : i18n("Syncing disabled")

                    trailing: Controls.Button {
                        text: Sync.syncEnabled ? i18n("Logout") : i18n("Login")
                        onClicked: {
                            Sync.syncEnabled ? Sync.logout() : syncProviderOverlay.open();
                        }
                    }
                }

                MobileForm.FormDelegateSeparator {}

                MobileForm.FormTextDelegate {
                    id: manualSync
                    text: i18n("Manually sync")

                    trailing: Controls.Button {
                        text: i18n("Sync Now")
                        enabled: Sync.syncEnabled
                        onClicked: {
                            syncFeedsAndEpisodes.run();
                        }
                    }
                }

                MobileForm.FormDelegateSeparator {}

                MobileForm.FormTextDelegate {
                    id: lastFullSync
                    text: i18n("Last full sync with server")
                    description: Sync.lastSuccessfulDownloadSync
                }

                MobileForm.FormDelegateSeparator {}

                MobileForm.FormTextDelegate {
                    id: lastQuickUpload
                    text: i18n("Last quick upload to sync server")
                    description: Sync.lastSuccessfulUploadSync
                }
            }
        }

        MobileForm.FormCard {
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.fillWidth: true

            contentItem: ColumnLayout {
                spacing: 0

                MobileForm.FormCardHeader {
                    title: i18n("Automatic syncing")
                }

                MobileForm.FormCheckDelegate {
                    enabled: Sync.syncEnabled
                    checked: SettingsManager.refreshOnStartup
                    text: i18n("Do full sync on startup")
                    onToggled: SettingsManager.refreshOnStartup = checked
                }

                MobileForm.FormCheckDelegate {
                    enabled: Sync.syncEnabled
                    checked: SettingsManager.syncWhenUpdatingFeeds
                    text: i18n("Do full sync when fetching podcasts")
                    onToggled: SettingsManager.syncWhenUpdatingFeeds = checked
                }

                MobileForm.FormCheckDelegate {
                    enabled: Sync.syncEnabled
                    checked: SettingsManager.syncWhenPlayerstateChanges
                    text: i18n("Upload episode play positions on play/pause toggle")
                    onToggled: SettingsManager.syncWhenPlayerstateChanges = checked
                }
            }
        }

        MobileForm.FormCard {
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.fillWidth: true

            contentItem: ColumnLayout {
                spacing: 0

                MobileForm.FormCardHeader {
                    title: i18n("Advanced options")
                }

                MobileForm.FormTextDelegate {
                    id: fetchAllEpisodeStates
                    text: i18n("Fetch all episode states from server")

                    trailing: Controls.Button {
                        text: i18n("Fetch")
                        enabled: Sync.syncEnabled
                        onClicked: {
                            forceSyncFeedsAndEpisodes.run();
                        }
                    }
                }

                MobileForm.FormDelegateSeparator {}

                MobileForm.FormTextDelegate {
                    id: fetchLocalEpisodeStates
                    text: i18n("Push all local episode states to server")

                    trailing: Controls.Button {
                        enabled: Sync.syncEnabled
                        text: i18n("Push")
                        onClicked: {
                            syncPushAllStatesDialog.open();
                        }
                    }
                }
            }
        }
    }

    // This item can be used to trigger an update of all feeds; it will open an
    // overlay with options in case the operation is not allowed by the settings
    ConnectionCheckAction {
        id: syncFeedsAndEpisodes

        function action() {
            Sync.doRegularSync();
        }
    }

    // This item can be used to trigger an update of all feeds; it will open an
    // overlay with options in case the operation is not allowed by the settings
    ConnectionCheckAction {
        id: forceSyncFeedsAndEpisodes

        function action() {
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

        title: i18n("Push all local episode states to server?")

        onAccepted: {
            syncPushAllStatesDialog.close();
            syncPushAllStates.run();
        }
        onRejected: syncPushAllStatesDialog.close();

        RowLayout {
            spacing: Kirigami.Units.largeSpacing
            Kirigami.Icon {
                Layout.preferredHeight: Kirigami.Units.gridUnit * 4
                Layout.preferredWidth: Kirigami.Units.gridUnit * 4
                source: Sync.provider === Sync.GPodderNextcloud ? "kaccounts-nextcloud" : "gpodder"
            }
            TextEdit {
                Layout.fillWidth: true
                Layout.fillHeight: true
                readOnly: true
                wrapMode: Text.WordWrap
                text: i18n("Please note that pushing the playback state of all local episodes to the server might take a very long time and/or might overload the server. Also note that this action will overwrite all existing episode states on the server.\n\nContinue?")
                color: Kirigami.Theme.textColor
                Keys.onReturnPressed: accepted();
            }
        }
    }

    // This item can be used to trigger a push of all episode states to the server;
    // it will open an overlay with options in case the operation is not allowed by the settings
    ConnectionCheckAction {
        id: syncPushAllStates

        function action() {
            Sync.doSyncPushAll();
        }
    }

    Kirigami.Dialog {
        id: syncProviderOverlay
        preferredWidth: Kirigami.Units.gridUnit * 20
        standardButtons: Kirigami.Dialog.NoButton

        showCloseButton: true

        title: i18n("Select Sync Provider")

        ColumnLayout {
            spacing: 0

            Repeater {
                focus: syncProviderOverlay.visible

                model: ListModel {
                    id: providerModel
                }
                Component.onCompleted: {
                    providerModel.append({"name": i18n("gpodder.net"),
                                        "subtitle": i18n("Synchronize with official gpodder.net server"),
                                        "icon": "gpodder",
                                        "provider": Sync.GPodderNet});
                    providerModel.append({"name": i18n("GPodder Nextcloud"),
                                        "subtitle": i18n("Synchronize with GPodder Nextcloud app"),
                                        "icon": "kaccounts-nextcloud",
                                        "provider": Sync.GPodderNextcloud});
                }
                delegate: Kirigami.BasicListItem {
                    Layout.fillWidth: true
                    label: model.name
                    subtitle: model.subtitle
                    icon: model.icon
                    //highlighted: false
                    iconSize: Kirigami.Units.gridUnit * 3
                    Keys.onReturnPressed: clicked()
                    onClicked: {
                        Sync.provider = model.provider;
                        syncProviderOverlay.close();
                        syncLoginOverlay.open();
                    }
                }
            }
        }
    }

    Kirigami.Dialog {
        id: syncLoginOverlay
        preferredWidth: Kirigami.Units.gridUnit * 25
        padding: Kirigami.Units.largeSpacing

        showCloseButton: true
        standardButtons: Controls.DialogButtonBox.Ok | Controls.DialogButtonBox.Cancel
        closePolicy: Kirigami.Dialog.CloseOnEscape | Kirigami.Dialog.CloseOnPressOutside

        title: i18n("Sync Login Credentials")

        onAccepted: {
            if (Sync.provider === Sync.GPodderNextcloud || customServerCheckBox.checked) {
                Sync.hostname = hostnameField.text;
            } else {
                Sync.hostname = ""
            }
            Sync.login(usernameField.text, passwordField.text);
            syncLoginOverlay.close();
        }
        onRejected: syncLoginOverlay.close();

        Column {
            spacing: Kirigami.Units.largeSpacing
            RowLayout {
                width: parent.width
                spacing: Kirigami.Units.largeSpacing
                Kirigami.Icon {
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 4
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 4
                    source: Sync.provider === Sync.GPodderNextcloud ? "kaccounts-nextcloud" : "gpodder"
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Kirigami.Heading {
                        clip: true
                        level: 2
                        text: Sync.provider === Sync.GPodderNextcloud ? i18n("Sync with GPodder Nextcloud app") : i18n("Sync with gpodder.net service")
                    }
                    TextEdit {
                        Layout.fillWidth: true
                        readOnly: true
                        wrapMode: Text.WordWrap
                        textFormat: Text.RichText
                        onLinkActivated: Qt.openUrlExternally(link)
                        text: Sync.provider === Sync.GPodderNextcloud ?
                            i18nc("argument is a weblink", "Sync with a Nextcloud server that has the GPodder Sync app installed: %1.<br/>It is advised to manually create an app password for Kasts through the web interface and use those credentials." , "<a href=\"https://apps.nextcloud.com/apps/gpoddersync\">https://apps.nextcloud.com/apps/gpoddersync</a>") :
                            i18nc("argument is a weblink", "If you don't already have an account, you should first create one at %1", "<a href=\"https://gpodder.net\">https://gpodder.net</a>")
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
                    text: i18n("Username:")
                }
                Controls.TextField {
                    id: usernameField
                    Layout.fillWidth: true
                    text: Sync.username
                    Keys.onReturnPressed: syncLoginOverlay.accepted();
                    // focus: syncLoginOverlay.visible // disabled for now since it causes problem with virtual keyboard appearing at the same time as the overlay
                }
                Controls.Label {
                    Layout.alignment: Qt.AlignRight
                    text: i18n("Password:")
                }
                Controls.TextField {
                    id: passwordField
                    Layout.fillWidth: true
                    echoMode: TextInput.Password
                    text: Sync.password
                    Keys.onReturnPressed: syncLoginOverlay.accepted();
                }
                Controls.CheckBox {
                    id: customServerCheckBox
                    Layout.row: 2
                    Layout.column: 1
                    visible: Sync.provider === Sync.GPodderNet
                    checked: false
                    text: i18n("Use custom server")
                }
                Controls.Label {
                    visible: Sync.provider === Sync.GPodderNextcloud || customServerCheckBox.checked
                    Layout.alignment: Qt.AlignRight
                    text: i18n("Hostname:")
                }
                Controls.TextField {
                    visible: Sync.provider === Sync.GPodderNextcloud || customServerCheckBox.checked
                    id: hostnameField
                    Layout.fillWidth: true
                    placeholderText: Sync.provider === Sync.GPodderNet ? "https://gpodder.net" : "https://nextcloud.mydomain.org"
                    text: Sync.hostname
                    Keys.onReturnPressed: syncLoginOverlay.accepted();
                }
            }
        }
    }

    Connections {
        target: Sync
        function onDeviceListReceived() {
            syncDeviceOverlay.open();
            syncDeviceOverlay.update();
        }
        function onLoginSucceeded() {
            if (Sync.provider === Sync.GPodderNextcloud) {
                firstSyncOverlay.open();
            }
        }
    }

    Kirigami.Dialog {
        id: syncDeviceOverlay
        preferredWidth: Kirigami.Units.gridUnit * 25
        padding: Kirigami.Units.largeSpacing

        showCloseButton: true

        title: i18n("Sync Device Settings")

        Column {
            spacing: Kirigami.Units.largeSpacing * 2
            Kirigami.Heading {
                level: 2
                text: i18n("Create a new device")
            }
            GridLayout {
                columns: 2
                width: parent.width
                Controls.Label {
                    text: i18n("Device Name:")
                }
                Controls.TextField {
                    id: deviceField
                    Layout.fillWidth: true
                    text: Sync.suggestedDevice
                    Keys.onReturnPressed: createDeviceButton.clicked();
                    // focus: syncDeviceOverlay.visible // disabled for now since it causes problem with virtual keyboard appearing at the same time as the overlay
                }
                Controls.Label {
                    text: i18n("Device Description:")
                }
                Controls.TextField {
                    id: deviceNameField
                    Layout.fillWidth: true
                    text: Sync.suggestedDeviceName
                    Keys.onReturnPressed: createDeviceButton.clicked();
                }
                Controls.Label {
                    text: i18n("Device Type:")
                }
                Controls.ComboBox {
                    id: deviceTypeField
                    textRole: "text"
                    valueRole: "value"
                    popup.z: 102 // popup has to go in front of OverlaySheet
                    model: [{"text": i18n("other"), "value": "other"},
                            {"text": i18n("desktop"), "value": "desktop"},
                            {"text": i18n("laptop"), "value": "laptop"},
                            {"text": i18n("server"), "value": "server"},
                            {"text": i18n("mobile"), "value": "mobile"}]
                }
            }
            Controls.Button {
                id: createDeviceButton
                text: i18n("Create Device")
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
                    text: i18n("or select an existing device")
                }
                model: ListModel {
                    id: deviceListModel
                }

                delegate: Kirigami.BasicListItem {
                    label: model.device.caption
                    highlighted: false
                    icon: model.device.type == "desktop" ? "computer" :
                        model.device.type == "laptop" ? "computer-laptop" :
                        model.device.type == "server" ? "network-server-database" :
                        model.device.type == "mobile" ? "smartphone" :
                        "emblem-music-symbolic"
                    onClicked: {
                        syncDeviceOverlay.close();
                        Sync.device = model.device.id;
                        Sync.deviceName = model.device.caption;
                        Sync.syncEnabled = true;
                        syncGroupOverlay.open();
                    }
                }
            }
        }

        function update() {
            deviceListModel.clear();
            for (var index in Sync.deviceList) {
                deviceListModel.append({"device": Sync.deviceList[index]});
            }
        }
    }

    Connections {
        target: Sync
        function onDeviceCreated() {
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

        title: i18n("Device Sync Settings")

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
                text: i18n("Should all podcast subscriptions on this gpodder.net account be synced across all devices?\nIf you don't know what this means, you should probably select \"Ok\".")
                color: Kirigami.Theme.textColor
                Keys.onReturnPressed: accepted();
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

        title: i18n("Sync Now?")

        onAccepted: {
            firstSyncOverlay.close();
            Sync.doRegularSync();
        }
        onRejected: firstSyncOverlay.close();

        RowLayout {
            spacing: Kirigami.Units.largeSpacing
            Kirigami.Icon {
                Layout.preferredHeight: Kirigami.Units.gridUnit * 4
                Layout.preferredWidth: Kirigami.Units.gridUnit * 4
                source: Sync.provider === Sync.GPodderNextcloud ? "kaccounts-nextcloud" : "gpodder"
            }
            TextEdit {
                Layout.fillWidth: true
                Layout.fillHeight: true
                readOnly: true
                wrapMode: Text.WordWrap
                text: i18n("Perform a first sync now?")
                color: Kirigami.Theme.textColor
                Keys.onReturnPressed: accepted();
            }
        }
    }
}
