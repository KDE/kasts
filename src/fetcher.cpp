/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "fetcher.h"
#include "fetcherlogging.h"

#include <KLocalizedString>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkProxyFactory>
#include <QNetworkReply>
#include <QTime>
#include <QTimer>

#include "database.h"
#include "enclosure.h"
#include "kasts-version.h"
#include "models/errorlogmodel.h"
#include "settingsmanager.h"
#include "sync/sync.h"
#include "utils/fetchfeedsjob.h"
#include "utils/networkconnectionmanager.h"
#include "utils/storagemanager.h"

Fetcher::Fetcher()
{
    connect(this, &Fetcher::error, &ErrorLogModel::instance(), &ErrorLogModel::monitorErrorMessages);

    m_updateProgress = -1;
    m_updateTotal = -1;
    m_updating = false;

    manager = new QNetworkAccessManager(this);
    manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    manager->setStrictTransportSecurityEnabled(true);
    manager->enableStrictTransportSecurityStore(true);

    // First save the original system proxy settings
    m_systemHttpProxy = qgetenv("http_proxy");
    m_systemHttpsProxy = qgetenv("https_proxy");
    m_isSystemProxyDefined = (QNetworkProxy::applicationProxy().type() != QNetworkProxy::ProxyType::NoProxy);
    qCDebug(kastsFetcher) << "saved system proxy:" << m_systemHttpProxy << m_systemHttpsProxy << m_isSystemProxyDefined;

    // Set network proxy based on saved settings
    setNetworkProxy();

    // setup update timer if required
    initializeUpdateTimer();
    connect(SettingsManager::self(), &SettingsManager::autoFeedUpdateIntervalChanged, this, &Fetcher::initializeUpdateTimer);
}

void Fetcher::fetch(const QString &url)
{
    QStringList urls(url);
    fetch(urls);
}

void Fetcher::fetchAll()
{
    if (Sync::instance().syncEnabled() && SettingsManager::self()->syncWhenUpdatingFeeds()) {
        Sync::instance().doRegularSync(true);
    } else {
        QStringList urls;
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT url FROM Feeds;"));
        Database::instance().execute(query);
        while (query.next()) {
            urls += query.value(0).toString();
        }

        if (urls.count() > 0) {
            fetch(urls);
        }
    }
}

void Fetcher::fetch(const QStringList &urls)
{
    if (m_updating)
        return; // update is already running, do nothing

    m_updating = true;
    m_updateProgress = 0;
    m_updateTotal = urls.count();
    Q_EMIT updatingChanged(m_updating);
    Q_EMIT updateProgressChanged(m_updateProgress);
    Q_EMIT updateTotalChanged(m_updateTotal);

    qCDebug(kastsFetcher) << "Create fetchFeedsJob";
    FetchFeedsJob *fetchFeedsJob = new FetchFeedsJob(urls, this);
    connect(this, &Fetcher::cancelFetching, fetchFeedsJob, &FetchFeedsJob::abort);
    connect(fetchFeedsJob, &FetchFeedsJob::processedAmountChanged, this, [this](KJob *job, KJob::Unit unit, qulonglong amount) {
        qCDebug(kastsFetcher) << "FetchFeedsJob::processedAmountChanged:" << amount;
        Q_UNUSED(job);
        Q_ASSERT(unit == KJob::Unit::Items);
        m_updateProgress = amount;
        Q_EMIT updateProgressChanged(m_updateProgress);
    });
    connect(fetchFeedsJob, &FetchFeedsJob::result, this, [this, fetchFeedsJob]() {
        qCDebug(kastsFetcher) << "result slot of FetchFeedsJob";
        if (fetchFeedsJob->error() && !fetchFeedsJob->aborted()) {
            Q_EMIT error(Error::Type::FeedUpdate, QString(), QString(), fetchFeedsJob->error(), fetchFeedsJob->errorString(), QString());
        }
        if (m_updating) {
            m_updating = false;
            Q_EMIT updatingChanged(m_updating);
        }
    });

    fetchFeedsJob->start();
    qCDebug(kastsFetcher) << "end of Fetcher::fetch";
}

QString Fetcher::image(const QString &url)
{
    if (url.isEmpty()) {
        return QLatin1String("no-image");
    }

    // if image is already cached, then return the path
    QString path = StorageManager::instance().imagePath(url);
    if (QFileInfo::exists(path)) {
        if (QFileInfo(path).size() != 0) {
            return QUrl::fromLocalFile(path).toString();
        }
    }

    // avoid restarting an image download if it's already running
    if (m_ongoingImageDownloads.contains(url)) {
        return QLatin1String("fetching");
    }

    // if image has not yet been cached, then check for network connectivity if
    // possible; and download the image
    if (!NetworkConnectionManager::instance().imageDownloadsAllowed()) {
        return QLatin1String("no-image");
    }

    m_ongoingImageDownloads.insert(url);
    QNetworkRequest request((QUrl(url)));
    request.setTransferTimeout();
    QNetworkReply *reply = get(request);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->isOpen() && !reply->error()) {
            QByteArray data = reply->readAll();
            QFile file(path);
            file.open(QIODevice::WriteOnly);
            file.write(data);
            file.close();
            Q_EMIT downloadFinished(url);
        }
        m_ongoingImageDownloads.remove(url);
        reply->deleteLater();
    });
    return QLatin1String("fetching");
}

QNetworkReply *Fetcher::download(const QString &url, const QString &filePath) const
{
    QNetworkRequest request((QUrl(url)));
    request.setTransferTimeout();

    QFile *file = new QFile(filePath);
    if (file->exists() && file->size() > 0) {
        // try to resume download
        int resumedAt = file->size();
        qCDebug(kastsFetcher) << "Resuming download at" << resumedAt << "bytes";
        QByteArray rangeHeaderValue = QByteArray("bytes=") + QByteArray::number(resumedAt) + QByteArray("-");
        request.setRawHeader(QByteArray("Range"), rangeHeaderValue);
        file->open(QIODevice::WriteOnly | QIODevice::Append);
    } else {
        qCDebug(kastsFetcher) << "Starting new download";
        file->open(QIODevice::WriteOnly);
    }

    QNetworkReply *reply = get(request);

    connect(reply, &QNetworkReply::readyRead, this, [=]() {
        if (reply->isOpen() && file) {
            QByteArray data = reply->readAll();
            file->write(data);
        }
    });

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->isOpen() && file) {
            QByteArray data = reply->readAll();
            file->write(data);
            file->close();

            Q_EMIT downloadFinished(url);
        }

        // clean up; close file if still open in case something has gone wrong
        if (file) {
            if (file->isOpen()) {
                file->close();
            }
            delete file;
        }
        reply->deleteLater();
    });

    return reply;
}

void Fetcher::getRedirectedUrl(const QUrl &url)
{
    QNetworkRequest request((QUrl(url)));
    request.setTransferTimeout(5000); // wait 5 seconds; it will fall back to original url otherwise

    QNetworkReply *reply = head(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        qCDebug(kastsFetcher) << "finished looking for redirect; this is the old url and the redirected url:" << url << reply->url();

        QUrl newUrl = reply->url();
        QTimer::singleShot(0, this, [this, url, newUrl]() {
            Q_EMIT foundRedirectedUrl(url, newUrl);
        });
        reply->deleteLater();
    });
}

QNetworkReply *Fetcher::get(QNetworkRequest &request) const
{
    setHeader(request);
    return manager->get(request);
}

QNetworkReply *Fetcher::post(QNetworkRequest &request, const QByteArray &data) const
{
    setHeader(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));
    return manager->post(request, data);
}

QNetworkReply *Fetcher::head(QNetworkRequest &request) const
{
    setHeader(request);
    return manager->head(request);
}

void Fetcher::setHeader(QNetworkRequest &request) const
{
    request.setRawHeader(QByteArray("User-Agent"), QByteArray("Kasts/") + QByteArray(KASTS_VERSION_STRING) + QByteArray(" Syndication"));
}

void Fetcher::initializeUpdateTimer()
{
    qCDebug(kastsFetcher) << "Fetcher::setUpdateTimer";
    qCDebug(kastsFetcher) << "new auto update interval =" << SettingsManager::self()->autoFeedUpdateInterval();

    if (m_updateTimer) {
        m_updateTimer->stop();
        disconnect(m_updateTimer, &QTimer::timeout, this, &Fetcher::checkUpdateTimer);
        delete m_updateTimer;
    }

    if (SettingsManager::self()->autoFeedUpdateInterval() > 0) {
        m_updateTriggerTime =
            QDateTime::currentDateTimeUtc().addSecs(3600 * SettingsManager::self()->autoFeedUpdateInterval()); // update interval specified in hours
        m_updateTimer = new QTimer(this);
        m_updateTimer->setTimerType(Qt::VeryCoarseTimer);

        connect(m_updateTimer, &QTimer::timeout, this, &Fetcher::checkUpdateTimer);

        m_updateTimer->start(m_checkInterval); // trigger every ten minutes
    }
}

void Fetcher::checkUpdateTimer()
{
    qCDebug(kastsFetcher) << "Fetcher::checkUpdateTimer; next automatic feed update in" << m_updateTriggerTime - QDateTime::currentDateTimeUtc();

    // add a few seconds as "fuzzy match" to avoid that the trigger is delayed
    // by another 10 minutes due to a difference of just a few milliseconds
    if (QDateTime::currentDateTimeUtc().addSecs(5) > m_updateTriggerTime) {
        qCDebug(kastsFetcher) << "Trigger for feed update has been reached; updating feeds now";
        QTimer::singleShot(0, this, &Fetcher::fetchAll);

        // set next update time
        m_updateTriggerTime =
            QDateTime::currentDateTimeUtc().addSecs(3600 * SettingsManager::self()->autoFeedUpdateInterval()); // update interval specified in hours
        qCDebug(kastsFetcher) << "new auto feed update trigger set to" << m_updateTriggerTime;
    }
}

void Fetcher::setNetworkProxy()
{
    SettingsManager *settings = SettingsManager::self();
    QNetworkProxy proxy;

    // define network proxy environment variable
    // this is needed for the audio backends which don't obey qt's settings
    QByteArray appProxy;
    if (!settings->proxyUser().isEmpty()) {
        appProxy += QUrl::toPercentEncoding(settings->proxyUser());
        if (!settings->proxyPassword().isEmpty()) {
            appProxy += ":" + QUrl::toPercentEncoding(settings->proxyPassword());
        }
        appProxy += "@";
    }
    appProxy += settings->proxyHost().toLocal8Bit() + ":" + QByteArray::number(settings->proxyPort());

    // type match to ProxyType from config.ksettings
    switch (settings->proxyType()) {
    case 1: // No Proxy
        proxy.setType(QNetworkProxy::NoProxy);
        QNetworkProxy::setApplicationProxy(proxy);

        // also reset environment variables if they have been set
        qunsetenv("http_proxy");
        qunsetenv("https_proxy");
        break;
    case 2: // HTTP
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName(settings->proxyHost());
        proxy.setPort(settings->proxyPort());
        proxy.setUser(settings->proxyUser());
        proxy.setPassword(settings->proxyPassword());
        QNetworkProxy::setApplicationProxy(proxy);

        // also set it through environment variables for the audio backends
        appProxy.prepend("http://");
        qputenv("http_proxy", appProxy);
        qputenv("https_proxy", appProxy);
        qCDebug(kastsFetcher) << "appProxy environment variable" << appProxy;
        break;
    case 3: // SOCKS 5
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(settings->proxyHost());
        proxy.setPort(settings->proxyPort());
        proxy.setUser(settings->proxyUser());
        proxy.setPassword(settings->proxyPassword());
        QNetworkProxy::setApplicationProxy(proxy);

        // also set it through environment variables for the audio backends
        appProxy.prepend("socks5://");
        qputenv("http_proxy", appProxy);
        qputenv("https_proxy", appProxy);
        qCDebug(kastsFetcher) << "appProxy environment variable" << appProxy;
        break;
    case 0: // System Default
    default:
        QNetworkProxyFactory::setUseSystemConfiguration(true);

        // also reset env variables that might have been overridden
        if (!m_systemHttpProxy.isEmpty()) {
            qputenv("http_proxy", m_systemHttpProxy);
        } else {
            qunsetenv("http_proxy");
        }
        if (!m_systemHttpProxy.isEmpty()) {
            qputenv("https_proxy", m_systemHttpsProxy);
        } else {
            qunsetenv("https_proxy");
        }
        break;
    }

    qCDebug(kastsFetcher) << "Network proxy set to:" << QNetworkProxy::applicationProxy();
}

bool Fetcher::isSystemProxyDefined()
{
    return m_isSystemProxyDefined;
}
