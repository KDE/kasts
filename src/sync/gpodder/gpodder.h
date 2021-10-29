/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <QVector>

#include "sync/sync.h"
#include "sync/syncutils.h"

class LoginRequest;
class LogoutRequest;
class DeviceRequest;
class UpdateDeviceRequest;
class SyncRequest;
class UpdateSyncRequest;
class SubscriptionRequest;
class UploadSubscriptionRequest;
class EpisodeActionRequest;
class UploadEpisodeActionRequest;

class GPodder : public QObject
{
    Q_OBJECT

public:
    GPodder(const QString &username, const QString &password, QObject *parent = nullptr);
    GPodder(const QString &username, const QString &password, const QString &hostname, const SyncUtils::Provider provider, QObject *parent = nullptr);

    LoginRequest *login();
    LogoutRequest *logout();
    DeviceRequest *getDevices();
    UpdateDeviceRequest *updateDevice(const QString &id, const QString &caption, const QString &type = QStringLiteral("other"));
    SyncRequest *getSyncStatus();
    UpdateSyncRequest *updateSyncStatus(const QVector<QStringList> &syncedDevices, const QStringList &unsyncedDevices);
    SubscriptionRequest *getSubscriptionChanges(const qulonglong &oldtimestamp, const QString &device);
    UploadSubscriptionRequest *uploadSubscriptionChanges(const QStringList &add, const QStringList &remove, const QString &device);
    EpisodeActionRequest *getEpisodeActions(const qulonglong &timestamp, bool aggregated = false);
    UploadEpisodeActionRequest *uploadEpisodeActions(const QVector<SyncUtils::EpisodeAction> &episodeActions);

private:
    QString baseUrl();
    void addAuthentication(QNetworkRequest &request);

    QString m_username;
    QString m_password;
    QString m_hostname = QLatin1String("https://gpodder.net");
    SyncUtils::Provider m_provider = SyncUtils::Provider::GPodderNet;
};
