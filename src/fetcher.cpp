/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "fetcher.h"
#include "fetcherlogging.h"

#include <KLocalizedString>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDomElement>
#include <QFile>
#include <QFileInfo>
#include <QMultiMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTextDocumentFragment>
#include <QTime>
#include <Syndication/Syndication>

#include "database.h"
#include "enclosure.h"
#include "fetchfeedsjob.h"
#include "kasts-version.h"
#include "models/errorlogmodel.h"
#include "settingsmanager.h"
#include "storagemanager.h"

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

#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
    m_nmInterface = new OrgFreedesktopNetworkManagerInterface(QStringLiteral("org.freedesktop.NetworkManager"),
                                                              QStringLiteral("/org/freedesktop/NetworkManager"),
                                                              QDBusConnection::systemBus(),
                                                              this);
#endif
}

void Fetcher::fetch(const QString &url)
{
    QStringList urls(url);
    fetch(urls);
}

void Fetcher::fetchAll()
{
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
        if (fetchFeedsJob->error()) {
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
    if (canCheckNetworkStatus()) {
        if (!networkConnected() || (isMeteredConnection() && !SettingsManager::self()->allowMeteredImageDownloads())) {
            return QLatin1String("no-image");
        }
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
    int resumedAt = 0;
    if (file->exists() && file->size() > 0) {
        // try to resume download
        resumedAt = file->size();
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

QNetworkReply *Fetcher::get(QNetworkRequest &request) const
{
    setHeader(request);
    return manager->get(request);
}

QNetworkReply *Fetcher::head(QNetworkRequest &request) const
{
    setHeader(request);
    return manager->head(request);
}

void Fetcher::setHeader(QNetworkRequest &request) const
{
    request.setRawHeader(QByteArray("User-Agent"), QByteArray("Kasts/") + QByteArray(KASTS_VERSION_STRING) + QByteArray("; Syndication"));
}

bool Fetcher::canCheckNetworkStatus() const
{
#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
    qCDebug(kastsFetcher) << "Can NetworkManager be reached?" << m_nmInterface->isValid();
    return (m_nmInterface && m_nmInterface->isValid());
#else
    return false;
#endif
}

bool Fetcher::networkConnected() const
{
#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
    qCDebug(kastsFetcher) << "Network connected?" << (m_nmInterface->state() >= 70) << m_nmInterface->state();
    return (m_nmInterface && m_nmInterface->state() >= 70);
#else
    return true;
#endif
}

bool Fetcher::isMeteredConnection() const
{
#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
    if (canCheckNetworkStatus()) {
        // Get network connection status through DBus (NetworkManager)
        // state == 1: explicitly configured as metered
        // state == 3: connection guessed as metered
        uint state = m_nmInterface->metered();
        qCDebug(kastsFetcher) << "Network Status:" << state;
        qCDebug(kastsFetcher) << "Connection is metered?" << (state == 1 || state == 3);
        return (state == 1 || state == 3);
    } else {
        return false;
    }
#else
    // TODO: get network connection type for Android and windows
    return false;
#endif
}
