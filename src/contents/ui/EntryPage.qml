import QtQuick 2.7
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.10 as Controls

import org.kde.kirigami 2.8 as Kirigami

import org.kde.alligator 1.0

Kirigami.ScrollablePage {
    id: page
    property QtObject data

    title: data.title

    ColumnLayout {
        Controls.Label {
            text: page.data.content
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
