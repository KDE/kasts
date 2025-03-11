/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <qqmlintegration.h>

class Error : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum Type {
        Unknown = -1,
        FeedUpdate = 0,
        MediaDownload,
        MeteredNotAllowed,
        InvalidMedia,
        DiscoverError,
        StorageMoveError,
        SyncError,
        MeteredStreamingNotAllowed,
        NoNetwork,
        Database,
    };
    Q_ENUM(Type)

    static int typeToDb(Type type); // needed to translate Error::Type values to int for sqlite
    static Type dbToType(int value); // needed to translate from int to Error::Type values for sqlite

    Q_PROPERTY(QString url MEMBER url CONSTANT)
    Q_PROPERTY(QString id MEMBER id CONSTANT)
    Q_PROPERTY(int code MEMBER code CONSTANT)
    Q_PROPERTY(QString message MEMBER message CONSTANT)
    Q_PROPERTY(QDateTime date MEMBER date CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)

    Error(Type type,
          const QString &url,
          const QString &id,
          const int code,
          const QString &message,
          const QDateTime &date,
          const QString &title = QLatin1String(""));

    QString title() const;
    QString description() const;

    Type type;
    QString url;
    QString id;
    int code;
    QString message;
    QDateTime date;

private:
    QString m_title;
};
