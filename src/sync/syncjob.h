/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QHash>
#include <QObject>
#include <QStringList>

#include <KJob>

#include "error.h"
#include "sync/syncutils.h"

class GPodder;

class SyncJob : public KJob
{
    Q_OBJECT

public:
    enum SyncJobError {
        SubscriptionDownloadError = KJob::UserDefinedError,
        SubscriptionUploadError,
        EpisodeDownloadError,
        EpisodeUploadError,
        InternalDataError,
    };

    enum SyncJobStatus {
        Started = 0,
        SubscriptionDownload,
        SubscriptionUpload,
        EpisodeDownload,
        ApplyEpisodeActions,
        EpisodeUpload,
        SubscriptionFetch,
        Finished,
        Aborted,
        Error,
    };

    SyncJob(SyncUtils::SyncStatus syncStatus, GPodder *gpodder, const QString &device, bool forceFetchAll, QObject *parent);

    void start() override;
    void abort();

    bool aborted();
    QString errorString() const override;

Q_SIGNALS:
    void aborting();

private:
    void doSync();

    void doRegularSync(); // regular sync; can be forced to update all feeds
    void doForceSync(); // force a full re-sync with the server; discarding local eposide acions
    void doQuickSync(); // only upload pending local episode actions; intended to be run directly after an episode action has been created

    void syncSubscriptions();
    void uploadSubscriptions(const QStringList &localAddFeedUrlList, const QStringList &localRemoveFeedUrlList);
    void fetchModifiedSubscriptions();
    void fetchRemoteEpisodeActions();
    void syncEpisodeStates();
    void uploadEpisodeActions(const QVector<SyncUtils::EpisodeAction> &episodeActions);
    void uploadEpisodeActionsPartial(const QVector<SyncUtils::EpisodeAction> &episodeActionList, const int startIndex);
    void updateDBTimestamp(const qulonglong &timestamp, const QString &timestampLabel);

    void removeAppliedSubscriptionChangesFromDB();
    void removeAppliedEpisodeActionsFromDB();

    QPair<QStringList, QStringList> getLocalSubscriptionChanges() const; // First list are additions, second are removals
    QVector<SyncUtils::EpisodeAction> getLocalEpisodeActions() const;

    void removeSubscriptionChangeConflicts(QStringList &addList, QStringList &removeList);
    QVector<SyncUtils::EpisodeAction> createListFromHash(const QHash<QString, QHash<QString, SyncUtils::EpisodeAction>> &episodeActionHash);
    void addToHashIfNewer(QHash<QString, QHash<QString, SyncUtils::EpisodeAction>> &episodeActionHash, const SyncUtils::EpisodeAction &episodeAction);
    void removeEpisodeActionConflicts(QHash<QString, QHash<QString, SyncUtils::EpisodeAction>> &local,
                                      QHash<QString, QHash<QString, SyncUtils::EpisodeAction>> &remote);
    QStringList getFeedsFromHash(const QHash<QString, QHash<QString, SyncUtils::EpisodeAction>> &hash);
    void debugEpisodeActionHash(const QHash<QString, QHash<QString, SyncUtils::EpisodeAction>> &hash);

    SyncUtils::SyncStatus m_syncStatus = SyncUtils::SyncStatus::NoSync;
    GPodder *m_gpodder = nullptr;
    QString m_device;
    bool m_forceFetchAll = false;
    bool m_abort = false;

    // internal variables used while syncing
    QStringList m_feedsToBeUpdatedSubs;
    QStringList m_feedsToBeUpdatedEps;
    int m_feedUpdateProgress = 0;
    int m_feedUpdateTotal = 0;
    QPair<QStringList, QStringList> m_localSubscriptionChanges;
    QVector<SyncUtils::EpisodeAction> m_localEpisodeActions;
    QHash<QString, QHash<QString, SyncUtils::EpisodeAction>> m_remoteEpisodeActionHash;

    // needed for UI notifications
    QString getProgressMessage(SyncJobStatus status) const;
};
