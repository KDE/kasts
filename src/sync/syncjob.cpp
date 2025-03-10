/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "syncjob.h"
#include "synclogging.h"

#include <QDateTime>
#include <QDir>
#include <QSqlQuery>
#include <QString>
#include <QTimer>

#include <KLocalizedString>

#include "audiomanager.h"
#include "database.h"
#include "datamanager.h"
#include "entry.h"
#include "models/errorlogmodel.h"
#include "settingsmanager.h"
#include "sync/gpodder/episodeactionrequest.h"
#include "sync/gpodder/gpodder.h"
#include "sync/gpodder/subscriptionrequest.h"
#include "sync/gpodder/uploadepisodeactionrequest.h"
#include "sync/gpodder/uploadsubscriptionrequest.h"
#include "sync/sync.h"
#include "sync/syncutils.h"
#include "utils/fetchfeedsjob.h"

using namespace SyncUtils;

SyncJob::SyncJob(SyncStatus syncStatus, GPodder *gpodder, const QString &device, bool forceFetchAll, QObject *parent)
    : KJob(parent)
    , m_syncStatus(syncStatus)
    , m_gpodder(gpodder)
    , m_device(device)
    , m_forceFetchAll(forceFetchAll)
{
    connect(&Sync::instance(), &Sync::abortSync, this, &SyncJob::aborting);

    setProgressUnit(KJob::Unit::Items);
}

void SyncJob::start()
{
    QTimer::singleShot(0, this, &SyncJob::doSync);
}

void SyncJob::abort()
{
    m_abort = true;
    Q_EMIT aborting();
}

bool SyncJob::aborted()
{
    return m_abort;
}

QString SyncJob::errorString() const
{
    switch (error()) {
    case SyncJobError::SubscriptionDownloadError:
        return i18n("Could not retrieve subscription updates from server");
        break;
    case SyncJobError::SubscriptionUploadError:
        return i18n("Could not upload subscription changes to server");
        break;
    case SyncJobError::EpisodeDownloadError:
        return i18n("Could not retrieve episode updates from server");
        break;
    case SyncJobError::EpisodeUploadError:
        return i18n("Could not upload episode updates to server");
        break;
    case SyncJobError::InternalDataError:
        return i18n("Internal data error");
        break;
    default:
        return KJob::errorString();
        break;
    }
}

void SyncJob::doSync()
{
    switch (m_syncStatus) {
    case SyncStatus::RegularSync:
    case SyncStatus::PushAllSync:
        doRegularSync();
        break;
    case SyncStatus::ForceSync:
        doForceSync();
        break;
    case SyncStatus::UploadOnlySync:
        doQuickSync();
        break;
    case SyncStatus::NoSync:
    default:
        qCDebug(kastsSync) << "Something's wrong. Sync job started with invalid sync type.";
        setError(SyncJobError::InternalDataError);
        emitResult();
        break;
    }
}

void SyncJob::doRegularSync()
{
    setTotalAmount(KJob::Unit::Items, 8);
    setProcessedAmount(KJob::Unit::Items, 0);
    Q_EMIT infoMessage(this, getProgressMessage(Started));

    syncSubscriptions();
}

void SyncJob::doForceSync()
{
    // Delete SyncTimestamps such that all feed and episode actions will be
    // retrieved again from the server
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM SyncTimestamps;"));
    Database::instance().execute(query);

    m_forceFetchAll = true;
    doRegularSync();
}

void SyncJob::doQuickSync()
{
    setTotalAmount(KJob::Unit::Items, 2);
    setProcessedAmount(KJob::Unit::Items, 0);
    Q_EMIT infoMessage(this, getProgressMessage(Started));

    // Quick sync of local subscription changes
    std::pair<QStringList, QStringList> localChanges = getLocalSubscriptionChanges();
    // store the local changes in a member variable such that the exact changes can be deleted from DB when processed
    m_localSubscriptionChanges = localChanges;

    QStringList addList = localChanges.first;
    QStringList removeList = localChanges.second;
    removeSubscriptionChangeConflicts(addList, removeList);
    uploadSubscriptions(addList, removeList);

    // Quick sync of local episodeActions
    // store these actions in member variable to be able to delete these exact same changes from DB when processed
    m_localEpisodeActions = getLocalEpisodeActions();

    QHash<QString, QHash<QString, EpisodeAction>> localEpisodeActionHash;
    for (const EpisodeAction &action : m_localEpisodeActions) {
        addToHashIfNewer(localEpisodeActionHash, action);
    }
    qCDebug(kastsSync) << "local hash";
    debugEpisodeActionHash(localEpisodeActionHash);

    uploadEpisodeActions(createListFromHash(localEpisodeActionHash));
}

void SyncJob::syncSubscriptions()
{
    setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
    Q_EMIT infoMessage(this, getProgressMessage(SubscriptionDownload));

    bool subscriptionTimestampExists = false;
    qulonglong subscriptionTimestamp = 0;

    // First check the database for previous timestamps
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT timestamp FROM SyncTimeStamps WHERE syncservice=:syncservice;"));
    query.bindValue(QStringLiteral(":syncservice"), subscriptionTimestampLabel);
    Database::instance().execute(query);
    if (query.next()) {
        subscriptionTimestamp = query.value(0).toULongLong();
        subscriptionTimestampExists = true;
        qCDebug(kastsSync) << "Previous gpodder subscription timestamp" << subscriptionTimestamp;
    }

    // Check for local changes
    // If no timestamp exists then upload all subscriptions. Otherwise, check
    // the database for actions since previous sync.
    QStringList localAddFeedList, localRemoveFeedList;
    if (subscriptionTimestamp == 0) {
        query.prepare(QStringLiteral("SELECT url FROM Feeds;"));
        Database::instance().execute(query);
        while (query.next()) {
            localAddFeedList << query.value(QStringLiteral("url")).toString();
        }
    } else {
        std::pair<QStringList, QStringList> localChanges = getLocalSubscriptionChanges();
        // immediately store the local changes such that the exact changes can be deleted from DB when processed
        m_localSubscriptionChanges = localChanges;

        localAddFeedList = localChanges.first;
        localRemoveFeedList = localChanges.second;
    }

    removeSubscriptionChangeConflicts(localAddFeedList, localRemoveFeedList);

    if (!m_gpodder) {
        setError(SyncJobError::InternalDataError);
        Q_EMIT infoMessage(this, getProgressMessage(Error));
        emitResult();
        return;
    }
    // Check the gpodder service for updates
    SubscriptionRequest *subRequest = m_gpodder->getSubscriptionChanges(subscriptionTimestamp, m_device);
    connect(this, &SyncJob::aborting, subRequest, &SubscriptionRequest::abort);
    connect(subRequest, &SubscriptionRequest::finished, this, [this, subRequest, localAddFeedList, localRemoveFeedList, subscriptionTimestampExists]() {
        if (subRequest->error() || subRequest->aborted()) {
            if (subRequest->aborted()) {
                Q_EMIT infoMessage(this, getProgressMessage(Aborted));
                emitResult();
                return;
            } else if (subRequest->error()) {
                setError(SyncJobError::SubscriptionDownloadError);
                setErrorText(subRequest->errorString());
                Q_EMIT infoMessage(this, getProgressMessage(Error));
            }
            // If this is a force sync (i.e. processing all updates), then
            // continue with fetching podcasts updates, otherwise it's not
            // possible to update new episodes if the sync server happens to be
            // down or is not reachable.
            if (m_forceFetchAll) {
                QSqlQuery query;
                query.prepare(QStringLiteral("SELECT url FROM Feeds;"));
                Database::instance().execute(query);
                while (query.next()) {
                    QString url = query.value(0).toString();
                    if (!m_feedsToBeUpdatedSubs.contains(url)) {
                        m_feedsToBeUpdatedSubs += url;
                    }
                }
                m_feedUpdateTotal = m_feedsToBeUpdatedSubs.count();
                setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 2); // skip upload step
                Q_EMIT infoMessage(this, getProgressMessage(SubscriptionFetch));

                QTimer::singleShot(0, this, &SyncJob::fetchModifiedSubscriptions);
            } else {
                emitResult();
                return;
            }
        } else {
            qCDebug(kastsSync) << "Finished device update request";

            qulonglong newSubscriptionTimestamp = subRequest->timestamp();
            QStringList remoteAddFeedList, remoteRemoveFeedList;

            removeSubscriptionChangeConflicts(remoteAddFeedList, remoteRemoveFeedList);

            for (const QString &url : subRequest->addList()) {
                qCDebug(kastsSync) << "Sync add feed:" << url;
                if (DataManager::instance().feedExists(url)) {
                    qCDebug(kastsSync) << "this one we have; do nothing";
                } else {
                    qCDebug(kastsSync) << "this one we don't have; add this feed";
                    remoteAddFeedList << url;
                }
            }

            for (const QString &url : subRequest->removeList()) {
                qCDebug(kastsSync) << "Sync remove feed:" << url;
                if (DataManager::instance().feedExists(url)) {
                    qCDebug(kastsSync) << "this one we have; needs to be removed";
                    remoteRemoveFeedList << url;
                } else {
                    qCDebug(kastsSync) << "this one we don't have; it was already removed locally; do nothing";
                }
            }

            qCDebug(kastsSync) << "localAddFeedList" << localAddFeedList;
            qCDebug(kastsSync) << "localRemoveFeedList" << localRemoveFeedList;
            qCDebug(kastsSync) << "remoteAddFeedList" << remoteAddFeedList;
            qCDebug(kastsSync) << "remoteRemoveFeedList" << remoteRemoveFeedList;

            // Now we apply the remote changes locally:
            Sync::instance().applySubscriptionChangesLocally(remoteAddFeedList, remoteRemoveFeedList);

            // We defer fetching the new feeds, since we will fetch them later on.
            // if this is the first sync or a force sync, then add all local feeds to
            // be updated
            if (!subscriptionTimestampExists || m_forceFetchAll) {
                QSqlQuery query;
                query.prepare(QStringLiteral("SELECT url FROM Feeds;"));
                Database::instance().execute(query);
                while (query.next()) {
                    QString url = query.value(0).toString();
                    if (!m_feedsToBeUpdatedSubs.contains(url)) {
                        m_feedsToBeUpdatedSubs += url;
                    }
                }
            }

            // Add the new feeds to the list of feeds that need to be refreshed.
            // We check with feedExists to make sure not to add the same podcast
            // with a slightly different url
            for (const QString &url : remoteAddFeedList) {
                if (!DataManager::instance().feedExists(url)) {
                    m_feedsToBeUpdatedSubs += url;
                }
            }
            m_feedUpdateTotal = m_feedsToBeUpdatedSubs.count();

            qCDebug(kastsSync) << "newSubscriptionTimestamp" << newSubscriptionTimestamp;
            updateDBTimestamp(newSubscriptionTimestamp, subscriptionTimestampLabel);

            setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
            Q_EMIT infoMessage(this, getProgressMessage(SubscriptionUpload));

            QTimer::singleShot(0, this, [this, localAddFeedList, localRemoveFeedList]() {
                uploadSubscriptions(localAddFeedList, localRemoveFeedList);
            });
        }
    });
}

void SyncJob::uploadSubscriptions(const QStringList &localAddFeedUrlList, const QStringList &localRemoveFeedUrlList)
{
    if (localAddFeedUrlList.isEmpty() && localRemoveFeedUrlList.isEmpty()) {
        qCDebug(kastsSync) << "No subscription changes to upload to server";

        // if this is not a quick upload only sync, continue with the feed updates
        if (m_syncStatus != SyncStatus::UploadOnlySync) {
            QTimer::singleShot(0, this, &SyncJob::fetchModifiedSubscriptions);
        }

        // Delete the uploaded changes from the database
        removeAppliedSubscriptionChangesFromDB();

        setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
        Q_EMIT infoMessage(this, getProgressMessage(SubscriptionFetch));
    } else {
        qCDebug(kastsSync) << "Uploading subscription changes:\n\tadd" << localAddFeedUrlList << "\n\tremove" << localRemoveFeedUrlList;
        if (!m_gpodder) {
            setError(SyncJobError::InternalDataError);
            Q_EMIT infoMessage(this, getProgressMessage(Error));
            emitResult();
            return;
        }
        UploadSubscriptionRequest *upSubRequest = m_gpodder->uploadSubscriptionChanges(localAddFeedUrlList, localRemoveFeedUrlList, m_device);
        connect(this, &SyncJob::aborting, upSubRequest, &UploadSubscriptionRequest::abort);
        connect(upSubRequest, &UploadSubscriptionRequest::finished, this, [this, upSubRequest]() {
            if (upSubRequest->error() || upSubRequest->aborted()) {
                if (upSubRequest->aborted()) {
                    Q_EMIT infoMessage(this, getProgressMessage(Aborted));
                } else if (upSubRequest->error()) {
                    setError(SyncJobError::SubscriptionUploadError);
                    setErrorText(upSubRequest->errorString());
                    Q_EMIT infoMessage(this, getProgressMessage(Error));
                }
                emitResult();
                return;
            }

            // Upload has succeeded
            qulonglong timestamp = upSubRequest->timestamp();
            qCDebug(kastsSync) << "timestamp after uploading local changes" << timestamp;
            updateDBTimestamp(timestamp, (m_syncStatus == SyncStatus::UploadOnlySync) ? uploadSubscriptionTimestampLabel : subscriptionTimestampLabel);

            // Delete the uploaded changes from the database
            removeAppliedSubscriptionChangesFromDB();

            // TODO: deal with updateUrlsList -> needs on-the-fly feed URL renaming
            QVector<std::pair<QString, QString>> updateUrlsList = upSubRequest->updateUrls();
            qCDebug(kastsSync) << "updateUrlsList:" << updateUrlsList;

            // if this is a quick upload only sync, then stop here, otherwise continue with
            // updating feeds that were added remotely.

            setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
            Q_EMIT infoMessage(this, getProgressMessage(SubscriptionFetch));

            if (m_syncStatus != SyncStatus::UploadOnlySync) {
                QTimer::singleShot(0, this, &SyncJob::fetchModifiedSubscriptions);
            }
        });
    }
}

void SyncJob::fetchModifiedSubscriptions()
{
    // Update the feeds that need to be updated such that we can find the
    // episodes in the database when we are receiving the remote episode
    // actions.
    m_feedUpdateTotal = m_feedsToBeUpdatedSubs.count();
    m_feedUpdateProgress = 0;
    FetchFeedsJob *fetchFeedsJob = new FetchFeedsJob(m_feedsToBeUpdatedSubs, this);
    connect(this, &SyncJob::aborting, fetchFeedsJob, &FetchFeedsJob::abort);
    connect(fetchFeedsJob, &FetchFeedsJob::processedAmountChanged, this, [this, fetchFeedsJob](KJob *job, KJob::Unit unit, qulonglong amount) {
        qCDebug(kastsSync) << "FetchFeedsJob::processedAmountChanged:" << amount;
        Q_UNUSED(job);
        Q_ASSERT(unit == KJob::Unit::Items);
        m_feedUpdateProgress = amount;
        if (!fetchFeedsJob->aborted() && !fetchFeedsJob->error()) {
            Q_EMIT infoMessage(this, getProgressMessage(SubscriptionFetch));
        }
    });
    connect(fetchFeedsJob, &FetchFeedsJob::result, this, [this, fetchFeedsJob]() {
        qCDebug(kastsSync) << "Feed update finished";
        if (fetchFeedsJob->error() || fetchFeedsJob->aborted()) {
            if (fetchFeedsJob->aborted()) {
                Q_EMIT infoMessage(this, getProgressMessage(Aborted));
            } else if (fetchFeedsJob->error()) {
                // FetchFeedsJob takes care of its own error reporting
                Q_EMIT infoMessage(this, getProgressMessage(Error));
            }
            emitResult();
            return;
        }
        Q_EMIT infoMessage(this, getProgressMessage(SubscriptionFetch));
        qCDebug(kastsSync) << "Done updating subscriptions and fetching updates";

        // We're ready to sync the episode states now
        // increase the progress counter now already since fetchRemoteEpisodeActions
        // can be executed multiple times
        setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
        m_remoteEpisodeActionHash.clear();
        QTimer::singleShot(0, this, &SyncJob::fetchRemoteEpisodeActions);
    });
    fetchFeedsJob->start();
}

void SyncJob::fetchRemoteEpisodeActions()
{
    qCDebug(kastsSync) << "Start syncing episode states";
    Q_EMIT infoMessage(this, getProgressMessage(EpisodeDownload));

    qulonglong episodeTimestamp = 0;

    // First check the database for previous timestamps
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT timestamp FROM SyncTimeStamps WHERE syncservice=:syncservice;"));
    query.bindValue(QStringLiteral(":syncservice"), episodeTimestampLabel);
    Database::instance().execute(query);
    if (query.next()) {
        episodeTimestamp = query.value(0).toULongLong();
        qCDebug(kastsSync) << "Previous gpodder episode timestamp" << episodeTimestamp;
    }

    if (!m_gpodder) {
        setError(SyncJobError::InternalDataError);
        Q_EMIT infoMessage(this, getProgressMessage(Error));
        emitResult();
        return;
    }
    // Check the gpodder service for episode action updates
    EpisodeActionRequest *epRequest = m_gpodder->getEpisodeActions(episodeTimestamp, (episodeTimestamp == 0));
    connect(this, &SyncJob::aborting, epRequest, &EpisodeActionRequest::abort);
    connect(epRequest, &EpisodeActionRequest::finished, this, [this, epRequest]() {
        qCDebug(kastsSync) << "Finished episode action request";
        if (epRequest->error() || epRequest->aborted()) {
            if (epRequest->aborted()) {
                Q_EMIT infoMessage(this, getProgressMessage(Aborted));
            } else if (epRequest->error()) {
                setError(SyncJobError::EpisodeUploadError);
                setErrorText(epRequest->errorString());
                Q_EMIT infoMessage(this, getProgressMessage(Error));
            }
            emitResult();
            return;
        }

        qulonglong newEpisodeTimestamp = epRequest->timestamp();
        qulonglong currentTimestamp = static_cast<qulonglong>(QDateTime::currentSecsSinceEpoch());

        qCDebug(kastsSync) << newEpisodeTimestamp;

        for (const EpisodeAction &action : epRequest->episodeActions()) {
            addToHashIfNewer(m_remoteEpisodeActionHash, action);

            qCDebug(kastsSync) << action.podcast << action.url << action.id << action.device << action.action << action.started << action.position
                               << action.total << action.timestamp;
        }

        updateDBTimestamp(newEpisodeTimestamp, episodeTimestampLabel);

        // Check returned timestamp against current timestamp.  If they aren't
        // close enough (let's take 10 seconds), that means that there are still
        // more episode actions to be fetched from the server.
        if (newEpisodeTimestamp > (currentTimestamp - 10) || epRequest->episodeActions().isEmpty()) {
            QTimer::singleShot(0, this, &SyncJob::syncEpisodeStates);
        } else {
            qCDebug(kastsSync) << "Fetching another batch of episode actions" << newEpisodeTimestamp << currentTimestamp;
            QTimer::singleShot(0, this, &SyncJob::fetchRemoteEpisodeActions);
        }
    });
}

void SyncJob::syncEpisodeStates()
{
    // store the local actions in member variable to be able to delete these exact same changes from DB when processed

    m_localEpisodeActions = getLocalEpisodeActions();

    QHash<QString, QHash<QString, EpisodeAction>> localEpisodeActionHash;
    for (const EpisodeAction &action : m_localEpisodeActions) {
        addToHashIfNewer(localEpisodeActionHash, action);
    }

    qCDebug(kastsSync) << "local hash";
    debugEpisodeActionHash(localEpisodeActionHash);

    qCDebug(kastsSync) << "remote hash";
    debugEpisodeActionHash(m_remoteEpisodeActionHash);

    // now remove conflicts between local and remote episode actions
    // based on the timestamp
    removeEpisodeActionConflicts(localEpisodeActionHash, m_remoteEpisodeActionHash);

    qCDebug(kastsSync) << "local hash";
    debugEpisodeActionHash(localEpisodeActionHash);

    qCDebug(kastsSync) << "remote hash";
    debugEpisodeActionHash(m_remoteEpisodeActionHash);

    // Now we update the feeds that need updating (don't update feeds that have
    // already been updated after the subscriptions were updated).
    for (const QString &url : getFeedsFromHash(m_remoteEpisodeActionHash)) {
        if (!m_feedsToBeUpdatedSubs.contains(url) && !m_feedsToBeUpdatedEps.contains(url)) {
            m_feedsToBeUpdatedEps += url;
        }
    }
    qCDebug(kastsSync) << "Feeds to be updated:" << m_feedsToBeUpdatedEps;
    m_feedUpdateTotal = m_feedsToBeUpdatedEps.count();
    m_feedUpdateProgress = 0;

    setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
    Q_EMIT infoMessage(this, getProgressMessage(SubscriptionFetch));

    FetchFeedsJob *fetchFeedsJob = new FetchFeedsJob(m_feedsToBeUpdatedEps, this);
    connect(this, &SyncJob::aborting, fetchFeedsJob, &FetchFeedsJob::abort);
    connect(fetchFeedsJob, &FetchFeedsJob::processedAmountChanged, this, [this, fetchFeedsJob](KJob *job, KJob::Unit unit, qulonglong amount) {
        qCDebug(kastsSync) << "FetchFeedsJob::processedAmountChanged:" << amount;
        Q_UNUSED(job);
        Q_ASSERT(unit == KJob::Unit::Items);
        m_feedUpdateProgress = amount;
        if (!fetchFeedsJob->aborted() && !fetchFeedsJob->error()) {
            Q_EMIT infoMessage(this, getProgressMessage(SubscriptionFetch));
        }
    });
    connect(fetchFeedsJob, &FetchFeedsJob::result, this, [this, fetchFeedsJob, localEpisodeActionHash]() {
        qCDebug(kastsSync) << "Feed update finished";
        if (fetchFeedsJob->error() || fetchFeedsJob->aborted()) {
            if (fetchFeedsJob->aborted()) {
                Q_EMIT infoMessage(this, getProgressMessage(Aborted));
            } else if (fetchFeedsJob->error()) {
                // FetchFeedsJob takes care of its own error reporting
                Q_EMIT infoMessage(this, getProgressMessage(Error));
            }
            emitResult();
            return;
        }
        Q_EMIT infoMessage(this, getProgressMessage(SubscriptionFetch));

        // Apply the remote changes locally
        setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
        Q_EMIT infoMessage(this, getProgressMessage(ApplyEpisodeActions));

        Sync::instance().applyEpisodeActionsLocally(m_remoteEpisodeActionHash);

        // Upload the local changes to the server
        QVector<EpisodeAction> localEpisodeActionList = createListFromHash(localEpisodeActionHash);

        setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
        Q_EMIT infoMessage(this, getProgressMessage(EpisodeUpload));
        // Now upload the episode actions to the server
        QTimer::singleShot(0, this, [this, localEpisodeActionList]() {
            uploadEpisodeActions(localEpisodeActionList);
        });
    });
    fetchFeedsJob->start();
}

void SyncJob::uploadEpisodeActions(const QVector<EpisodeAction> &episodeActions)
{
    // We have to upload episode actions in batches because otherwise the server
    // will reject them.
    uploadEpisodeActionsPartial(episodeActions, 0);
}

void SyncJob::uploadEpisodeActionsPartial(const QVector<EpisodeAction> &episodeActionList, const int startIndex)
{
    if (episodeActionList.count() == 0) {
        // nothing to upload; we don't have to contact the server
        qCDebug(kastsSync) << "No episode actions to upload to server";

        removeAppliedEpisodeActionsFromDB();

        setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
        Q_EMIT infoMessage(this, getProgressMessage(Finished));
        emitResult();
        return;
    }

    qCDebug(kastsSync) << "Uploading episode actions" << startIndex << "to"
                       << std::min(startIndex + maxAmountEpisodeUploads, static_cast<int>(episodeActionList.count())) << "of" << episodeActionList.count()
                       << "total episode actions";

    if (!m_gpodder) {
        setError(SyncJobError::InternalDataError);
        Q_EMIT infoMessage(this, getProgressMessage(Error));
        emitResult();
        return;
    }
    UploadEpisodeActionRequest *upEpRequest = m_gpodder->uploadEpisodeActions(episodeActionList.mid(startIndex, maxAmountEpisodeUploads));
    connect(this, &SyncJob::aborting, upEpRequest, &UploadEpisodeActionRequest::abort);
    connect(upEpRequest, &UploadEpisodeActionRequest::finished, this, [this, upEpRequest, episodeActionList, startIndex]() {
        qCDebug(kastsSync) << "Finished uploading batch of episode actions to server";
        if (upEpRequest->error() || upEpRequest->aborted()) {
            if (upEpRequest->aborted()) {
                Q_EMIT infoMessage(this, getProgressMessage(Aborted));
            } else if (upEpRequest->error()) {
                setError(SyncJobError::EpisodeUploadError);
                setErrorText(upEpRequest->errorString());
                Q_EMIT infoMessage(this, getProgressMessage(Error));
            }
            emitResult();
            return;
        }

        if (episodeActionList.count() > startIndex + maxAmountEpisodeUploads) {
            // Still episodeActions remaining to be uploaded
            QTimer::singleShot(0, this, [this, &episodeActionList, startIndex]() {
                uploadEpisodeActionsPartial(episodeActionList, startIndex + maxAmountEpisodeUploads);
            });
        } else {
            // All episodeActions have been uploaded

            qCDebug(kastsSync) << "New uploadEpisodeTimestamp from server" << upEpRequest->timestamp();
            updateDBTimestamp(upEpRequest->timestamp(), (m_syncStatus == SyncStatus::UploadOnlySync) ? uploadEpisodeTimestampLabel : episodeTimestampLabel);

            removeAppliedEpisodeActionsFromDB();

            setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
            Q_EMIT infoMessage(this, getProgressMessage(Finished));

            // This is the final exit point for the Job unless an error or abort occured
            qCDebug(kastsSync) << "Syncing finished";
            emitResult();
        }
    });
}

void SyncJob::updateDBTimestamp(const qulonglong &timestamp, const QString &timestampLabel)
{
    if (timestamp > 1) { // only accept timestamp if it's larger than zero
        bool timestampExists = false;
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT timestamp FROM SyncTimeStamps WHERE syncservice=:syncservice;"));
        query.bindValue(QStringLiteral(":syncservice"), timestampLabel);
        Database::instance().execute(query);
        if (query.next()) {
            timestampExists = true;
        }

        if (timestampExists) {
            query.prepare(QStringLiteral("UPDATE SyncTimeStamps SET timestamp=:timestamp WHERE syncservice=:syncservice;"));
        } else {
            query.prepare(QStringLiteral("INSERT INTO SyncTimeStamps VALUES (:syncservice, :timestamp);"));
        }
        query.bindValue(QStringLiteral(":syncservice"), timestampLabel);
        query.bindValue(QStringLiteral(":timestamp"), timestamp + 1); // add 1 second to avoid fetching our own previously sent updates next time
        Database::instance().execute(query);
    }
}

void SyncJob::removeAppliedSubscriptionChangesFromDB()
{
    Database::instance().transaction();
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM FeedActions WHERE url=:url AND action=:action;"));

    for (const QString &url : m_localSubscriptionChanges.first) {
        query.bindValue(QStringLiteral(":url"), url);
        query.bindValue(QStringLiteral(":action"), QStringLiteral("add"));
        Database::instance().execute(query);
    }

    for (const QString &url : m_localSubscriptionChanges.second) {
        query.bindValue(QStringLiteral(":url"), url);
        query.bindValue(QStringLiteral(":action"), QStringLiteral("remove"));
        Database::instance().execute(query);
    }
    Database::instance().commit();
}

void SyncJob::removeAppliedEpisodeActionsFromDB()
{
    Database::instance().transaction();
    QSqlQuery query;
    query.prepare(
        QStringLiteral("DELETE FROM EpisodeActions WHERE podcast=:podcast AND url=:url AND id=:id AND action=:action AND started=:started AND "
                       "position=:position AND total=:total AND timestamp=:timestamp;"));
    for (const EpisodeAction &epAction : m_localEpisodeActions) {
        qCDebug(kastsSync) << "Removing episode action from DB" << epAction.id;
        query.bindValue(QStringLiteral(":podcast"), epAction.podcast);
        query.bindValue(QStringLiteral(":url"), epAction.url);
        query.bindValue(QStringLiteral(":id"), epAction.id);
        query.bindValue(QStringLiteral(":action"), epAction.action);
        query.bindValue(QStringLiteral(":started"), epAction.started);
        query.bindValue(QStringLiteral(":position"), epAction.position);
        query.bindValue(QStringLiteral(":total"), epAction.total);
        query.bindValue(QStringLiteral(":timestamp"), epAction.timestamp);
        Database::instance().execute(query);
    }
    Database::instance().commit();
}

void SyncJob::removeSubscriptionChangeConflicts(QStringList &addList, QStringList &removeList)
{
    // Do some sanity checks and cleaning-up
    addList.removeDuplicates();
    removeList.removeDuplicates();
    for (const QString &addUrl : addList) {
        if (removeList.contains(addUrl)) {
            addList.removeAt(addList.indexOf(addUrl));
            removeList.removeAt(removeList.indexOf(addUrl));
        }
    }
    for (const QString &removeUrl : removeList) {
        if (addList.contains(removeUrl)) {
            removeList.removeAt(removeList.indexOf(removeUrl));
            addList.removeAt(addList.indexOf(removeUrl));
        }
    }
}

QVector<EpisodeAction> SyncJob::createListFromHash(const QHash<QString, QHash<QString, EpisodeAction>> &episodeActionHash)
{
    QVector<EpisodeAction> episodeActionList;

    for (const QHash<QString, EpisodeAction> &actions : episodeActionHash) {
        for (const EpisodeAction &action : actions) {
            if (action.action == QStringLiteral("play")) {
                episodeActionList << action;
            }
        }
    }

    return episodeActionList;
}

std::pair<QStringList, QStringList> SyncJob::getLocalSubscriptionChanges() const
{
    std::pair<QStringList, QStringList> localChanges;
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM FeedActions;"));
    Database::instance().execute(query);
    while (query.next()) {
        QString url = query.value(QStringLiteral("url")).toString();
        QString action = query.value(QStringLiteral("action")).toString();
        // qulonglong timestamp = query.value(QStringLiteral("timestamp")).toULongLong();
        if (action == QStringLiteral("add")) {
            localChanges.first << url;
        } else if (action == QStringLiteral("remove")) {
            localChanges.second << url;
        }
    }

    return localChanges;
}

QVector<EpisodeAction> SyncJob::getLocalEpisodeActions() const
{
    QVector<EpisodeAction> localEpisodeActions;
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM EpisodeActions;"));
    Database::instance().execute(query);
    while (query.next()) {
        QString podcast = query.value(QStringLiteral("podcast")).toString();
        QString url = query.value(QStringLiteral("url")).toString();
        QString id = query.value(QStringLiteral("id")).toString();
        QString action = query.value(QStringLiteral("action")).toString();
        qulonglong started = query.value(QStringLiteral("started")).toULongLong();
        qulonglong position = query.value(QStringLiteral("position")).toULongLong();
        qulonglong total = query.value(QStringLiteral("total")).toULongLong();
        qulonglong timestamp = query.value(QStringLiteral("timestamp")).toULongLong();
        EpisodeAction episodeAction = {podcast, url, id, m_device, action, started, position, total, timestamp};
        localEpisodeActions += episodeAction;
    }

    return localEpisodeActions;
}

void SyncJob::addToHashIfNewer(QHash<QString, QHash<QString, EpisodeAction>> &episodeActionHash, const EpisodeAction &episodeAction)
{
    if (episodeAction.action == QStringLiteral("play")) {
        if (episodeActionHash.contains(episodeAction.id) && episodeActionHash[episodeAction.id].contains(QStringLiteral("play"))) {
            if (episodeActionHash[episodeAction.id][QStringLiteral("play")].timestamp <= episodeAction.timestamp) {
                episodeActionHash[episodeAction.id][QStringLiteral("play")] = episodeAction;
            }
        } else {
            episodeActionHash[episodeAction.id][QStringLiteral("play")] = episodeAction;
        }
    }

    if (episodeAction.action == QStringLiteral("download") || episodeAction.action == QStringLiteral("delete")) {
        if (episodeActionHash.contains(episodeAction.id)) {
            if (episodeActionHash[episodeAction.id].contains(QStringLiteral("download"))) {
                if (episodeActionHash[episodeAction.id][QStringLiteral("download")].timestamp <= episodeAction.timestamp) {
                    episodeActionHash[episodeAction.id][QStringLiteral("download-delete")] = episodeAction;
                }
            } else if (episodeActionHash[episodeAction.id].contains(QStringLiteral("delete"))) {
                if (episodeActionHash[episodeAction.id][QStringLiteral("delete")].timestamp <= episodeAction.timestamp) {
                    episodeActionHash[episodeAction.id][QStringLiteral("download-delete")] = episodeAction;
                }
            } else {
                episodeActionHash[episodeAction.id][QStringLiteral("download-delete")] = episodeAction;
            }
        } else {
            episodeActionHash[episodeAction.id][QStringLiteral("download-delete")] = episodeAction;
        }
    }

    if (episodeAction.action == QStringLiteral("new")) {
        if (episodeActionHash.contains(episodeAction.id) && episodeActionHash[episodeAction.id].contains(QStringLiteral("new"))) {
            if (episodeActionHash[episodeAction.id][QStringLiteral("new")].timestamp <= episodeAction.timestamp) {
                episodeActionHash[episodeAction.id][QStringLiteral("new")] = episodeAction;
            }
        } else {
            episodeActionHash[episodeAction.id][QStringLiteral("new")] = episodeAction;
        }
    }
}

void SyncJob::removeEpisodeActionConflicts(QHash<QString, QHash<QString, EpisodeAction>> &local, QHash<QString, QHash<QString, EpisodeAction>> &remote)
{
    QStringList actions;
    actions << QStringLiteral("play") << QStringLiteral("download-delete") << QStringLiteral("new");

    // We first remove the conflicts from the hash with local changes
    for (const QHash<QString, EpisodeAction> &hashItem : remote) {
        for (const QString &action : actions) {
            QString id = hashItem[action].id;
            if (local.contains(id) && local.value(id).contains(action)) {
                if (local[id][action].timestamp < remote[id][action].timestamp) {
                    local[id].remove(action);
                }
            }
        }
    }

    // And now the same for the remote
    for (const QHash<QString, EpisodeAction> &hashItem : local) {
        for (const QString &action : actions) {
            QString id = hashItem[action].id;
            if (remote.contains(id) && remote.value(id).contains(action)) {
                if (remote[id][action].timestamp < local[id][action].timestamp) {
                    remote[id].remove(action);
                }
            }
        }
    }
}

QStringList SyncJob::getFeedsFromHash(const QHash<QString, QHash<QString, EpisodeAction>> &hash)
{
    QStringList feedUrls;
    for (const QHash<QString, EpisodeAction> &actionList : hash) {
        for (const EpisodeAction &action : actionList) {
            feedUrls += action.podcast;
        }
    }
    return feedUrls;
}

void SyncJob::debugEpisodeActionHash(const QHash<QString, QHash<QString, EpisodeAction>> &hash)
{
    for (const QHash<QString, EpisodeAction> &hashItem : hash) {
        for (const EpisodeAction &action : hashItem) {
            qCDebug(kastsSync) << action.podcast << action.url << action.id << action.device << action.action << action.started << action.position
                               << action.total << action.timestamp;
        }
    }
}

QString SyncJob::getProgressMessage(SyncJobStatus status) const
{
    int processed = processedAmount(KJob::Unit::Items);
    int total = totalAmount(KJob::Unit::Items);

    switch (status) {
    case Started:
        return i18nc("Subscription/Episode sync progress step", "(Step %1 of %2) Start sync", processed, total);
        break;
    case SubscriptionDownload:
        return i18nc("Subscription/Episode sync progress step", "(Step %1 of %2) Requesting remote subscription updates", processed, total);
        break;
    case SubscriptionUpload:
        return i18nc("Subscription/Episode sync progress step", "(Step %1 of %2) Uploading local subscription updates", processed, total);
        break;
    case SubscriptionFetch:
        return i18ncp("Subscription/Episode sync progress step",
                      "(Step %3 of %4) Updated %2 of %1 podcast",
                      "(Step %3 of %4) Updated %2 of %1 podcasts",
                      m_feedUpdateTotal,
                      m_feedUpdateProgress,
                      processed,
                      total);
        break;
    case EpisodeDownload:
        return i18nc("Subscription/Episode sync progress step", "(Step %1 of %2) Requesting remote episode updates", processed, total);
        break;
    case ApplyEpisodeActions:
        return i18nc("Subscription/Episode sync progress step", "(Step %1 of %2) Applying remote episode changes", processed, total);
        break;
    case EpisodeUpload:
        return i18nc("Subscription/Episode sync progress step", "(Step %1 of %2) Uploading local episode updates", processed, total);
        break;
    case Finished:
        return i18nc("Subscription/Episode sync progress step", "(Step %1 of %2) Finished sync", processed, total);
        break;
    case Aborted:
        return i18nc("Subscription/Episode sync progress step", "Sync aborted");
        break;
    case Error:
    default:
        return i18nc("Subscription/Episode sync progress step", "Sync finished with error");
        break;
    }
}
