/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "gpodder.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSqlQuery>
#include <QUrl>

#include "database.h"
#include "fetcher.h"
#include "sync/gpodder/devicerequest.h"
#include "sync/gpodder/episodeactionrequest.h"
#include "sync/gpodder/loginrequest.h"
#include "sync/gpodder/logoutrequest.h"
#include "sync/gpodder/subscriptionrequest.h"
#include "sync/gpodder/syncrequest.h"
#include "sync/gpodder/updatedevicerequest.h"
#include "sync/gpodder/updatesyncrequest.h"
#include "sync/gpodder/uploadepisodeactionrequest.h"
#include "sync/gpodder/uploadsubscriptionrequest.h"
#include "sync/sync.h"
#include "sync/syncutils.h"
#include "synclogging.h"

GPodder::GPodder(const QString &username, const QString &password, QObject *parent)
    : QObject(parent)
    , m_username(username)
    , m_password(password)
{
}

GPodder::GPodder(const QString &username, const QString &password, const QString &hostname, const SyncUtils::Provider provider, QObject *parent)
    : QObject(parent)
    , m_username(username)
    , m_password(password)
    , m_hostname(hostname)
    , m_provider(provider)
{
}

LoginRequest *GPodder::login()
{
    QString url = QStringLiteral("%1/api/2/auth/%2/login.json").arg(baseUrl(), m_username);
    QNetworkRequest request((QUrl(url)));
    request.setTransferTimeout();
    addAuthentication(request);

    QNetworkReply *reply = Fetcher::instance().post(request, QByteArray());
    LoginRequest *loginRequest = new LoginRequest(m_provider, reply, this);
    return loginRequest;
}

LogoutRequest *GPodder::logout()
{
    QString url = QStringLiteral("%1/api/2/auth/%2/logout.json").arg(baseUrl(), m_username);
    QNetworkRequest request((QUrl(url)));
    request.setTransferTimeout();
    addAuthentication(request);

    QNetworkReply *reply = Fetcher::instance().post(request, QByteArray());
    LogoutRequest *logoutRequest = new LogoutRequest(m_provider, reply, this);
    return logoutRequest;
}

DeviceRequest *GPodder::getDevices()
{
    QString url = QStringLiteral("%1/api/2/devices/%2.json").arg(baseUrl(), m_username);
    QNetworkRequest request((QUrl(url)));
    request.setTransferTimeout();
    addAuthentication(request);

    QNetworkReply *reply = Fetcher::instance().get(request);
    DeviceRequest *deviceRequest = new DeviceRequest(m_provider, reply, this);
    return deviceRequest;
}

UpdateDeviceRequest *GPodder::updateDevice(const QString &id, const QString &caption, const QString &type)
{
    QString url = QStringLiteral("%1/api/2/devices/%2/%3.json").arg(baseUrl(), m_username, id);
    QNetworkRequest request((QUrl(url)));
    request.setTransferTimeout();
    addAuthentication(request);

    QJsonObject deviceObject;
    deviceObject.insert(QStringLiteral("caption"), caption);
    deviceObject.insert(QStringLiteral("type"), type);
    QJsonDocument json(deviceObject);

    QByteArray data = json.toJson(QJsonDocument::Compact);

    QNetworkReply *reply = Fetcher::instance().post(request, data);

    UpdateDeviceRequest *updateDeviceRequest = new UpdateDeviceRequest(m_provider, reply, this);
    return updateDeviceRequest;
}
SyncRequest *GPodder::getSyncStatus()
{
    QString url = QStringLiteral("%1/api/2/sync-devices/%2.json").arg(baseUrl(), m_username);
    QNetworkRequest request((QUrl(url)));
    request.setTransferTimeout();
    addAuthentication(request);

    QNetworkReply *reply = Fetcher::instance().get(request);
    SyncRequest *syncRequest = new SyncRequest(m_provider, reply, this);
    return syncRequest;
}

UpdateSyncRequest *GPodder::updateSyncStatus(const QVector<QStringList> &syncedDevices, const QStringList &unsyncedDevices)
{
    QString url = QStringLiteral("%1/api/2/sync-devices/%2.json").arg(baseUrl(), m_username);
    QNetworkRequest request((QUrl(url)));
    request.setTransferTimeout();
    addAuthentication(request);

    QJsonArray syncGroupArray;
    for (const QStringList &syncGroup : syncedDevices) {
        syncGroupArray.push_back(QJsonArray::fromStringList(syncGroup));
    }
    QJsonArray unsyncArray = QJsonArray::fromStringList(unsyncedDevices);

    QJsonObject uploadObject;
    uploadObject.insert(QStringLiteral("synchronize"), syncGroupArray);
    uploadObject.insert(QStringLiteral("stop-synchronize"), unsyncArray);
    QJsonDocument json(uploadObject);

    QByteArray data = json.toJson(QJsonDocument::Compact);

    QNetworkReply *reply = Fetcher::instance().post(request, data);

    UpdateSyncRequest *updateSyncRequest = new UpdateSyncRequest(m_provider, reply, this);
    return updateSyncRequest;
}

SubscriptionRequest *GPodder::getSubscriptionChanges(const qulonglong &oldtimestamp, const QString &device)
{
    QString url;
    if (m_provider == SyncUtils::Provider::GPodderNextcloud) {
        url = QStringLiteral("%1/index.php/apps/gpoddersync/subscriptions?since=%2").arg(baseUrl(), QString::number(oldtimestamp));
    } else {
        url = QStringLiteral("%1/api/2/subscriptions/%2/%3.json?since=%4").arg(baseUrl(), m_username, device, QString::number(oldtimestamp));
    }
    QNetworkRequest request((QUrl(url)));
    request.setTransferTimeout();
    addAuthentication(request);

    QNetworkReply *reply = Fetcher::instance().get(request);

    SubscriptionRequest *subscriptionRequest = new SubscriptionRequest(m_provider, reply, this);
    return subscriptionRequest;
}

UploadSubscriptionRequest *GPodder::uploadSubscriptionChanges(const QStringList &add, const QStringList &remove, const QString &device)
{
    QString url;
    if (m_provider == SyncUtils::Provider::GPodderNextcloud) {
        url = QStringLiteral("%1/index.php/apps/gpoddersync/subscription_change/create").arg(baseUrl());
    } else {
        url = QStringLiteral("%1/api/2/subscriptions/%2/%3.json").arg(baseUrl(), m_username, device);
    }
    QNetworkRequest request((QUrl(url)));
    request.setTransferTimeout();
    addAuthentication(request);

    QJsonArray addArray = QJsonArray::fromStringList(add);
    QJsonArray removeArray = QJsonArray::fromStringList(remove);

    QJsonObject uploadObject;
    uploadObject.insert(QStringLiteral("add"), addArray);
    uploadObject.insert(QStringLiteral("remove"), removeArray);
    QJsonDocument json(uploadObject);

    QByteArray data = json.toJson(QJsonDocument::Compact);

    QNetworkReply *reply = Fetcher::instance().post(request, data);

    UploadSubscriptionRequest *uploadSubscriptionRequest = new UploadSubscriptionRequest(m_provider, reply, this);
    return uploadSubscriptionRequest;
}

EpisodeActionRequest *GPodder::getEpisodeActions(const qulonglong &timestamp, bool aggregated)
{
    QString url;
    if (m_provider == SyncUtils::Provider::GPodderNextcloud) {
        url = QStringLiteral("%1/index.php/apps/gpoddersync/episode_action?since=%2").arg(baseUrl(), QString::number(timestamp));
    } else {
        url = QStringLiteral("%1/api/2/episodes/%2.json?since=%3&aggregated=%4")
                  .arg(baseUrl(), m_username, QString::number(timestamp), aggregated ? QStringLiteral("true") : QStringLiteral("false"));
    }
    QNetworkRequest request((QUrl(url)));
    request.setTransferTimeout();
    addAuthentication(request);

    QNetworkReply *reply = Fetcher::instance().get(request);

    EpisodeActionRequest *episodeActionRequest = new EpisodeActionRequest(m_provider, reply, this);
    return episodeActionRequest;
}

UploadEpisodeActionRequest *GPodder::uploadEpisodeActions(const QList<SyncUtils::EpisodeAction> &episodeActions)
{
    QString url;
    if (m_provider == SyncUtils::Provider::GPodderNextcloud) {
        url = QStringLiteral("%1/index.php/apps/gpoddersync/episode_action/create").arg(baseUrl());
    } else {
        url = QStringLiteral("%1/api/2/episodes/%2.json").arg(baseUrl(), m_username);
    }
    QNetworkRequest request((QUrl(url)));
    request.setTransferTimeout();
    addAuthentication(request);

    QJsonArray actionArray;
    for (const SyncUtils::EpisodeAction &episodeAction : episodeActions) {
        QJsonObject actionObject;
        actionObject.insert(QStringLiteral("podcast"), episodeAction.podcast);
        if (!episodeAction.url.isEmpty()) {
            actionObject.insert(QStringLiteral("episode"), episodeAction.url);
        } else if (!episodeAction.id.isEmpty()) {
            QSqlQuery query;
            query.prepare(QStringLiteral("SELECT url FROM Enclosures JOIN Entries ON Entries.entryuid=Enclosures.entryuid WHERE Entries.id=:id;"));
            query.bindValue(QStringLiteral(":id"), episodeAction.id);
            Database::instance().execute(query);
            if (!query.next()) {
                qCDebug(kastsSync) << "cannot find episode with id:" << episodeAction.id;
                continue;
            } else {
                actionObject.insert(QStringLiteral("episode"), query.value(QStringLiteral("url")).toString());
            }
        }
        actionObject.insert(QStringLiteral("guid"), episodeAction.id);
        actionObject.insert(QStringLiteral("device"), episodeAction.device);
        actionObject.insert(QStringLiteral("action"), episodeAction.action);

        QString dateTime = QDateTime::fromSecsSinceEpoch(episodeAction.timestamp).toUTC().toString(Qt::ISODate);
        // Qt::ISODate adds "Z" to the end of the string; cut it off since
        // GPodderNextcloud cannot handle it
        dateTime.chop(1);
        actionObject.insert(QStringLiteral("timestamp"), dateTime);
        if (episodeAction.action == QStringLiteral("play")) {
            actionObject.insert(QStringLiteral("started"), static_cast<qint64>(episodeAction.started));
            actionObject.insert(QStringLiteral("position"), static_cast<qint64>(episodeAction.position));
            actionObject.insert(QStringLiteral("total"), static_cast<qint64>(episodeAction.total));
        }
        actionArray.push_back(actionObject);
    }
    QJsonDocument json(actionArray);

    QByteArray data = json.toJson(QJsonDocument::Compact);

    QNetworkReply *reply = Fetcher::instance().post(request, data);

    UploadEpisodeActionRequest *uploadEpisodeActionRequest = new UploadEpisodeActionRequest(m_provider, reply, this);
    return uploadEpisodeActionRequest;
}

QString GPodder::baseUrl()
{
    return m_hostname;
}

void GPodder::addAuthentication(QNetworkRequest &request)
{
    QByteArray headerData = "Basic " + QString(m_username + QStringLiteral(":") + m_password).toLocal8Bit().toBase64();
    request.setRawHeader("Authorization", headerData);
}
