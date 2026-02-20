/**
 * SPDX-FileCopyrightText: 2021 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QQmlEngine>

#include <KFormat>

#include "error.h"
#include "sync/syncutils.h"

namespace QKeychain
{
class WritePasswordJob;
class DeletePasswordJob;
}

class GPodder;

class Sync : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool syncEnabled READ syncEnabled WRITE setSyncEnabled NOTIFY syncEnabledChanged)
    Q_PROPERTY(QString username READ username NOTIFY credentialsChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY credentialsChanged)
    Q_PROPERTY(QString device READ device WRITE setDevice NOTIFY deviceChanged)
    Q_PROPERTY(QString deviceName READ deviceName WRITE setDeviceName NOTIFY deviceNameChanged)
    Q_PROPERTY(QString hostname READ hostname WRITE setHostname NOTIFY hostnameChanged)
    Q_PROPERTY(SyncUtils::Provider provider READ provider WRITE setProvider NOTIFY providerChanged)

    Q_PROPERTY(QString suggestedDevice READ suggestedDevice CONSTANT)
    Q_PROPERTY(QString suggestedDeviceName READ suggestedDeviceName CONSTANT)
    Q_PROPERTY(QVector<SyncUtils::Device> deviceList READ deviceList NOTIFY deviceListReceived)

    Q_PROPERTY(SyncUtils::SyncStatus syncStatus MEMBER m_syncStatus NOTIFY syncProgressChanged)
    Q_PROPERTY(int syncProgress MEMBER m_syncProgress NOTIFY syncProgressChanged)
    Q_PROPERTY(int syncProgressTotal MEMBER m_syncProgressTotal CONSTANT)
    Q_PROPERTY(QString syncProgressText MEMBER m_syncProgressText NOTIFY syncProgressChanged)
    Q_PROPERTY(QString lastSuccessfulDownloadSync READ lastSuccessfulDownloadSync NOTIFY syncProgressChanged)
    Q_PROPERTY(QString lastSuccessfulUploadSync READ lastSuccessfulUploadSync NOTIFY syncProgressChanged)

public:
    static Sync &instance()
    {
        static Sync _instance;
        return _instance;
    }
    static Sync *create(QQmlEngine *engine, QJSEngine *)
    {
        engine->setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
        return &instance();
    }

    bool syncEnabled() const;
    QString username() const;
    QString password() const;
    QString device() const;
    QString deviceName() const;
    QString hostname() const;
    SyncUtils::Provider provider() const;
    QString lastSuccessfulSync(const QStringList &matchingLabels = {}) const;
    QString lastSuccessfulDownloadSync() const;
    QString lastSuccessfulUploadSync() const;

    QString suggestedDevice() const;
    QString suggestedDeviceName() const;
    QVector<SyncUtils::Device> deviceList() const;

    void setSyncEnabled(bool status);
    void setPassword(const QString &password);
    void setDevice(const QString &device);
    void setDeviceName(const QString &deviceName);
    void setHostname(const QString &hostname);
    void setProvider(const SyncUtils::Provider provider);

    Q_INVOKABLE void login(const QString &username, const QString &password);
    Q_INVOKABLE void retrieveCredentialsFromConfig();
    Q_INVOKABLE void logout();
    Q_INVOKABLE void registerNewDevice(const QString &id, const QString &caption, const QString &type = QStringLiteral("other"));
    Q_INVOKABLE void linkUpAllDevices();

    void doSync(SyncUtils::SyncStatus status, bool forceFetchAll = false); // base method for syncing
    Q_INVOKABLE void doRegularSync(bool forceFetchAll = false); // regular sync; can be forced to update all feeds
    Q_INVOKABLE void doForceSync(); // force a full re-sync with the server; discarding local eposide acions
    Q_INVOKABLE void doSyncPushAll(); // upload all local episode states to the server; this will likely overwrite all actions stored on the server
    Q_INVOKABLE void doQuickSync(); // only upload pending local episode actions; intended to be run directly after an episode action has been created

    // Next are some generic methods to store and apply local changes to be synced
    void storeAddFeedAction(const QString &url);
    void storeRemoveFeedAction(const QString &url);
    void storePlayEpisodeAction(const QString &id, const qulonglong started, const qulonglong position);
    void applySubscriptionChangesLocally(const QStringList &addList, const QStringList &removeList);
    void applyEpisodeActionsLocally(const QHash<QString, QHash<QString, SyncUtils::EpisodeAction>> &episodeActionHash);

    // new actions based on uids; should replace the ones above after refactor
    void storePlayedEpisodeAction(const QList<qint64> &entryuids);

Q_SIGNALS:
    void syncEnabledChanged();
    void credentialsChanged();
    void deviceChanged();
    void deviceNameChanged();
    void hostnameChanged();
    void providerChanged();
    void deviceCreated();

    void passwordSaveFinished(bool success);
    void passwordRetrievalFinished(QString password);
    void passwordInputRequired();

    void loginSucceeded();
    void deviceListReceived();

    void error(Error::Type type, const QString &url, const QString &id, const int errorId, const QString &errorString, const QString &title);
    void syncProgressChanged();
    void abortSync();

private:
    Sync();

    void clearSettings();
    void savePasswordToKeyChain(const QString &username, const QString &password);
    void savePasswordToFile(const QString &username, const QString &password);
    void retrievePasswordFromKeyChain(const QString &username);
    QString retrievePasswordFromFile(const QString &username);
    void deletePasswordFromKeychain(const QString &username);

    void retrieveAllLocalEpisodeStates();
    void onWriteDummyJobFinished(QKeychain::WritePasswordJob *writeDummyJob, const QString &username);
    void onWritePasswordJobFinished(QKeychain::WritePasswordJob *job, const QString &username, const QString &password);
    void onDeleteJobFinished(QKeychain::DeletePasswordJob *deleteJob, const QString &username);

    GPodder *m_gpodder = nullptr;

    bool m_syncEnabled;
    QString m_username;
    QString m_password;
    QString m_device;
    QString m_deviceName;
    QString m_hostname;
    SyncUtils::Provider m_provider;

    QVector<SyncUtils::Device> m_deviceList;

    KFormat m_kformat;

    // variables needed for linkUpAllDevices()
    QStringList m_syncUpAllSubscriptions;
    int m_deviceResponses;

    // internal variables used while syncing
    bool m_allowSyncActionLogging = true;

    // internal variables used for UI notifications
    SyncUtils::SyncStatus m_syncStatus = SyncUtils::SyncStatus::NoSync;
    int m_syncProgress = 0;
    int m_syncProgressTotal = 7;
    QString m_syncProgressText = QLatin1String("");
};
