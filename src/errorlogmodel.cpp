/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "errorlogmodel.h"

#include <QSqlQuery>

#include "database.h"
#include "datamanager.h"
#include "fetcher.h"

ErrorLogModel::ErrorLogModel()
    : QAbstractListModel(nullptr)
{
    connect(&Fetcher::instance(), &Fetcher::error, this, &ErrorLogModel::monitorErrorMessages);

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Errors ORDER BY date DESC;"));
    Database::instance().execute(query);
    while (query.next()) {
        Error *error = new Error(Error::dbToType(query.value(QStringLiteral("type")).toInt()),
                                 query.value(QStringLiteral("url")).toString(),
                                 query.value(QStringLiteral("id")).toString(),

                                 query.value(QStringLiteral("code")).toInt(),
                                 query.value(QStringLiteral("message")).toString(),
                                 QDateTime::fromSecsSinceEpoch(query.value(QStringLiteral("date")).toInt()));
        m_errors += error;
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

void ErrorLogModel::monitorErrorMessages(const Error::Type type, const QString &url, const QString &id, const int errorCode, const QString &errorString)
{
    qDebug() << "Error happened:" << type << url << id << errorCode << errorString;
    QString title;

    Error *error = new Error(type, url, id, errorCode, errorString, QDateTime::currentDateTime());
    beginInsertRows(QModelIndex(), 0, 0);
    m_errors.prepend(error);
    endInsertRows();

    // Also add error to database
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO Errors VALUES (:type, :url, :id, :code, :message, :date);"));
    query.bindValue(QStringLiteral(":type"), Error::typeToDb(error->type));
    query.bindValue(QStringLiteral(":url"), error->url);
    query.bindValue(QStringLiteral(":id"), error->id);
    query.bindValue(QStringLiteral(":code"), error->code);
    query.bindValue(QStringLiteral(":message"), error->message);
    query.bindValue(QStringLiteral(":date"), error->date.toSecsSinceEpoch());
    Database::instance().execute(query);

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