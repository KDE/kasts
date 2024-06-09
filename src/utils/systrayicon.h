/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QQmlEngine>
#ifndef Q_OS_ANDROID
#include <QSystemTrayIcon>
#endif

class SystrayIcon : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum IconColor {
        Colorful,
        Light,
        Dark,
    };
    Q_ENUM(IconColor)

    Q_PROPERTY(bool available READ available CONSTANT)

    static SystrayIcon &instance()
    {
        static SystrayIcon _instance;
        return _instance;
    }
    static SystrayIcon *create(QQmlEngine *engine, QJSEngine *)
    {
        engine->setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
        return &instance();
    }

    [[nodiscard]] bool available() const;

    void setIconColor(IconColor iconColor);

Q_SIGNALS:
    void raiseWindow();

private:
#ifndef Q_OS_ANDROID
    QSystemTrayIcon m_trayIcon;
#endif

    SystrayIcon();
    int iconColorEnumToInt(IconColor iconColor);
    IconColor intToIconColorEnum(int iconColorCode);
};
