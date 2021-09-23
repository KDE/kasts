/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KJob>
#include <QNetworkReply>
#include <QString>

#include <Syndication/Syndication>

#include "enclosure.h"

class UpdateFeedJob : public KJob
{
    Q_OBJECT

public:
    explicit UpdateFeedJob(const QString &url, QObject *parent = nullptr);

    void start() override;
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
    void feedUpdateStatusChanged(const QString &url, bool status);
    void aborting();

private:
    void retrieveFeed();
    void processFeed(Syndication::FeedPtr feed);
    bool processEntry(Syndication::ItemPtr entry);
    void processAuthor(const QString &entryId, const QString &authorName, const QString &authorUri, const QString &authorEmail);
    void processEnclosure(Syndication::EnclosurePtr enclosure, Syndication::ItemPtr entry);
    void processChapter(const QString &entryId, const int &start, const QString &chapterTitle, const QString &link, const QString &image);
    void writeToDatabase();

    bool m_abort = false;

    QString m_url;
    QNetworkReply *m_reply = nullptr;

    bool m_isNewFeed;
    QVector<EntryDetails> m_entries;
    QVector<AuthorDetails> m_authors;
    QVector<EnclosureDetails> m_enclosures;
    QVector<ChapterDetails> m_chapters;

    QStringList m_existingEntryIds;
    QVector<QPair<QString, QString>> m_existingEnclosures;
    QVector<QPair<QString, QString>> m_existingAuthors;
    QVector<QPair<QString, int>> m_existingChapters;
};
