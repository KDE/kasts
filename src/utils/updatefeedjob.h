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
    explicit UpdateFeedJob(const QByteArray &data, const DataTypes::FeedDetails &oldFeedDetails, QObject *parent = nullptr);

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
    void resultsAvailable(DataTypes::FeedDetails feedDetails);
    void error(Error::Type type, const QString &url, const QString &id, const int errorId, const QString &errorString, const QString &title);

private:
    void processFeed(const Syndication::FeedPtr feed);
    void processEntry(const Syndication::ItemPtr feedEntry, QHash<QString, DataTypes::EntryDetails> &entries);
    void processAuthor(const QString &name, const QString &email, QHash<QString, DataTypes::AuthorDetails> &authors);
    void processEnclosure(const Syndication::EnclosurePtr enclosure,
                          const QString &oldEntryTitle,
                          const QString &newEntryTitle,
                          QHash<QString, DataTypes::EnclosureDetails> &enclosures);
    void processChapter(const int &start, const QString &title, const QString &link, const QString &image, QHash<int, DataTypes::ChapterDetails> &chapters);
    void writeToDatabase();

    bool dbExecute(QSqlQuery &query);
    bool dbTransaction();
    bool dbCommit();

    QString generateFeedDirname(const QString &name);
    bool m_abort = false;

    QString m_url;
    QByteArray m_data;

    bool m_markUnreadOnNewFeed;
    DataTypes::FeedDetails m_oldFeedDetails, m_feedDetails;
};

Q_DECLARE_METATYPE(DataTypes::FeedDetails)
