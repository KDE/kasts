/**
 * SPDX-FileCopyrightText: 2021 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "sync.h"
#include "synclogging.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlQuery>
#include <QString>
#include <QSysInfo>
#include <QTimer>

#include <KFormat>
#include <KLocalizedString>

#include <qt6keychain/keychain.h>

#include "audiomanager.h"
#include "database.h"
#include "datamanager.h"
#include "entry.h"
#include "models/errorlogmodel.h"
#include "settingsmanager.h"
#include "sync/gpodder/devicerequest.h"
#include "sync/gpodder/gpodder.h"
#include "sync/gpodder/logoutrequest.h"
#include "sync/gpodder/subscriptionrequest.h"
#include "sync/gpodder/syncrequest.h"
#include "sync/gpodder/updatedevicerequest.h"
#include "sync/gpodder/updatesyncrequest.h"
#include "sync/gpodder/uploadsubscriptionrequest.h"
#include "sync/syncjob.h"
#include "sync/syncutils.h"
#include "utils/networkconnectionmanager.h"
#include "utils/storagemanager.h"

using namespace SyncUtils;

Sync::Sync()
    : QObject()
{
    connect(this, &Sync::error, &ErrorLogModel::instance(), &ErrorLogModel::monitorErrorMessages);
    connect(&AudioManager::instance(), &AudioManager::playbackStateChanged, this, &Sync::doQuickSync);

    retrieveCredentialsFromConfig();
}

void Sync::retrieveCredentialsFromConfig()
{
    if (!SettingsManager::self()->syncEnabled()) {
        m_syncEnabled = false;
        Q_EMIT syncEnabledChanged();
    } else if (!SettingsManager::self()->syncUsername().isEmpty()) {
        m_username = SettingsManager::self()->syncUsername();
        m_hostname = SettingsManager::self()->syncHostname();
        m_provider = static_cast<Provider>(SettingsManager::self()->syncProvider());

        connect(this, &Sync::passwordRetrievalFinished, this, [this](QString password) {
            disconnect(this, &Sync::passwordRetrievalFinished, this, nullptr);
            if (!password.isEmpty()) {
                m_syncEnabled = SettingsManager::self()->syncEnabled();
                m_password = password;
                m_hostname = SettingsManager::self()->syncHostname();

                if (m_provider == Provider::GPodderNet) {
                    m_device = SettingsManager::self()->syncDevice();
                    m_deviceName = SettingsManager::self()->syncDeviceName();

                    if (m_syncEnabled && !m_username.isEmpty() && !m_password.isEmpty() && !m_device.isEmpty()) {
                        if (m_hostname.isEmpty()) { // use default official server
                            m_gpodder = new GPodder(m_username, m_password, this);
                        } else { // i.e. custom gpodder host
                            m_gpodder = new GPodder(m_username, m_password, m_hostname, m_provider, this);
                        }
                    }
                } else if (m_provider == Provider::GPodderNextcloud) {
                    if (m_syncEnabled && !m_username.isEmpty() && !m_password.isEmpty() && !m_hostname.isEmpty()) {
                        m_gpodder = new GPodder(m_username, m_password, m_hostname, m_provider, this);
                    }
                }

                m_syncEnabled = SettingsManager::self()->syncEnabled();
                Q_EMIT syncEnabledChanged();

                // Now that we have all credentials we can do the initial sync if
                // it's enabled in the config.  If it's not enabled, then we handle
                // the automatic refresh through Main.qml
                if (NetworkConnectionManager::instance().feedUpdatesAllowed()) {
                    if (SettingsManager::self()->refreshOnStartup() && SettingsManager::self()->syncWhenUpdatingFeeds()) {
                        doRegularSync(true);
                    }
                }
            } else {
                // Ask for password and try to log in; if it succeeds, try
                // again to save the password.
                m_syncEnabled = false;
                QTimer::singleShot(0, this, [this]() {
                    Q_EMIT passwordInputRequired();
                });
            }
        });
        retrievePasswordFromKeyChain(m_username);
    }
}

bool Sync::syncEnabled() const
{
    return m_syncEnabled;
}

QString Sync::username() const
{
    return m_username;
}

QString Sync::password() const
{
    return m_password;
}

QString Sync::device() const
{
    return m_device;
}

QString Sync::deviceName() const
{
    return m_deviceName;
}

QString Sync::hostname() const
{
    return m_hostname;
}

Provider Sync::provider() const
{
    return m_provider;
}

QVector<Device> Sync::deviceList() const
{
    return m_deviceList;
}

QString Sync::lastSuccessfulSync(const QStringList &matchingLabels) const
{
    qulonglong timestamp = 0;
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM SyncTimeStamps;"));
    Database::instance().execute(query);
    while (query.next()) {
        QString label = query.value(QStringLiteral("syncservice")).toString();
        bool match = matchingLabels.isEmpty() || matchingLabels.contains(label);
        if (match) {
            qulonglong timestampDB = query.value(QStringLiteral("timestamp")).toULongLong();
            if (timestampDB > timestamp) {
                timestamp = timestampDB;
            }
        }
    }

    if (timestamp > 1) {
        QDateTime datetime = QDateTime::fromSecsSinceEpoch(timestamp);
        return m_kformat.formatRelativeDateTime(datetime, QLocale::ShortFormat);
    } else {
        return i18n("Never");
    }
}

QString Sync::lastSuccessfulDownloadSync() const
{
    QStringList labels = {subscriptionTimestampLabel, episodeTimestampLabel};
    return lastSuccessfulSync(labels);
}

QString Sync::lastSuccessfulUploadSync() const
{
    QStringList labels = {uploadSubscriptionTimestampLabel, uploadEpisodeTimestampLabel};
    return lastSuccessfulSync(labels);
}

QString Sync::suggestedDevice() const
{
    return QStringLiteral("kasts-") + QSysInfo::machineHostName();
}

QString Sync::suggestedDeviceName() const
{
    return i18nc("Suggested description for this device on gpodder sync service; argument is the hostname", "Kasts on %1", QSysInfo::machineHostName());
}

void Sync::setSyncEnabled(bool status)
{
    m_syncEnabled = status;
    SettingsManager::self()->setSyncEnabled(m_syncEnabled);
    SettingsManager::self()->save();
    Q_EMIT syncEnabledChanged();
}

void Sync::setPassword(const QString &password)
{
    // this method is used to set the password if the proper credentials could
    // not be retrieved from the keychain or file
    connect(this, &Sync::passwordSaveFinished, this, [this]() {
        disconnect(this, &Sync::passwordSaveFinished, this, nullptr);
        QTimer::singleShot(0, this, [this]() {
            retrieveCredentialsFromConfig();
        });
    });
    savePasswordToKeyChain(m_username, password);
}

void Sync::setDevice(const QString &device)
{
    m_device = device;
    SettingsManager::self()->setSyncDevice(m_device);
    SettingsManager::self()->save();
    Q_EMIT deviceChanged();
}

void Sync::setDeviceName(const QString &deviceName)
{
    m_deviceName = deviceName;
    SettingsManager::self()->setSyncDeviceName(m_deviceName);
    SettingsManager::self()->save();
    Q_EMIT deviceNameChanged();
}

void Sync::setHostname(const QString &hostname)
{
    if (hostname.isEmpty()) {
        m_hostname.clear();
    } else {
        QString cleanedHostname = hostname;
        QUrl hostUrl = QUrl(hostname);

        if (hostUrl.scheme().isEmpty()) {
            hostUrl.setScheme(QStringLiteral("https"));
            if (hostUrl.authority().isEmpty() && !hostUrl.path().isEmpty()) {
                hostUrl.setAuthority(hostUrl.path());
                hostUrl.setPath(QLatin1String(""));
            }
            cleanedHostname = hostUrl.toString();
        }

        m_hostname = cleanedHostname;
    }

    SettingsManager::self()->setSyncHostname(m_hostname);
    SettingsManager::self()->save();
    Q_EMIT hostnameChanged();
}

void Sync::setProvider(const Provider provider)
{
    m_provider = provider;
    SettingsManager::self()->setSyncProvider(m_provider);
    SettingsManager::self()->save();
    Q_EMIT providerChanged();
}

void Sync::login(const QString &username, const QString &password)
{
    if (m_gpodder) {
        delete m_gpodder;
        m_gpodder = nullptr;
    }

    m_deviceList.clear();

    if (m_provider == Provider::GPodderNextcloud) {
        m_gpodder = new GPodder(username, password, m_hostname, Provider::GPodderNextcloud, this);

        SubscriptionRequest *subRequest = m_gpodder->getSubscriptionChanges(0, QLatin1String(""));
        connect(subRequest, &SubscriptionRequest::finished, this, [this, subRequest, username, password]() {
            if (subRequest->error() || subRequest->aborted()) {
                if (subRequest->error()) {
                    Q_EMIT error(Error::Type::SyncError,
                                 QLatin1String(""),
                                 QLatin1String(""),
                                 subRequest->error(),
                                 subRequest->errorString(),
                                 i18n("Could not log into GPodder-nextcloud server"));
                }
                if (m_syncEnabled) {
                    setSyncEnabled(false);
                }
            } else {
                connect(this, &Sync::passwordSaveFinished, this, [this, username, password](bool success) {
                    disconnect(this, &Sync::passwordSaveFinished, this, nullptr);
                    if (success) {
                        m_username = username;
                        m_password = password;
                        SettingsManager::self()->setSyncUsername(username);
                        SettingsManager::self()->save();
                        Q_EMIT credentialsChanged();

                        setSyncEnabled(true);
                        Q_EMIT loginSucceeded();
                    }
                });
                savePasswordToKeyChain(username, password);
            }
            subRequest->deleteLater();
        });
    } else {
        if (m_hostname.isEmpty()) { // official gpodder.net server
            m_gpodder = new GPodder(username, password, this);
        } else { // custom server
            m_gpodder = new GPodder(username, password, m_hostname, Provider::GPodderNet, this);
        }

        DeviceRequest *deviceRequest = m_gpodder->getDevices();
        connect(deviceRequest, &DeviceRequest::finished, this, [this, deviceRequest, username, password]() {
            if (deviceRequest->error() || deviceRequest->aborted()) {
                if (deviceRequest->error()) {
                    Q_EMIT error(Error::Type::SyncError,
                                 QLatin1String(""),
                                 QLatin1String(""),
                                 deviceRequest->error(),
                                 deviceRequest->errorString(),
                                 i18n("Could not log into GPodder server"));
                }
                m_gpodder->deleteLater();
                m_gpodder = nullptr;
                if (m_syncEnabled) {
                    setSyncEnabled(false);
                }
            } else {
                m_deviceList = deviceRequest->devices();

                connect(this, &Sync::passwordSaveFinished, this, [this, username, password](bool success) {
                    disconnect(this, &Sync::passwordSaveFinished, this, nullptr);
                    if (success) {
                        m_username = username;
                        m_password = password;
                        SettingsManager::self()->setSyncUsername(username);
                        SettingsManager::self()->save();
                        Q_EMIT credentialsChanged();

                        Q_EMIT loginSucceeded();
                        Q_EMIT deviceListReceived(); // required in order to open follow-up device-pick dialog
                    }
                });
                savePasswordToKeyChain(username, password);
            }
            deviceRequest->deleteLater();
        });
    }
}

void Sync::logout()
{
    if (m_provider == Provider::GPodderNextcloud) {
        clearSettings();
    } else {
        if (!m_gpodder) {
            clearSettings();
            return;
        }
        LogoutRequest *logoutRequest = m_gpodder->logout();
        connect(logoutRequest, &LogoutRequest::finished, this, [this, logoutRequest]() {
            if (logoutRequest->error() || logoutRequest->aborted()) {
                if (logoutRequest->error()) {
                    // Let's not report this error, since it doesn't matter anyway:
                    // 1) If we're not logged in, there's no problem
                    // 2) If we are logged in, but somehow cannot log out, then it
                    //    shouldn't matter either, since the session probably expired
                    /*
                    Q_EMIT error(Error::Type::SyncError,
                                QLatin1String(""),
                                QLatin1String(""),
                                logoutRequest->error(),
                                logoutRequest->errorString(),
                                i18n("Could not log out of GPodder server"));
                    */
                }
            }
            clearSettings();
        });
    }
}

void Sync::clearSettings()
{
    if (m_gpodder) {
        m_gpodder->deleteLater();
        m_gpodder = nullptr;
    }

    QSqlQuery query;
    // Delete pending EpisodeActions
    query.prepare(QStringLiteral("DELETE FROM EpisodeActions;"));
    Database::instance().execute(query);

    // Delete pending FeedActions
    query.prepare(QStringLiteral("DELETE FROM FeedActions;"));
    Database::instance().execute(query);

    // Delete SyncTimestamps
    query.prepare(QStringLiteral("DELETE FROM SyncTimestamps;"));
    Database::instance().execute(query);

    setSyncEnabled(false);

    // Delete password from keychain and password file
    deletePasswordFromKeychain(m_username);

    m_username.clear();
    m_password.clear();
    m_device.clear();
    m_deviceName.clear();
    m_hostname.clear();
    m_provider = Provider::GPodderNet;
    SettingsManager::self()->setSyncUsername(m_username);
    SettingsManager::self()->setSyncDevice(m_device);
    SettingsManager::self()->setSyncDeviceName(m_deviceName);
    SettingsManager::self()->setSyncHostname(m_hostname);
    SettingsManager::self()->setSyncProvider(static_cast<int>(m_provider));
    SettingsManager::self()->save();

    Q_EMIT credentialsChanged();
    Q_EMIT hostnameChanged();
    Q_EMIT syncProgressChanged();
}

void Sync::onWritePasswordJobFinished(QKeychain::WritePasswordJob *job, const QString &username, const QString &password)
{
    if (job->error()) {
        qCDebug(kastsSync) << "Could not save password to the keychain: " << qPrintable(job->errorString());
        // fall back to file
        savePasswordToFile(username, password);
    } else {
        qCDebug(kastsSync) << "Password saved to keychain";
        Q_EMIT passwordSaveFinished(true);
    }
    job->deleteLater();
}

void Sync::savePasswordToKeyChain(const QString &username, const QString &password)
{
    qCDebug(kastsSync) << "Save the password to the keychain for" << username;

    QKeychain::WritePasswordJob *job = new QKeychain::WritePasswordJob(qAppName(), this);
    job->setAutoDelete(false);
    job->setKey(username);
    job->setTextData(password);

    QKeychain::WritePasswordJob::connect(job, &QKeychain::Job::finished, this, [this, username, password, job]() {
        onWritePasswordJobFinished(job, username, password);
    });
    job->start();
}

void Sync::savePasswordToFile(const QString &username, const QString &password)
{
    qCDebug(kastsSync) << "Save the password to file for" << username;

    // NOTE: Store in the same location as database, which can be different from
    //       the storagePath
    QString filePath = StorageManager::instance().passwordFilePath(username);

    QFile passwordFile(filePath);
    passwordFile.remove();

    QDir fileDir = QFileInfo(passwordFile).dir();
    if (!((fileDir.exists() || fileDir.mkpath(QStringLiteral("."))) && passwordFile.open(QFile::WriteOnly))) {
        Q_EMIT error(Error::Type::SyncError,
                     passwordFile.fileName(),
                     QLatin1String(""),
                     0,
                     i18n("I/O denied: Cannot save password."),
                     i18n("I/O denied: Cannot save password."));
        Q_EMIT passwordSaveFinished(false);
    } else {
        passwordFile.write(password.toUtf8());
        passwordFile.close();
        Q_EMIT passwordSaveFinished(true);
    }
}

void Sync::retrievePasswordFromKeyChain(const QString &username)
{
    // Workaround: first try and store a dummy entry to the keychain to ensure
    // that the keychain is unlocked before we try to retrieve the real password

    QKeychain::WritePasswordJob *writeDummyJob = new QKeychain::WritePasswordJob(qAppName(), this);
    writeDummyJob->setAutoDelete(false);
    writeDummyJob->setKey(QStringLiteral("dummy"));
    writeDummyJob->setTextData(QStringLiteral("dummy"));

    QKeychain::WritePasswordJob::connect(writeDummyJob, &QKeychain::Job::finished, this, [this, writeDummyJob, username]() {
        if (writeDummyJob->error()) {
            qCDebug(kastsSync) << "Could not open keychain: " << qPrintable(writeDummyJob->errorString());
            // fall back to password from file
            Q_EMIT passwordRetrievalFinished(retrievePasswordFromFile(username));
        } else {
            // opening keychain succeeded, let's try to read the password

            QKeychain::ReadPasswordJob *readJob = new QKeychain::ReadPasswordJob(qAppName());
            readJob->setAutoDelete(false);
            readJob->setKey(username);

            connect(readJob, &QKeychain::Job::finished, this, [this, readJob, username]() {
                if (readJob->error() == QKeychain::Error::NoError) {
                    Q_EMIT passwordRetrievalFinished(readJob->textData());
                    // if a password file is present, delete it
                    QFile(StorageManager::instance().passwordFilePath(username)).remove();
                } else {
                    qCDebug(kastsSync) << "Could not read the access token from the keychain: " << qPrintable(readJob->errorString());
                    // no password from the keychain, try token file
                    QString password = retrievePasswordFromFile(username);
                    Q_EMIT passwordRetrievalFinished(password);
                    if (readJob->error() == QKeychain::Error::EntryNotFound) {
                        if (!password.isEmpty()) {
                            qCDebug(kastsSync) << "Migrating password from file to the keychain for " << username;
                            connect(this, &Sync::passwordSaveFinished, this, [this, username](bool saved) {
                                disconnect(this, &Sync::passwordSaveFinished, this, nullptr);
                                bool removed = false;
                                if (saved) {
                                    QFile passwordFile(StorageManager::instance().passwordFilePath(username));
                                    removed = passwordFile.remove();
                                }
                                if (!(saved && removed)) {
                                    qCDebug(kastsSync) << "Migrating password from the file to the keychain failed";
                                }
                            });
                            savePasswordToKeyChain(username, password);
                        }
                    }
                }
                readJob->deleteLater();
            });
            readJob->start();
        }
        writeDummyJob->deleteLater();
    });
    writeDummyJob->start();
}

QString Sync::retrievePasswordFromFile(const QString &username)
{
    QFile passwordFile(StorageManager::instance().passwordFilePath(username));

    if (passwordFile.open(QFile::ReadOnly)) {
        qCDebug(kastsSync) << "Retrieved password from file for user" << username;
        return QString::fromUtf8(passwordFile.readAll());
    } else {
        Q_EMIT error(Error::Type::SyncError,
                     passwordFile.fileName(),
                     QLatin1String(""),
                     0,
                     i18n("I/O denied: Cannot access password file."),
                     i18n("I/O denied: Cannot access password file."));

        return QLatin1String("");
    }
}

void Sync::onDeleteJobFinished(QKeychain::DeletePasswordJob *deleteJob, const QString &username)
{
    if (deleteJob->error() == QKeychain::Error::NoError) {
        qCDebug(kastsSync) << "Password for username" << username << "successfully deleted from keychain";

        // now also delete the dummy entry
        QKeychain::DeletePasswordJob *deleteDummyJob = new QKeychain::DeletePasswordJob(qAppName());
        deleteDummyJob->setAutoDelete(true);
        deleteDummyJob->setKey(QStringLiteral("dummy"));

        QKeychain::DeletePasswordJob::connect(deleteDummyJob, &QKeychain::Job::finished, this, [=]() {
            if (deleteDummyJob->error()) {
                qCDebug(kastsSync) << "Deleting dummy from keychain unsuccessful";
            } else {
                qCDebug(kastsSync) << "Deleting dummy from keychain successful";
            }
        });
        deleteDummyJob->start();
    } else if (deleteJob->error() == QKeychain::Error::EntryNotFound) {
        qCDebug(kastsSync) << "No password for username" << username << "found in keychain";
    } else {
        qCDebug(kastsSync) << "Could not access keychain to delete password for username" << username;
    }
}

void Sync::onWriteDummyJobFinished(QKeychain::WritePasswordJob *writeDummyJob, const QString &username)
{
    if (writeDummyJob->error()) {
        qCDebug(kastsSync) << "Could not open keychain: " << qPrintable(writeDummyJob->errorString());
    } else {
        // opening keychain succeeded, let's try to delete the password

        QFile(StorageManager::instance().passwordFilePath(username)).remove();

        QKeychain::DeletePasswordJob *deleteJob = new QKeychain::DeletePasswordJob(qAppName());
        deleteJob->setAutoDelete(true);
        deleteJob->setKey(username);

        QKeychain::DeletePasswordJob::connect(deleteJob, &QKeychain::Job::finished, this, [this, deleteJob, username]() {
            onDeleteJobFinished(deleteJob, username);
        });
        deleteJob->start();
    }
    writeDummyJob->deleteLater();
}

void Sync::deletePasswordFromKeychain(const QString &username)
{
    // Workaround: first try and store a dummy entry to the keychain to ensure
    // that the keychain is unlocked before we try to delete the real password

    QKeychain::WritePasswordJob *writeDummyJob = new QKeychain::WritePasswordJob(qAppName(), this);
    writeDummyJob->setAutoDelete(false);
    writeDummyJob->setKey(QStringLiteral("dummy"));
    writeDummyJob->setTextData(QStringLiteral("dummy"));

    QKeychain::WritePasswordJob::connect(writeDummyJob, &QKeychain::Job::finished, this, [this, writeDummyJob, username]() {
        onWriteDummyJobFinished(writeDummyJob, username);
    });
    writeDummyJob->start();
}

void Sync::registerNewDevice(const QString &id, const QString &caption, const QString &type)
{
    if (!m_gpodder) {
        return;
    }
    UpdateDeviceRequest *updateDeviceRequest = m_gpodder->updateDevice(id, caption, type);
    connect(updateDeviceRequest, &UpdateDeviceRequest::finished, this, [this, updateDeviceRequest, id, caption]() {
        if (updateDeviceRequest->error() || updateDeviceRequest->aborted()) {
            if (updateDeviceRequest->error()) {
                Q_EMIT error(Error::Type::SyncError,
                             QLatin1String(""),
                             QLatin1String(""),
                             updateDeviceRequest->error(),
                             updateDeviceRequest->errorString(),
                             i18n("Could not create GPodder device"));
            }
        } else {
            setDevice(id);
            setDeviceName(caption);
            setSyncEnabled(true);
            Q_EMIT deviceCreated();
        }
        updateDeviceRequest->deleteLater();
    });
}

void Sync::linkUpAllDevices()
{
    if (!m_gpodder) {
        return;
    }
    SyncRequest *syncRequest = m_gpodder->getSyncStatus();
    connect(syncRequest, &SyncRequest::finished, this, [this, syncRequest]() {
        if (syncRequest->error() || syncRequest->aborted()) {
            if (syncRequest->error()) {
                Q_EMIT error(Error::Type::SyncError,
                             QLatin1String(""),
                             QLatin1String(""),
                             syncRequest->error(),
                             syncRequest->errorString(),
                             i18n("Could not retrieve synced device status"));
            }
            syncRequest->deleteLater();
            return;
        }

        QStringList syncDevices;
        QVector<QStringList> rawSyncedDevices = syncRequest->syncedDevices();
        for (const QStringList &group : std::as_const(rawSyncedDevices)) {
            syncDevices += group;
        }
        syncDevices += syncRequest->unsyncedDevices();

        syncDevices.removeDuplicates();
        QVector<QStringList> syncDeviceGroups;
        syncDeviceGroups += syncDevices;
        if (!m_gpodder) {
            return;
        }
        UpdateSyncRequest *upSyncRequest = m_gpodder->updateSyncStatus(syncDeviceGroups, QStringList());
        connect(upSyncRequest, &UpdateSyncRequest::finished, this, [this, upSyncRequest, syncDevices]() {
            // For some reason, the response is always "Internal Server Error"
            // even though the request is processed properly.  So we just
            // continue rather than abort...
            if (upSyncRequest->error() || upSyncRequest->aborted()) {
                if (upSyncRequest->error()) {
                    // Q_EMIT error(Error::Type::SyncError,
                    //            QLatin1String(""),
                    //            QLatin1String(""),
                    //            upSyncRequest->error(),
                    //            upSyncRequest->errorString(),
                    //            i18n("Could not update synced device status"));
                }
                // upSyncRequest->deleteLater();
                // return;
            }

            // Assemble a list of all subscriptions of all devices
            m_syncUpAllSubscriptions.clear();
            m_deviceResponses = 0;
            for (const QString &device : syncDevices) {
                if (!m_gpodder) {
                    return;
                }
                SubscriptionRequest *subRequest = m_gpodder->getSubscriptionChanges(0, device);
                connect(subRequest, &SubscriptionRequest::finished, this, [this, subRequest, device, syncDevices]() {
                    if (subRequest->error() || subRequest->aborted()) {
                        if (subRequest->error()) {
                            Q_EMIT error(Error::Type::SyncError,
                                         QLatin1String(""),
                                         QLatin1String(""),
                                         subRequest->error(),
                                         subRequest->errorString(),
                                         i18n("Could not retrieve subscriptions for device %1", device));
                        }
                    } else {
                        m_syncUpAllSubscriptions += subRequest->addList();
                    }
                    if (syncDevices.count() == ++m_deviceResponses) {
                        // We have now received all responses for all devices
                        for (const QString &syncdevice : syncDevices) {
                            if (!m_gpodder) {
                                return;
                            }
                            UploadSubscriptionRequest *upSubRequest = m_gpodder->uploadSubscriptionChanges(m_syncUpAllSubscriptions, QStringList(), syncdevice);
                            connect(upSubRequest, &UploadSubscriptionRequest::finished, this, [this, upSubRequest, syncdevice]() {
                                if (upSubRequest->error()) {
                                    Q_EMIT error(Error::Type::SyncError,
                                                 QLatin1String(""),
                                                 QLatin1String(""),
                                                 upSubRequest->error(),
                                                 upSubRequest->errorString(),
                                                 i18n("Could not upload subscriptions for device %1", syncdevice));
                                }
                                upSubRequest->deleteLater();
                            });
                        }
                    }
                    subRequest->deleteLater();
                });
            }
            upSyncRequest->deleteLater();
        });
        syncRequest->deleteLater();
    });
}

void Sync::doSync(SyncStatus status, bool forceFetchAll)
{
    if (!m_syncEnabled || !m_gpodder || !(m_syncStatus == SyncStatus::NoSync || m_syncStatus == SyncStatus::UploadOnlySync)) {
        return;
    }

    if (m_provider == Provider::GPodderNet && (m_username.isEmpty() || m_device.isEmpty())) {
        return;
    }

    if (m_provider == Provider::GPodderNextcloud && (m_username.isEmpty() || m_hostname.isEmpty())) {
        return;
    }

    // If a quick upload-only sync is running, abort it
    if (m_syncStatus == SyncStatus::UploadOnlySync) {
        Q_EMIT abortSync();
    }

    m_syncStatus = status;

    if (status == SyncUtils::SyncStatus::PushAllSync) {
        retrieveAllLocalEpisodeStates();
    }

    SyncJob *syncJob = new SyncJob(status, m_gpodder, m_device, forceFetchAll, this);
    connect(this, &Sync::abortSync, syncJob, &SyncJob::abort);
    connect(syncJob, &SyncJob::infoMessage, this, [this](KJob *job, const QString &message) {
        m_syncProgressTotal = job->totalAmount(KJob::Unit::Items);
        m_syncProgress = job->processedAmount(KJob::Unit::Items);
        m_syncProgressText = message;
        Q_EMIT syncProgressChanged();
    });
    connect(syncJob, &SyncJob::finished, this, [this](KJob *job) {
        if (job->error()) {
            Q_EMIT error(Error::Type::SyncError, QLatin1String(""), QLatin1String(""), job->error(), job->errorText(), job->errorString());
        }
        m_syncStatus = SyncStatus::NoSync;
        Q_EMIT syncProgressChanged();
    });
    syncJob->start();
}

void Sync::doRegularSync(bool forceFetchAll)
{
    doSync(SyncStatus::RegularSync, forceFetchAll);
}

void Sync::doForceSync()
{
    doSync(SyncStatus::ForceSync, true);
}

void Sync::doSyncPushAll()
{
    doSync(SyncStatus::PushAllSync, false);
}

void Sync::doQuickSync()
{
    if (!SettingsManager::self()->syncWhenPlayerstateChanges()) {
        return;
    }

    // since this method is supposed to be called automatically, we cannot check
    // the network state from the UI, so we have to do it here
    if (!NetworkConnectionManager::instance().feedUpdatesAllowed()) {
        qCDebug(kastsSync) << "Not uploading episode actions on metered connection due to settings";
        return;
    }

    if (!m_syncEnabled || !m_gpodder || m_syncStatus != SyncStatus::NoSync) {
        return;
    }

    if (m_provider == Provider::GPodderNet && (m_username.isEmpty() || m_device.isEmpty())) {
        return;
    }

    if (m_provider == Provider::GPodderNextcloud && (m_username.isEmpty() || m_hostname.isEmpty())) {
        return;
    }

    m_syncStatus = SyncStatus::UploadOnlySync;

    SyncJob *syncJob = new SyncJob(m_syncStatus, m_gpodder, m_device, false, this);
    connect(this, &Sync::abortSync, syncJob, &SyncJob::abort);
    connect(syncJob, &SyncJob::finished, this, [this]() {
        // don't do error reporting or status updates on quick upload-only syncs
        m_syncStatus = SyncStatus::NoSync;
    });
    syncJob->start();
}

void Sync::applySubscriptionChangesLocally(const QStringList &addList, const QStringList &removeList)
{
    m_allowSyncActionLogging = false;

    // removals
    DataManager::instance().removeFeeds(removeList);

    // additions
    DataManager::instance().addFeeds(addList, false);

    m_allowSyncActionLogging = true;
}

void Sync::applyEpisodeActionsLocally(const QHash<QString, QHash<QString, EpisodeAction>> &episodeActionHash)
{
    m_allowSyncActionLogging = false;

    for (const QHash<QString, EpisodeAction> &actions : episodeActionHash) {
        for (const EpisodeAction &action : actions) {
            if (action.action == QStringLiteral("play")) {
                Entry *entry = DataManager::instance().getEntry(action.id);
                if (entry && entry->hasEnclosure()) {
                    qCDebug(kastsSync) << action.position << action.total << static_cast<qint64>(action.position) << entry->enclosure()->duration()
                                       << SettingsManager::self()->markAsPlayedBeforeEnd();
                    if ((action.position >= action.total - SettingsManager::self()->markAsPlayedBeforeEnd()
                         || static_cast<qint64>(action.position) >= entry->enclosure()->duration() - SettingsManager::self()->markAsPlayedBeforeEnd())
                        && action.total > 0) {
                        // Episode has been played
                        qCDebug(kastsSync) << "mark as played:" << entry->title();
                        entry->setRead(true);
                    } else if (action.position > 0 && static_cast<qint64>(action.position) * 1000 >= entry->enclosure()->duration()) {
                        // Episode is being listened to
                        qCDebug(kastsSync) << "set play position and add to queue:" << entry->title();
                        entry->enclosure()->setPlayPosition(action.position * 1000);
                        entry->setQueueStatus(true);
                        if (AudioManager::instance().entry() == entry) {
                            AudioManager::instance().setPosition(action.position * 1000);
                        }
                    } else {
                        // Episode has not been listened to yet
                        qCDebug(kastsSync) << "reset play position:" << entry->title();
                        entry->enclosure()->setPlayPosition(0);
                    }
                }
            }

            if (action.action == QStringLiteral("delete")) {
                Entry *entry = DataManager::instance().getEntry(action.id);
                if (entry && entry->hasEnclosure()) {
                    // "delete" means that at least the Episode has been played
                    qCDebug(kastsSync) << "mark as played:" << entry->title();
                    entry->setRead(true);
                }
            }

            QCoreApplication::processEvents(); // keep the main thread semi-responsive
        }
    }

    m_allowSyncActionLogging = true;

    // Don't sync the download or delete status since it's broken in gpodder.net:
    // the service only allows to upload only one download or delete action per
    // episode; afterwards, it's not possible to override it with a similar action
    // with a newer timestamp.  Hence we consider this information not reliable.
}

void Sync::storeAddFeedAction(const QString &url)
{
    if (syncEnabled() && m_allowSyncActionLogging) {
        QSqlQuery query;
        query.prepare(QStringLiteral("INSERT INTO FeedActions (url, action, timestamp) VALUES (:url, :action, :timestamp);"));
        query.bindValue(QStringLiteral(":url"), url);
        query.bindValue(QStringLiteral(":action"), QStringLiteral("add"));
        query.bindValue(QStringLiteral(":timestamp"), QDateTime::currentSecsSinceEpoch());
        Database::instance().execute(query);
        qCDebug(kastsSync) << "Logged a feed add action for" << url;
    }
}

void Sync::storeRemoveFeedAction(const QString &url)
{
    if (syncEnabled() && m_allowSyncActionLogging) {
        QSqlQuery query;
        query.prepare(QStringLiteral("INSERT INTO FeedActions (url, action, timestamp) VALUES (:url, :action, :timestamp);"));
        query.bindValue(QStringLiteral(":url"), url);
        query.bindValue(QStringLiteral(":action"), QStringLiteral("remove"));
        query.bindValue(QStringLiteral(":timestamp"), QDateTime::currentSecsSinceEpoch());
        Database::instance().execute(query);
        qCDebug(kastsSync) << "Logged a feed remove action for" << url;
    }
}

void Sync::storePlayEpisodeAction(const QString &id, const qulonglong started, const qulonglong position)
{
    if (syncEnabled() && m_allowSyncActionLogging) {
        Entry *entry = DataManager::instance().getEntry(id);
        if (entry && entry->hasEnclosure()) {
            const qulonglong started_sec = started / 1000; // convert to seconds
            const qulonglong position_sec = position / 1000; // convert to seconds
            const qulonglong total =
                (entry->enclosure()->duration() > 0) ? entry->enclosure()->duration() : 1; // workaround for episodes with bad metadata on gpodder server

            QSqlQuery query;
            query.prepare(
                QStringLiteral("INSERT INTO EpisodeActions (podcast, url, id, action, started, position, total, timestamp) VALUES (:podcast, :url, :id, "
                               ":action, :started, :position, :total, :timestamp);"));
            query.bindValue(QStringLiteral(":podcast"), entry->feed()->url());
            query.bindValue(QStringLiteral(":url"), entry->enclosure()->url());
            query.bindValue(QStringLiteral(":id"), entry->id());
            query.bindValue(QStringLiteral(":action"), QStringLiteral("play"));
            query.bindValue(QStringLiteral(":started"), started_sec);
            query.bindValue(QStringLiteral(":position"), position_sec);
            query.bindValue(QStringLiteral(":total"), total);
            query.bindValue(QStringLiteral(":timestamp"), QDateTime::currentSecsSinceEpoch());
            Database::instance().execute(query);

            qCDebug(kastsSync) << "Logged an episode play action for" << entry->title() << "play position changed:" << started_sec << position_sec << total;
        }
    }
}

void Sync::storePlayedEpisodeAction(const QList<qint64> &entryuids)
{
    for (const qint64 entryuid : entryuids) {
        if (syncEnabled() && m_allowSyncActionLogging) {
            if (DataManager::instance().getEntry(entryuid)->hasEnclosure()) {
                Entry *entry = DataManager::instance().getEntry(entryuid);
                const qulonglong duration =
                    (entry->enclosure()->duration() > 0) ? entry->enclosure()->duration() : 1; // crazy workaround for episodes with bad metadata
                storePlayEpisodeAction(entry->id(), duration * 1000, duration * 1000);
            }
        }
    }
}

void Sync::retrieveAllLocalEpisodeStates()
{
    QVector<SyncUtils::EpisodeAction> actions;

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Enclosures INNER JOIN Entries ON Enclosures.id = Entries.id WHERE Entries.hasEnclosure = 1;"));
    Database::instance().execute(query);
    while (query.next()) {
        qulonglong position_sec = query.value(QStringLiteral("playposition")).toInt() / 1000;
        qulonglong duration = query.value(QStringLiteral("duration")).toInt();
        bool read = query.value(QStringLiteral("read")).toBool();
        if (read) {
            if (duration == 0)
                duration = 1; // crazy workaround for episodes with bad metadata
            position_sec = duration;
        }
        if (position_sec > 0 && duration > 0) {
            SyncUtils::EpisodeAction action;
            action.podcast = query.value(QStringLiteral("feed")).toString();
            action.id = query.value(QStringLiteral("id")).toString();
            action.url = query.value(QStringLiteral("url")).toString();
            action.started = position_sec;
            action.position = position_sec;
            action.total = duration;

            actions << action;

            qCDebug(kastsSync) << "Logged an episode play action for" << action.id << "play position:" << position_sec << duration << read;
        }
    }

    QSqlQuery writeQuery;
    Database::instance().transaction();
    for (SyncUtils::EpisodeAction &action : actions) {
        writeQuery.prepare(
            QStringLiteral("INSERT INTO EpisodeActions (podcast, url, id, action, started, position, total, timestamp) VALUES (:podcast, :url, :id, :action, "
                           ":started, :position, :total, :timestamp);"));
        writeQuery.bindValue(QStringLiteral(":podcast"), action.podcast);
        writeQuery.bindValue(QStringLiteral(":url"), action.url);
        writeQuery.bindValue(QStringLiteral(":id"), action.id);
        writeQuery.bindValue(QStringLiteral(":action"), QStringLiteral("play"));
        writeQuery.bindValue(QStringLiteral(":started"), action.started);
        writeQuery.bindValue(QStringLiteral(":position"), action.position);
        writeQuery.bindValue(QStringLiteral(":total"), action.total);
        writeQuery.bindValue(QStringLiteral(":timestamp"), QDateTime::currentSecsSinceEpoch());
        Database::instance().execute(writeQuery);
    }
    Database::instance().commit();
}
