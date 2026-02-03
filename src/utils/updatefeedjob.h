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

#include <QDomElement>
#include <QList>
#include <QMultiMap>
#include <Syndication/Syndication>
#include <ThreadWeaver/Job>

#include "datatypes.h"
#include "error.h"

class UpdateFeedJob : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT

public:
    explicit UpdateFeedJob(const QString &url, const QByteArray &data, QObject *parent = nullptr);

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
    void entryAdded(const qint64 entryuid);
    void entryUpdated(const qint64 entryuid);
    void aborting();
    void finished();
    void error(Error::Type type, const QString &url, const QString &id, const int errorId, const QString &errorString, const QString &title);

private:
    void processFeed(Syndication::FeedPtr feed);
    bool processFeedAuthors(const QList<Syndication::PersonPtr> &authors, const QMultiMap<QString, QDomElement> &otherItems);
    bool processFeedAuthor(const QString &name, const QString &email);
    bool processEntry(const Syndication::ItemPtr &entry);
    bool processEntryAuthors(const QString &id, const QList<Syndication::PersonPtr> &authors, const QMultiMap<QString, QDomElement> &otherItems);
    bool processEntryAuthor(const QString &id, const QString &name, const QString &email);
    bool processChapters(const QString &id, const QMultiMap<QString, QDomElement> &otherItems, const QString &link);
    bool processEnclosures(const QString &id, const QList<Syndication::EnclosurePtr> &enclosures);
    void writeToDatabase();

    bool dbExecute(QSqlQuery &query);
    bool dbTransaction();
    bool dbCommit();

    QString generateFeedDirname(const QString &name);
    bool m_abort = false;

    QString m_url;
    QByteArray m_data;

    bool m_markUnreadOnNewFeed;

    DataTypes::FeedDetails m_feed;
};
