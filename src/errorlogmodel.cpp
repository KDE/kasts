/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "errorlogmodel.h"

#include <QSqlQuery>

#include "fetcher.h"
#include "database.h"
#include "datamanager.h"


ErrorLogModel::ErrorLogModel()
    : QAbstractListModel(nullptr)
{
    connect(&Fetcher::instance(), &Fetcher::error, this, &ErrorLogModel::monitorErrorMessages);

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Errors ORDER BY date DESC;"));
    Database::instance().execute(query);
    while (query.next()) {
        QString id = query.value(QStringLiteral("id")).toString();
        QString url = query.value(QStringLiteral("url")).toString();

        Error* error = new Error(url, id, query.value(QStringLiteral("code")).toInt(), query.value(QStringLiteral("string")).toString(), QDateTime::fromSecsSinceEpoch(query.value(QStringLiteral("date")).toInt()));
        m_errors += error;
    }
}

QVariant ErrorLogModel::data(const QModelIndex &index, int role) const
{
    if (role != 0)
        return QVariant();
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

void ErrorLogModel::monitorErrorMessages(const QString &url, const QString& id, const int errorCode, const QString& errorString)
{
    qDebug() << "Error happened:" << url << id << errorCode << errorString;
    QString title;

    Error* error = new Error(url, id, errorCode, errorString, QDateTime::currentDateTime());
    beginInsertRows(QModelIndex(), 0, 0);
    m_errors.prepend(error);
    endInsertRows();

    // Also add error to database
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO Errors VALUES (:url, :id, :code, :string, :date);"));
    query.bindValue(QStringLiteral(":url"), error->url);
    query.bindValue(QStringLiteral(":id"), error->id);
    query.bindValue(QStringLiteral(":code"), error->code);
    query.bindValue(QStringLiteral(":string"), error->string);
    query.bindValue(QStringLiteral(":date"), error->date.toSecsSinceEpoch());
    Database::instance().execute(query);
}

void ErrorLogModel::clearAll()
{
    beginResetModel();
    for (auto& error : m_errors) {
        delete error;
    }
    m_errors.clear();
    endResetModel();

    // Also clear errors from database
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM Errors;"));
    Database::instance().execute(query);
}
