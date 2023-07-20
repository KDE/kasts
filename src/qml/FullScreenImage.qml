/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.components 1.0 as Addons

Addons.AlbumMaximizeComponent {
    id: root

    required property var image
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
