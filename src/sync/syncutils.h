/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QMetaType>
#include <QString>

namespace SyncUtils
{
Q_NAMESPACE

// constants
const QString subscriptionTimestampLabel = QStringLiteral("syncsubscriptions");
const QString episodeTimestampLabel = QStringLiteral("syncepisodes");
const QString uploadSubscriptionTimestampLabel = QStringLiteral("uploadsyncsubscriptions");
const QString uploadEpisodeTimestampLabel = QStringLiteral("uploadsyncepisodes");
const int maxAmountEpisodeUploads = 30;

// enums
enum Provider {
    GPodderNet = 0,
    GPodderNextcloud,
};
Q_ENUM_NS(Provider)

enum SyncStatus {
    NoSync = 0,
    RegularSync,
    ForceSync,
    UploadOnlySync,
};

Q_ENUM_NS(SyncStatus)

// structs
struct EpisodeAction {
    QString podcast;
    QString url;
    QString id;
    QString device;
    QString action;
    qulonglong started;
    qulonglong position;
    qulonglong total;
    qulonglong timestamp;
};

struct Device {
    Q_GADGET
public:
    QString id;
    QString caption;
    QString type;
    int subscriptions;
    Q_PROPERTY(QString caption MEMBER caption)
    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString type MEMBER type)
    Q_PROPERTY(int subscriptions MEMBER subscriptions)
};
}
