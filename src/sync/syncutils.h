/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QMetaType>
#include <QQmlEngine>
#include <QString>

namespace SyncUtils
{
Q_NAMESPACE
QML_ELEMENT

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
    PushAllSync,
};

Q_ENUM_NS(SyncStatus)

// structs
struct EpisodeAction {
    qint64 entryuid;
    qint64 feeduid;
    QString podcast;
    QString url;
    QString id;
    QString device;
    QString action;
    qulonglong started;
    qulonglong position;
    qulonglong total;
    qint64 durationdb; // This holds the duration of the enclosure as determined by Kasts
    qulonglong timestamp;
};

struct Device {
    Q_GADGET
    QML_VALUE_TYPE(device)
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
