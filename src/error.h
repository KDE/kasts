/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include "datamanager.h"

class Error : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString url MEMBER url CONSTANT)
    Q_PROPERTY(QString id MEMBER id CONSTANT)
    Q_PROPERTY(int code MEMBER code CONSTANT)
    Q_PROPERTY(QString string MEMBER string CONSTANT)
    Q_PROPERTY(QDateTime date MEMBER date CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)

public:
    Error(const QString url, const QString id, const int code, const QString string, const QDateTime date): QObject(nullptr)
    {
        this->url = url;
        this->id = id;
        this->code = code;
        this->string = string;
        this->date = date;
    };

    QString url;
    QString id;
    int code;
    QString string;
    QDateTime date;

    QString title () {
        QString title;
        if (!id.isEmpty()) {
            if (DataManager::instance().getEntry(id))
                title = DataManager::instance().getEntry(id)->title();
        } else if (!url.isEmpty()) {
            if (DataManager::instance().getFeed(url))
                title = DataManager::instance().getFeed(url)->name();
        }
        return title;
    }
};
