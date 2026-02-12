/**
 * SPDX-FileCopyrightText: 2025 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QMetaType>
#include <QQmlEngine>
#include <QString>

#include "enclosure.h"

namespace DataTypes
{
Q_NAMESPACE
QML_ELEMENT

// enums
enum RecordState {
    Unmodified = 0,
    New,
    Modified,
    Deleted,
};
Q_ENUM_NS(RecordState)

// structs
struct AuthorDetails {
    QString name;
    QString email;
    RecordState state;

    // Fields that are only used in case state == Modified
    QString oldEmail;
};

struct EnclosureDetails {
    qint64 enclosureuid;
    int duration;
    int size;
    QString type;
    QString url;
    int playPosition;
    Enclosure::Status downloaded;
    RecordState state;

    // Fields that are only used in case state == Modified
    int oldDuration;
    int oldSize;
    QString oldType;
    QString oldUrl;
};

struct ChapterDetails {
    int start;
    QString title;
    QString link;
    QString image;
    RecordState state;

    // Fields that are only used in case state == Modified
    QString oldTitle;
    QString oldLink;
    QString oldImage;
};

struct EntryDetails {
    qint64 entryuid;
    qint64 feeduid;
    QString id;
    QString title;
    QString content;
    int created;
    int updated;
    QString link;
    bool read;
    bool isNew;
    bool favorite;
    bool hasEnclosure;
    QString image;
    QHash<QString, AuthorDetails> authors; // key = author name
    QHash<QString, EnclosureDetails> enclosures; // key = enclosure url
    QHash<int, ChapterDetails> chapters; // key = start
    RecordState state;

    // Fields that are only used in case state == Modified
    QString oldTitle;
    QString oldContent;
    int oldCreated;
    int oldUpdated;
    QString oldLink;
    bool oldHasEnclosure; // TODO: probably don't need this since there is the enclosure QHash anyway
    QString oldImage;
};

struct FeedDetails {
    qint64 feeduid;
    QString name;
    QString url;
    QString image;
    QString link;
    QString description;
    int subscribed;
    int lastUpdated;
    bool isNew;
    QString dirname;
    QString lastHash;
    int filterType = 0;
    int sortType = 0;
    QHash<QString, AuthorDetails> authors; // key = author name
    QHash<QString, EntryDetails> entries; // key = id from feed
    RecordState state;

    // Fields that are only used in case state == Modified
    QString oldName;
    QString oldUrl;
    QString oldImage;
    QString oldLink;
    QString oldDescription;
    int oldLastUpdated;
    QString oldDirname;
    QString oldLastHash;
};
}
