/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>
#include <Syndication/Syndication>

class Fetcher : public QObject
{
    Q_OBJECT
public:
    static Fetcher &instance()
    {
        static Fetcher _instance;
        return _instance;
    }
    Q_INVOKABLE void fetch(const QString &url);
    Q_INVOKABLE void fetchAll();
    Q_INVOKABLE QString image(const QString &url);
    void removeImage(const QString &url);
    Q_INVOKABLE QNetworkReply *download(QString url);
    QNetworkReply *get(QNetworkRequest &request);
    QString filePath(const QString &url);

private:
    Fetcher();

    void processFeed(Syndication::FeedPtr feed, const QString &url);
    void processEntry(Syndication::ItemPtr entry, const QString &url);
    void processAuthor(Syndication::PersonPtr author, const QString &entryId, const QString &url);
    void processEnclosure(Syndication::EnclosurePtr enclosure, Syndication::ItemPtr entry, const QString &feedUrl);


    QNetworkAccessManager *manager;

Q_SIGNALS:
    void startedFetchingFeed(const QString &url);
    void feedUpdated(const QString &url);
    void feedDetailsUpdated(const QString &url, const QString &name, const QString &image, const QString &link, const QString &description, const QDateTime &lastUpdated);
    void error(const QString &url, int errorId, const QString &errorString);
    void entryAdded(const QString &feedurl, const QString &id);
    void downloadFinished(QString url);
};
