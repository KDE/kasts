/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/errorlogmodel.h"

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
        Error *error = new Error(Error::dbToType(query.value(QStringLiteral("type")).toInt()),
                                 query.value(QStringLiteral("feedid")).toInt(),
                                 query.value(QStringLiteral("entryid")).toInt(),
                                 query.value(QStringLiteral("code")).toInt(),
                                 query.value(QStringLiteral("message")).toString(),
                                 QDateTime::fromSecsSinceEpoch(query.value(QStringLiteral("date")).toInt()),
                                 query.value(QStringLiteral("title")).toString());
        m_errors += error;

        connect(&Database::instance(), &Database::error, this, &ErrorLogModel::monitorErrorMessages);
    }
}

QVariant ErrorLogModel::data(const QModelIndex &index, int role) const
{
    if (role != 0) {
        return QVariant();
    }
    return QVariant::fromValue(m_errors[index.row()]);
}

QHash<int, QByteArray> ErrorLogModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[0] = "error";
    return roleNames;
}

int ErrorLogModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_errors.count();
}

void ErrorLogModel::monitorErrorMessages(const Error::Type type,
                                         const int feedid,
                                         const int entryid,
                                         const int errorCode,
                                         const QString &errorString,
                                         const QString &title)
{
    qDebug() << "Error happened:" << type << feedid << entryid << errorCode << errorString;

    Error *error = new Error(type, feedid, entryid, errorCode, errorString, QDateTime::currentDateTime(), title);
    beginInsertRows(QModelIndex(), 0, 0);
    m_errors.prepend(error);
    endInsertRows();

    // Also add error to database
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO Errors VALUES (:type, :feedid, :entryid, :code, :message, :date, :title);"));
    query.bindValue(QStringLiteral(":type"), Error::typeToDb(type));
    query.bindValue(QStringLiteral(":feedid"), feedid);
    query.bindValue(QStringLiteral(":entryid"), entryid);
    query.bindValue(QStringLiteral(":code"), errorCode);
    query.bindValue(QStringLiteral(":message"), errorString);
    query.bindValue(QStringLiteral(":date"), error->date.toSecsSinceEpoch());
    query.bindValue(QStringLiteral(":title"), title);
    Database::executeThread(query); // use this call to avoid an infinite loop when an error occurs

    // Send signal to display inline error message
    Q_EMIT newErrorLogged(error);
}

void ErrorLogModel::clearAll()
{
    beginResetModel();
    for (auto &error : m_errors) {
        delete error;
    }
    m_errors.clear();
    endResetModel();

    // Also clear errors from database
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM Errors;"));
    Database::instance().execute(query);
}
