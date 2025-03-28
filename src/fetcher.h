/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QDateTime>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QQmlEngine>
#include <QTimer>
#include <QUrl>
#include <Syndication/Syndication>

#include "error.h"

class Fetcher : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int updateProgress MEMBER m_updateProgress NOTIFY updateProgressChanged)
    Q_PROPERTY(int updateTotal MEMBER m_updateTotal NOTIFY updateTotalChanged)
    Q_PROPERTY(bool updating MEMBER m_updating NOTIFY updatingChanged)

public:
    static Fetcher &instance()
    {
        static Fetcher _instance;
        return _instance;
    }
    static Fetcher *create(QQmlEngine *engine, QJSEngine *)
    {
        engine->setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
        return &instance();
    }

    Q_INVOKABLE void fetch(const QString &url);
    Q_INVOKABLE void fetch(const QStringList &urls);
    Q_INVOKABLE void fetchAll();
    Q_INVOKABLE QString image(const QString &url);
    Q_INVOKABLE QNetworkReply *download(const QString &url, const QString &fileName) const;
    void getRedirectedUrl(const QUrl &url);

    QNetworkReply *get(QNetworkRequest &request) const;
    QNetworkReply *post(QNetworkRequest &request, const QByteArray &data) const;

    void initializeUpdateTimer();
    void checkUpdateTimer();

    Q_INVOKABLE void setNetworkProxy();
    Q_INVOKABLE bool isSystemProxyDefined();

Q_SIGNALS:
    void entryAdded(const QString &feedurl, const QString &id);
    void entryUpdated(const QString &feedurl, const QString &id);
    void feedUpdated(const QString &url);
    void feedDetailsUpdated(const QString &url,
                            const QString &name,
                            const QString &image,
                            const QString &link,
                            const QString &description,
                            const QDateTime &lastUpdated,
                            const QString &dirname);
    void feedUpdateStatusChanged(const QString &url, bool status);
    void cancelFetching();

    void updateProgressChanged(int progress);
    void updateTotalChanged(int nrOfFeeds);
    void updatingChanged(bool state);

    void error(Error::Type type, const QString &url, const QString &id, const int errorId, const QString &errorString, const QString &title);
    void downloadFinished(QString url) const;
    void foundRedirectedUrl(const QUrl &url, const QUrl &newUrl);

private:
    Fetcher();

    QNetworkReply *head(QNetworkRequest &request) const;
    void setHeader(QNetworkRequest &request) const;

    QSet<QString> m_ongoingImageDownloads;

    QNetworkAccessManager *manager;
    int m_updateProgress;
    int m_updateTotal;
    bool m_updating;

    const qint64 m_checkInterval = 10 * 60 * 1000; // trigger timer every 10 minutes
    QTimer *m_updateTimer;
    QDateTime m_updateTriggerTime;

    QByteArray m_systemHttpProxy;
    QByteArray m_systemHttpsProxy;
    bool m_isSystemProxyDefined;
};
