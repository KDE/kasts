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
    explicit UpdateFeedJob(const int &feedid, const QByteArray &data, const DataTypes::FeedDetails &feed, QObject *parent = nullptr);

    void run(ThreadWeaver::JobPointer, ThreadWeaver::Thread *) override;
    void abort();

Q_SIGNALS:
    void feedDetailsUpdated(const int &feedid,
                            const QString &url,
                            const QString &name,
                            const QString &image,
                            const QString &link,
                            const QString &description,
                            const QDateTime &lastUpdated,
                            const QString &dirname);
    void feedUpdated(const int &feedid);
    void entryAdded(const int &feedid, const int &entryid);
    void entryUpdated(const int &feedid, const int &entryid);
    void aborting();
    void finished();
    void error(Error::Type type, const int &feedid, const int &entryid, const int errorId, const QString &errorString, const QString &title);

private:
    void processFeed(Syndication::FeedPtr feed);
    bool processEntry(Syndication::ItemPtr entry);
    bool processFeedAuthor(const int &feedid, const QString &authorName, const QString &authorEmail);
    bool processEntryAuthor(const int &entryid, const QString &id, const QString &authorName, const QString &authorEmail);
    bool
    processEnclosure(const int &entryid, Syndication::EnclosurePtr enclosure, const DataTypes::EntryDetails &newEntry, const DataTypes::EntryDetails &oldEntry);
    bool processChapter(const int &entryid, const QString &id, const int &start, const QString &chapterTitle, const QString &link, const QString &image);
    void writeToDatabase();

    bool dbExecute(QSqlQuery &query);
    bool dbTransaction();
    bool dbCommit();

    QString generateFeedDirname(const QString &name);
    bool m_abort = false;

    int m_feedid;
    QByteArray m_data;

    bool m_markUnreadOnNewFeed;
    DataTypes::FeedDetails m_feed, m_updateFeed;
    QVector<DataTypes::EntryDetails> m_entries, m_newEntries, m_updateEntries;
    QVector<DataTypes::FeedAuthorDetails> m_feedAuthors, m_newFeedAuthors, m_updateFeedAuthors;
    QVector<DataTypes::EntryAuthorDetails> m_entryAuthors, m_newEntryAuthors, m_updateEntryAuthors;
    QVector<DataTypes::EnclosureDetails> m_enclosures, m_newEnclosures, m_updateEnclosures;
    QVector<DataTypes::ChapterDetails> m_chapters, m_newChapters, m_updateChapters;
};
