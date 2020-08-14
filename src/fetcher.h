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
    Q_INVOKABLE void fetch(QString url);
    Q_INVOKABLE void fetchAll();
    Q_INVOKABLE QString image(QString);
    void removeImage(QString);
    Q_INVOKABLE void download(QString url);
    QNetworkReply *get(QNetworkRequest &request);

private:
    Fetcher();

    QString filePath(QString);
    void processFeed(Syndication::FeedPtr feed, QString url);
    void processEntry(Syndication::ItemPtr entry, QString url);
    void processAuthor(Syndication::PersonPtr author, QString entryId, QString url);
    void processEnclosure(Syndication::EnclosurePtr enclosure, Syndication::ItemPtr entry, QString feedUrl);

    QNetworkAccessManager *manager;

Q_SIGNALS:
    void startedFetchingFeed(QString url);
    void feedUpdated(QString url);
    void feedDetailsUpdated(QString url, QString name, QString image, QString link, QString description, QDateTime lastUpdated);
    void error(QString url, int errorId, QString errorString);
    void imageDownloadFinished(QString url);
};
