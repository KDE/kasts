/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#ifndef Q_OS_ANDROID
#include <QSystemTrayIcon>
#endif

class SystrayIcon
#ifndef Q_OS_ANDROID
    : public QSystemTrayIcon
#else
    : public QObject
#endif
{
    Q_OBJECT

public:
    Q_PROPERTY(bool available READ available CONSTANT)

    static SystrayIcon &instance()
    {
        static SystrayIcon _instance;
        return _instance;
    }

    ~SystrayIcon() override;

    [[nodiscard]] bool available() const;

Q_SIGNALS:
    void raiseWindow();

private:
    explicit SystrayIcon(QObject *parent = nullptr);
};
