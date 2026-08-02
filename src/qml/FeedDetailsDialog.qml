// SPDX-FileCopyrightText: 2020-2026 Tobias Fella <tobias.fella@kde.org>
// SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kasts

Controls.Dialog {
    id: root

    width: Math.min(implicitWidth, Kirigami.Units.gridUnit * 24)

    header: RowLayout {
        Kirigami.Heading {
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.topMargin: Kirigami.Units.largeSpacing
            text: feed.name
            elide: Qt.ElideRight
            Layout.fillWidth: true
        }
        Controls.ToolButton {
            Layout.alignment: Qt.AlignRight
            Layout.rightMargin: Kirigami.Units.largeSpacing
            Layout.topMargin: Kirigami.Units.largeSpacing
            icon.name: "dialog-close"
            onClicked: root.close()
        }
    }

    contentItem: Controls.Control {
        leftPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
        rightPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
        topPadding: Kirigami.Units.largeSpacing
        bottomPadding: Kirigami.Units.largeSpacing

        // HACK: opening more info changes the position of the header
        onHeightChanged: entryList.contentY = entryList.originY

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            Controls.Label {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                textFormat: root.showMoreInfo ? TextEdit.RichText : Text.StyledText
                text: feed.description
                font.pointSize: Kirigami.Theme.defaultFont.pointSize
                wrapMode: Text.Wrap
                color: Kirigami.Theme.textColor
                lineHeight: 1.2
            }

            RowLayout {
                id: feedUrlLayout
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: Kirigami.Units.smallSpacing
                Controls.Label {
                    Layout.alignment: Qt.AlignTop
                    textFormat: TextEdit.RichText
                    text: KI18n.i18n("Podcast URL:")
                    wrapMode: TextEdit.Wrap
                }
                Kirigami.UrlButton {
                    id: feedUrl
                    Layout.alignment: Qt.AlignTop
                    url: feed.url
                    wrapMode: TextEdit.Wrap
                    horizontalAlignment: Text.AlignLeft
                    Layout.fillWidth: true
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: Kirigami.Units.smallSpacing
                Controls.Label {
                    Layout.alignment: Qt.AlignTop
                    textFormat: TextEdit.RichText
                    text: KI18n.i18n("Weblink:")
                    wrapMode: TextEdit.Wrap
                }

                Kirigami.UrlButton {
                    Layout.alignment: Qt.AlignTop
                    url: feed.link
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                }
            }
            Kirigami.SelectableLabel {
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true

                selectByMouse: !Kirigami.Settings.isMobile
                textFormat: TextEdit.RichText
                text: isSubscribed ? KI18n.i18n("Subscribed since: %1", feed.subscribed.toLocaleString(Qt.locale(), Locale.ShortFormat)) : ""
                wrapMode: Text.WordWrap
            }
            Kirigami.SelectableLabel {
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true

                selectByMouse: !Kirigami.Settings.isMobile
                textFormat: TextEdit.RichText
                text: isSubscribed ? KI18n.i18n("Last updated: %1", feed.lastUpdated.toLocaleString(Qt.locale(), Locale.ShortFormat)) : ""
                wrapMode: Text.WordWrap
            }
            Kirigami.SelectableLabel {
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true

                selectByMouse: !Kirigami.Settings.isMobile
                textFormat: TextEdit.RichText
                text: KI18n.i18np("1 Episode", "%1 Episodes", feed.entryCount) + ", " + KI18n.i18np("1 Unplayed", "%1 Unplayed", feed.unreadEntryCount)
                wrapMode: Text.WordWrap
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
