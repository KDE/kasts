/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/errorlogmodel.h"

#include <KLocalizedString>
#include <QDebug>
#include <QSqlQuery>

#include "database.h"

ErrorLogModel::ErrorLogModel()
    : QAbstractListModel(nullptr)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Errors ORDER BY date DESC;"));
    Database::instance().execute(query);
    while (query.next()) {
        ErrorLogModel::Error error;
        error.type = ErrorLogModel::dbToType(query.value(QStringLiteral("type")).toInt());
        error.message = query.value(QStringLiteral("message")).toString();
        error.date = QDateTime::fromSecsSinceEpoch(query.value(QStringLiteral("date")).toInt());
        m_errors += error;

        connect(&Database::instance(), &Database::error, this, &ErrorLogModel::monitorErrorMessages);
    }
}

QVariant ErrorLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    int row = index.row();
    if (row >= 0 && row < m_errors.size()) {
        switch (role) {
        case MessageRole:
            return QVariant::fromValue(m_errors[index.row()].message);
        case DescriptionRole:
            return QVariant::fromValue(description(m_errors[index.row()].type));
        case DateRole:
            return QVariant::fromValue(m_errors[index.row()].date);

        default:
            return QVariant();
        }
    } else {
        return QVariant();
    }
}

QHash<int, QByteArray> ErrorLogModel::roleNames() const
{
    return {
        {MessageRole, "message"},
        {DescriptionRole, "description"},
        {DateRole, "date"},
    };
}

int ErrorLogModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_errors.count();
}

void ErrorLogModel::monitorErrorMessages(const ErrorLogModel::Type type, const QString &message)
{
    qDebug() << "Error happened:" << type << message;

    ErrorLogModel::Error error;
    error.type = type;
    error.message = message;
    error.date = QDateTime::currentDateTime();
    beginInsertRows(QModelIndex(), 0, 0);
    m_errors.prepend(error);
    endInsertRows();

    // Also add error to database
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO Errors (type, message, date) VALUES (:type, :message, :date);"));
    query.bindValue(QStringLiteral(":type"), ErrorLogModel::typeToDb(type));
    query.bindValue(QStringLiteral(":message"), message);
    query.bindValue(QStringLiteral(":date"), error.date.toSecsSinceEpoch());
    Database::executeThread(query); // use this call to avoid an infinite loop when an error occurs

    // Send signal to display inline error message
    Q_EMIT newErrorLogged(message);
}

void ErrorLogModel::clearAll()
{
    beginResetModel();
    m_errors.clear();
    endResetModel();

    // Also clear errors from database
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM Errors;"));
    Database::instance().execute(query);
}

QString ErrorLogModel::description(const ErrorLogModel::Type type)
{
    switch (type) {
    case ErrorLogModel::Type::FeedUpdate:
        return i18n("Podcast update error");
    case ErrorLogModel::Type::MediaDownload:
        return i18n("Media download error");
    case ErrorLogModel::Type::MeteredNotAllowed:
        return i18n("Update not allowed on metered connection");
    case ErrorLogModel::Type::InvalidMedia:
        return i18n("Invalid media file");
    case ErrorLogModel::Type::DiscoverError:
        return i18n("Nothing found");
    case ErrorLogModel::Type::StorageMoveError:
        return i18n("Error moving storage path");
    case ErrorLogModel::Type::SyncError:
        return i18n("Error syncing feed and/or episode status");
    case ErrorLogModel::Type::MeteredStreamingNotAllowed:
        return i18n("Streaming not allowed on metered connection");
    case ErrorLogModel::Type::NoNetwork:
        return i18n("No network connection");
    case ErrorLogModel::Type::Database:
        return i18n("Database error");
    default:
        return QString();
    }
}

int ErrorLogModel::typeToDb(const ErrorLogModel::Type type)
{
    return static_cast<int>(type);
}

ErrorLogModel::Type ErrorLogModel::dbToType(const int value)
{
    return ErrorLogModel::Type(value);
}
