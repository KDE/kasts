/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QAbstractListModel>
#include <QVariant>
#include <QVector>
#include <QSqlQuery>
#include <QFile>

#include "database.h"
#include "queuemodel.h"
#include "fetcher.h"

QueueModel::QueueModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&Fetcher::instance(), &Fetcher::downloadFinished, this, [this](const QString &url) {
        beginResetModel();
        for (auto &entry : m_entries) {
            delete entry;
        }
        m_entries.clear();
        updateQueue();
        endResetModel();
    });

    updateQueue();
}

void QueueModel::updateQueue()
{

/*
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Enclosures"));
    Database::instance().execute(query);

    while (query.next()) {
        int feed_index = -1;
        int entry_index = -1;

        QString feedurl = query.value(QStringLiteral("feed")).toString();
        QString id = query.value(QStringLiteral("id")).toString();
        int duration = query.value(QStringLiteral("duration")).toInt();
        int size = query.value(QStringLiteral("size")).toInt();
        QString title = query.value(QStringLiteral("title")).toString();
        QString type = query.value(QStringLiteral("type")).toString();
        QString url = query.value(QStringLiteral("url")).toString();

        QFile file(Fetcher::instance().filePath(url));
        if(file.size() == size) {

            // Enclosure is in database and file has been downloaded !
            // Let's find the feed and entry index value so we can create
            // an entry object
            QSqlQuery feedQuery;
            feedQuery.prepare(QStringLiteral("SELECT rowid FROM Feeds WHERE url=:feedurl"));
            feedQuery.bindValue(QStringLiteral(":feedurl"), feedurl);
            Database::instance().execute(feedQuery);
            if (!feedQuery.next()) {
                qWarning() << "Feed not found:" << feedurl;
                // TODO: remove enclosures belonging to non-existent feed
            }
            feed_index = feedQuery.value(QStringLiteral("rowid")).toInt() - 1;
            qDebug() << feed_index << feedurl;

            // Find query index
            QSqlQuery entryQuery;
            entryQuery.prepare(QStringLiteral("SELECT id FROM Entries WHERE feed=:feedurl;"));
            entryQuery.bindValue(QStringLiteral(":feedurl"), feedurl);
            entryQuery.bindValue(QStringLiteral(":id"), id);
            Database::instance().execute(entryQuery);
            int counter = -1;
            while (entryQuery.next()) {
                counter++;
                QString idquery = entryQuery.value(QStringLiteral("id")).toString();
                if (idquery == id) entry_index = counter;
            }
            qDebug() << entry_index << id;

        } else {
            // TODO: there is some problem with the already downloaded file
            // should probably delete the file (and reset status probably not needed)
        }
        if ((feed_index > -1) && (entry_index > -1)) {
            Feed *feed = new Feed(feed_index);
            Entry *entry = new Entry(feed, entry_index);
            m_entries.append(entry);
        }
    }
    */
}

QueueModel::~QueueModel()
{
    qDeleteAll(m_entries);
}

QVariant QueueModel::data(const QModelIndex &index, int role) const
{
    if (role != 0)
        return QVariant();
    if (m_entries[index.row()] == nullptr)
        return QVariant();
    return QVariant::fromValue(m_entries[index.row()]);
}

QHash<int, QByteArray> QueueModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[0] = "entry";
    return roleNames;
}

int QueueModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_entries.size();
}

bool QueueModel::move(int from, int to)
{
    return moveRows(QModelIndex(), from, 1, QModelIndex(), to);
}

bool QueueModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild)
{
    Q_ASSERT(count == 1);  // Only implemented the case of moving one row at a time
    Q_ASSERT(sourceParent == destinationParent); // No moving between lists

    int to = (sourceRow < destinationChild) ? destinationChild + 1 : destinationChild;

    if (!beginMoveRows(sourceParent, sourceRow, sourceRow, destinationParent, to)) {
        return false;
    }

    m_entries.move(sourceRow, destinationChild);

    endMoveRows();

    return true;
}

void QueueModel::addEntry(QString feedurl, QString id) {
    qDebug() << feedurl << id;

    QSqlQuery feedQuery;
    feedQuery.prepare(QStringLiteral("SELECT * FROM Feeds WHERE url=:feedurl"));
    feedQuery.bindValue(QStringLiteral(":feedurl"), feedurl);
    Database::instance().execute(feedQuery);
    if (!feedQuery.next()) {
        qWarning() << "Feed not found:" << feedurl;
        // TODO: remove enclosures belonging to non-existent feed
        return;
    }
    int feed_index = feedQuery.value(QStringLiteral("rowid")).toInt() - 1;
    qDebug() << feed_index << feedurl;
    Feed* feed = new Feed(feed_index);

    // Find query index
    QSqlQuery entryQuery;
    entryQuery.prepare(QStringLiteral("SELECT rowid FROM Entries WHERE feed=:feedurl ORDER BY updated;"));
    entryQuery.bindValue(QStringLiteral(":feedurl"), feedurl);
    Database::instance().execute(entryQuery);
    int counter = -1;
    int entry_index = -1;
    while (entryQuery.next()) {
        counter++;
        QString idquery = entryQuery.value(QStringLiteral("id")).toString();
        if (idquery == id) entry_index = counter;
    }
    if (entry_index == -1) return;

    beginInsertRows(QModelIndex(), rowCount(QModelIndex()) - 1, rowCount(QModelIndex()) - 1);
    m_entries.append(new Entry(feed, entry_index));
    endInsertRows();
    return;
}
