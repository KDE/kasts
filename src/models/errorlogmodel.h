/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QQmlEngine>
#include <QVariant>

class Error;

class ErrorLogModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

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

    enum RoleNames {
        MessageRole = Qt::DisplayRole,
        DescriptionRole = Qt::UserRole + 1,
        DateRole,
    };
    Q_ENUM(RoleNames);

    static ErrorLogModel &instance()
    {
        static ErrorLogModel _instance;
        return _instance;
    }
    static ErrorLogModel *create(QQmlEngine *engine, QJSEngine *)
    {
        engine->setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
        return &instance();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;

    Q_INVOKABLE void clearAll();

public:
    void monitorErrorMessages(const ErrorLogModel::Type type, const QString &message);

Q_SIGNALS:
    void newErrorLogged(const QString &message);

private:
    struct Error {
        Type type;
        QString message;
        QDateTime date;
    };
    explicit ErrorLogModel();
    QList<ErrorLogModel::Error> m_errors;

    static int typeToDb(const Type type); // needed to translate ErrorLogModel::Type values to int for sqlite
    static Type dbToType(const int value); // needed to translate from int to ErrorLogModel::Type values for sqlite
    static QString description(const Type type);
};
