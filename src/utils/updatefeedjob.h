/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QSqlQuery>
#include <QString>

#include <QDomElement>
#include <QList>
#include <QMultiMap>
#include <Syndication/Syndication>
#include <ThreadWeaver/Job>

#include "datatypes.h"
#include "models/errorlogmodel.h"

class UpdateFeedJob : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT

public:
    explicit UpdateFeedJob(const qint64 feeduid, QObject *parent = nullptr);

    void run(ThreadWeaver::JobPointer, ThreadWeaver::Thread *) override;
    void abort();

Q_SIGNALS:
    void feedDetailsUpdated(const qint64 feeduid,
                            const QString &url,
                            const QString &name,
                            const QString &image,
                            const QString &link,
                            const QString &description,
                            const QDateTime &lastUpdated,
                            const QString &dirname);
    void feedUpdated(const qint64 feeduid);
    void entriesAdded(const QList<qint64> &entryuids);
    void entriesUpdated(const QList<qint64> &entryuids);
    void aborting();
    void finished();
    void error(ErrorLogModel::Type type, const QString &message, const qint64 feeduid);

private:
    bool downloadFeed(DataTypes::FeedDetails &updatedFeed, QByteArray &data);
    void processFeed(const Syndication::FeedPtr feed, DataTypes::FeedDetails &updatedFeed, const QByteArray &data);
    bool
    processFeedAuthors(const QList<Syndication::PersonPtr> &authors, const QMultiMap<QString, QDomElement> &otherItems, DataTypes::FeedDetails &updatedFeed);
    bool processFeedAuthor(const QString &name, const QString &email, DataTypes::FeedDetails &updatedFeed);
    bool processEntry(const Syndication::ItemPtr &entry, DataTypes::FeedDetails &updatedFeed, bool markUnreadOnNew);
    bool processEntryAuthors(const QString &id,
                             const QList<Syndication::PersonPtr> &authors,
                             const QMultiMap<QString, QDomElement> &otherItems,
                             DataTypes::FeedDetails &updatedFeed);
    bool processEntryAuthor(const QString &id, const QString &name, const QString &email, DataTypes::FeedDetails &updatedFeed);
    bool processChapters(const QString &id, const QMultiMap<QString, QDomElement> &otherItems, const QString &link, DataTypes::FeedDetails &updatedFeed);
    bool processEnclosures(const QString &id, const QList<Syndication::EnclosurePtr> &enclosures, DataTypes::FeedDetails &updatedFeed);
    void writeToDatabase(DataTypes::FeedDetails &updatedFeed);

    bool dbExecute(QSqlQuery &query);
    bool dbTransaction();
    bool dbCommit();

    QString generateFeedDirname(const QString &name);
    bool m_abort = false;

    qint64 m_feeduid;
    QString m_url;
};
