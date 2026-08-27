/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/entriesproxymodel.h"

#include <QModelIndex>
#include <QSqlQuery>
#include <QString>
#include <QTimer>

#include "database.h"
#include "models/entriesmodel.h"
#include "models/entriesproxymodellogging.h"
#include "objectslogging.h"

EntriesProxyModel::EntriesProxyModel(const qint64 feeduid, QObject *parent)
    : AbstractEpisodeProxyModel(parent)
{
    m_entriesModel = new EntriesModel(feeduid, this);
    setSourceModel(m_entriesModel);

    // restore saved filter and sorting
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT filterType, sortType FROM Feeds WHERE feeduid=:feeduid;"));
    query.bindValue(QStringLiteral(":feeduid"), feeduid);
    Database::instance().execute(query);
    if (query.next()) {
        AbstractEpisodeProxyModel::FilterType filterType = AbstractEpisodeProxyModel::FilterType(query.value(QStringLiteral("filterType")).toInt());
        AbstractEpisodeProxyModel::SortType sortType = AbstractEpisodeProxyModel::SortType(query.value(QStringLiteral("sortType")).toInt());

        if (filterType != this->filterType()) {
            setFilterType(filterType);
        }
        if (sortType != this->sortType()) {
            setSortType(sortType);
        }

        // save filter to db when changed
        connect(this, &EntriesProxyModel::filterTypeChanged, this, [this, feeduid]() {
            qCDebug(kastsEntriesProxyModel) << "Save filter type to database for feed" << feeduid;
            int filterTypeValue = static_cast<int>(this->filterType());

            Database::instance().transaction();
            QSqlQuery writeQuery;
            writeQuery.prepare(QStringLiteral("UPDATE Feeds SET filterType=:filterType WHERE feeduid=:feeduid;"));
            writeQuery.bindValue(QStringLiteral(":feeduid"), feeduid);
            writeQuery.bindValue(QStringLiteral(":filterType"), filterTypeValue);
            Database::instance().execute(writeQuery);
            Database::instance().commit();
        });

        // save sort to db when changed
        connect(this, &EntriesProxyModel::sortTypeChanged, this, [this, feeduid]() {
            qCDebug(kastsEntriesProxyModel) << "Save sort type to database for feed" << feeduid;
            int sortTypeValue = static_cast<int>(this->sortType());

            Database::instance().transaction();
            QSqlQuery writeQuery;
            writeQuery.prepare(QStringLiteral("UPDATE Feeds SET sortType=:sortType WHERE feeduid=:feeduid;"));
            writeQuery.bindValue(QStringLiteral(":feeduid"), feeduid);
            writeQuery.bindValue(QStringLiteral(":sortType"), sortTypeValue);
            Database::instance().execute(writeQuery);
            Database::instance().commit();
        });
    }
    qCDebug(kastsObjects) << "EntriesProxyModel constructed" << feeduid;
}

EntriesProxyModel::~EntriesProxyModel()
{
    qCDebug(kastsObjects) << "EntriesProxyModel destructed" << m_entriesModel;
}
