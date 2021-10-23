/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>

#include <solidextras/networkstatus.h>

class SolidExtrasQmlPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")
    void registerTypes(const char* uri) override;
};

using namespace SolidExtras;

void SolidExtrasQmlPlugin::registerTypes(const char*)
{
    qmlRegisterSingletonType<NetworkStatus>("org.kde.kasts.solidextras", 1, 0, "NetworkStatus", [](QQmlEngine *, QJSEngine *) -> QObject * {
        return new NetworkStatus;
    });
}

#include "solidextrasqmlplugin.moc"
