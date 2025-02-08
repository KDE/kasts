/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.labs.components as Addons

Addons.AlbumMaximizeComponent {
    id: root

    required property string image
    required property QtObject loader
    property string description: undefined

    property list<Addons.AlbumModelItem> mymodel: [
        Addons.AlbumModelItem {
            type: Addons.AlbumModelItem.Image
            source: image
            // tempSource: "path/to/tempSource"
            caption: description
        }
    ]

    initialIndex: 0
    model: mymodel

    onClosed: {
        loader.active = false;
    }
}
