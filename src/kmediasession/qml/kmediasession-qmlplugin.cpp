/**
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>

#include <kmediasession.h>
#include <metadata.h>

class KMediaSessionQmlPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")
    void registerTypes(const char *uri) override;
};

void KMediaSessionQmlPlugin::registerTypes(const char *)
{
    qmlRegisterType<KMediaSession>("org.kde.kmediasession", 1, 0, "KMediaSession");
    qmlRegisterType<MetaData>("org.kde.kmediasession", 1, 0, "MetaData");

    qRegisterMetaType<KMediaSession::MediaBackends>("KMediaSession::MediaBackends");
    qRegisterMetaType<QList<KMediaSession::MediaBackends>>("QList<KMediaSession::MediaBackends>");
}

#include "kmediasession-qmlplugin.moc"
