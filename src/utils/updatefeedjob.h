/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSqlQuery>
#include <QString>

#include <Syndication/Syndication>
#include <ThreadWeaver/Job>

#include "datatypes.h"
#include "error.h"

class UpdateFeedJob : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT

public:
    explicit UpdateFeedJob(const QString &url, const QByteArray &data, const DataTypes::FeedDetails &feed, QObject *parent = nullptr);

    void run(ThreadWeaver::JobPointer, ThreadWeaver::Thread *) override;
    void abort();

Q_SIGNALS:
    void feedDetailsUpdated(const QString &url,
                            const QString &name,
                            const QString &image,
                            const QString &link,
                            const QString &description,
                            const QDateTime &lastUpdated,
                            const QString &dirname);
    void feedUpdated(const QString &url);
    void entryAdded(const QString &feedurl, const QString &id);
    void entryUpdated(const QString &feedurl, const QString &id);
    void aborting();
    void finished();
    void error(Error::Type type, const QString &url, const QString &id, const int errorId, const QString &errorString, const QString &title);

private:
    void processFeed(Syndication::FeedPtr feed);
    bool processEntry(Syndication::ItemPtr entry);
    bool processAuthor(const QString &entryId, const QString &authorName, const QString &authorUri, const QString &authorEmail);
    bool processEnclosure(Syndication::EnclosurePtr enclosure, const DataTypes::EntryDetails &newEntry, const DataTypes::EntryDetails &oldEntry);
    bool processChapter(const QString &entryId, const int &start, const QString &chapterTitle, const QString &link, const QString &image);
    void writeToDatabase();

    bool dbExecute(QSqlQuery &query);
    bool dbTransaction();
    bool dbCommit();

    QString generateFeedDirname(const QString &name);
    bool m_abort = false;

    QString m_url;
    QByteArray m_data;

    bool m_markUnreadOnNewFeed;
    DataTypes::FeedDetails m_feed, m_updateFeed;
    QVector<DataTypes::EntryDetails> m_entries, m_newEntries, m_updateEntries;
    QVector<DataTypes::AuthorDetails> m_authors, m_newAuthors, m_updateAuthors;
    QVector<DataTypes::EnclosureDetails> m_enclosures, m_newEnclosures, m_updateEnclosures;
    QVector<DataTypes::ChapterDetails> m_chapters, m_newChapters, m_updateChapters;
};
