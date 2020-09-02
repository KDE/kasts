/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QDebug>
#include <QModelIndex>
#include <QSqlQuery>
#include <QUrl>
#include <QVariant>

#include "database.h"
#include "feedListModel.h"
#include "fetcher.h"

FeedListModel::FeedListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&Database::instance(), &Database::feedAdded, this, [this]() {
        beginInsertRows(QModelIndex(), rowCount(QModelIndex()) - 1, rowCount(QModelIndex()) - 1);
        endInsertRows();
    });
    connect(&Fetcher::instance(), &Fetcher::feedDetailsUpdated, this, [this](QString url, QString name, QString image, QString link, QString description, QDateTime lastUpdated) {
        for (int i = rowCount(QModelIndex()) - 1; i >= 0; i--) {
            if (m_feeds[i]->url() == url) {
                m_feeds[i]->setName(name);
                m_feeds[i]->setImage(image);
                m_feeds[i]->setLink(link);
                m_feeds[i]->setDescription(description);
                m_feeds[i]->setLastUpdated(lastUpdated);
                Q_EMIT dataChanged(createIndex(i, 0), createIndex(i, 0));
                break;
            }
        }
    });
}

QHash<int, QByteArray> FeedListModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[0] = "feed";
    return roleNames;
}

int FeedListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT() FROM Feeds;"));
    Database::instance().execute(query);
    if (!query.next())
        qWarning() << "Failed to query feed count";
    return query.value(0).toInt();
}

QVariant FeedListModel::data(const QModelIndex &index, int role) const
{
    if (role != 0)
        return QVariant();
    if (m_feeds.length() <= index.row())
        loadFeed(index.row());
    return QVariant::fromValue(m_feeds[index.row()]);
}

void FeedListModel::loadFeed(int index) const
{
    m_feeds += new Feed(index);
}

void FeedListModel::removeFeed(int index)
{
    m_feeds[index]->remove();
    delete m_feeds[index];
    beginRemoveRows(QModelIndex(), index, index);
    m_feeds.removeAt(index);
    endRemoveRows();
}

void FeedListModel::refreshAll()
{
    for (auto &feed : m_feeds) {
        feed->refresh();
    }
}
