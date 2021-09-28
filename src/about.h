/**
 * SPDX-FileCopyrightText: 2021 Felipe Kinoshita <kinofhek@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <KAboutData>

class AboutType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(KAboutData aboutData READ aboutData CONSTANT)
public:
    static AboutType &instance()
    {
        static AboutType _instance;
        return _instance;
    }

    [[nodiscard]] KAboutData aboutData() const
    {
        return KAboutData::applicationData();
    }
};
