/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>

#include <Syndication/Syndication>
#include <ThreadWeaver/Job>

#include "enclosure.h"

class UpdateFeedJob : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT

public:
    explicit UpdateFeedJob(const QString &url, const QByteArray &data, QObject *parent = nullptr);
    ~UpdateFeedJob();

    void run(ThreadWeaver::JobPointer, ThreadWeaver::Thread *) override;
    void abort();

    struct EntryDetails {
        QString feed;
        QString id;
        QString title;
        QString content;
        int created;
        int updated;
        QString link;
        bool read;
        bool isNew;
        bool hasEnclosure;
        QString image;
    };

    struct AuthorDetails {
        QString feed;
        QString id;
        QString name;
        QString uri;
        QString email;
    };

    struct EnclosureDetails {
        QString feed;
        QString id;
        int duration;
        int size;
        QString title;
        QString type;
        QString url;
        int playPosition;
        Enclosure::Status downloaded;
    };

    struct ChapterDetails {
        QString feed;
        QString id;
        int start;
        QString title;
        QString link;
        QString image;
    };

Q_SIGNALS:
    void feedDetailsUpdated(const QString &url,
                            const QString &name,
                            const QString &image,
                            const QString &link,
                            const QString &description,
                            const QDateTime &lastUpdated);
    void feedUpdated(const QString &url);
    void entryAdded(const QString &feedurl, const QString &id);
    void entryUpdated(const QString &feedurl, const QString &id);
    void feedUpdateStatusChanged(const QString &url, bool status);
    void aborting();
    void finished();

private:
    void processFeed(Syndication::FeedPtr feed);
    bool processEntry(Syndication::ItemPtr entry);
    bool processAuthor(const QString &entryId, const QString &authorName, const QString &authorUri, const QString &authorEmail);
    bool processEnclosure(Syndication::EnclosurePtr enclosure, Syndication::ItemPtr entry);
    bool processChapter(const QString &entryId, const int &start, const QString &chapterTitle, const QString &link, const QString &image);
    void writeToDatabase();

    bool m_abort = false;

    QString m_url;
    QByteArray m_data;

    bool m_isNewFeed;
    QVector<EntryDetails> m_entries, m_newEntries, m_updateEntries;
    QVector<AuthorDetails> m_authors, m_newAuthors, m_updateAuthors;
    QVector<EnclosureDetails> m_enclosures, m_newEnclosures, m_updateEnclosures;
    QVector<ChapterDetails> m_chapters, m_newChapters, m_updateChapters;
};
