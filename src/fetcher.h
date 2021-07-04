/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QUrl>
#include <Syndication/Syndication>

#include "error.h"

#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
#include "NMinterface.h"
#endif

class Fetcher : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int updateProgress MEMBER m_updateProgress NOTIFY updateProgressChanged)
    Q_PROPERTY(int updateTotal MEMBER m_updateTotal NOTIFY updateTotalChanged)
    Q_PROPERTY(bool updating MEMBER m_updating NOTIFY updatingChanged)

public:
    static Fetcher &instance()
    {
        static Fetcher _instance;
        return _instance;
    }

    Q_INVOKABLE void fetch(const QString &url);
    Q_INVOKABLE void fetch(const QStringList &urls);
    Q_INVOKABLE void fetchAll();
    Q_INVOKABLE QString image(const QString &url) const;
    Q_INVOKABLE QNetworkReply *download(const QString &url, const QString &fileName) const;

    QNetworkReply *get(QNetworkRequest &request) const;

    // Network status related methods
    Q_INVOKABLE bool canCheckNetworkStatus() const;
    bool networkConnected() const;
    Q_INVOKABLE bool isMeteredConnection() const;

Q_SIGNALS:
    void startedFetchingFeed(const QString &url);
    void feedUpdated(const QString &url);
    void feedDetailsUpdated(const QString &url,
                            const QString &name,
                            const QString &image,
                            const QString &link,
                            const QString &description,
                            const QDateTime &lastUpdated);
    void feedUpdateFinished(const QString &url);
    void error(Error::Type type, const QString &url, const QString &id, const int errorId, const QString &errorString, const QString &title);
    void entryAdded(const QString &feedurl, const QString &id);
    void downloadFinished(QString url) const;

    void updateProgressChanged(int progress);
    void updateTotalChanged(int nrOfFeeds);
    void updatingChanged(bool state);

private Q_SLOTS:
    void updateMonitor(int progress);

private:
    Fetcher();

    void retrieveFeed(const QString &url);
    void processFeed(Syndication::FeedPtr feed, const QString &url);
    bool processEntry(Syndication::ItemPtr entry, const QString &url, bool isNewFeed); // returns true if this is a new entry; false if it already existed
    void processAuthor(const QString &url, const QString &entryId, const QString &authorName, const QString &authorUri, const QString &authorEmail);
    void processEnclosure(Syndication::EnclosurePtr enclosure, Syndication::ItemPtr entry, const QString &feedUrl);

    QNetworkReply *head(QNetworkRequest &request) const;
    void setHeader(QNetworkRequest &request) const;

    QNetworkAccessManager *manager;
    int m_updateProgress;
    int m_updateTotal;
    bool m_updating;

#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
    OrgFreedesktopNetworkManagerInterface *m_nmInterface;
#endif
};
