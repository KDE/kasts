/**
 * SPDX-FileCopyrightText: 2025 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QMetaType>
#include <QQmlEngine>
#include <QString>

namespace DataTypes
{
Q_NAMESPACE
QML_ELEMENT

// enums and conversion functions
enum EnclosureStatus {
    Error = -1,
    Downloadable = 0,
    Downloading = 1,
    Queued = 2,
    PartiallyDownloaded = 3,
    Downloaded = 4,
    NoEnclosure = 5,
};
Q_ENUM_NS(EnclosureStatus)
inline int statusToDb(DataTypes::EnclosureStatus status) // needed to translate Enclosure::Status values to int for sqlite
{
    return static_cast<int>(status);
};
inline DataTypes::EnclosureStatus dbToStatus(int value) // needed to translate from int to Enclosure::Status values for sqlite
{
    return DataTypes::EnclosureStatus(value);
};

enum RecordState {
    Unmodified = 0,
    New,
    Modified,
    Deleted,
};
Q_ENUM_NS(RecordState)

// structs
struct EnclosureDetails {
    qint64 enclosureuid = 0;
    qint64 duration;
    qint64 size;
    qint64 downloadSize;
    QString type;
    QString url;
    DataTypes::EnclosureStatus status = DataTypes::EnclosureStatus::NoEnclosure;
};

struct EntryDetails {
    qint64 entryuid = 0;
    qint64 feeduid;
    QString id;
    QString title;
    QString content;
    qint64 created;
    qint64 updated;
    QString link;
    QString authors; // only one string with combined authors
    bool read;
    bool isNew;
    bool favorite;
    qint64 playPosition;
    bool removed;
    bool hasEnclosure = false;
    QString image;
    EnclosureDetails enclosure; // only show one enclosure at a time
};

struct FeedDetails {
    qint64 feeduid = 0;
    QString name;
    QString url;
    QString image;
    QString link;
    QString authors; // only one string with combined authors
    QString description;
    qint64 subscribed;
    qint64 lastUpdated;
    bool isNew;
    QString dirname;
    QString lastHash;
    int filterType = 0;
    int sortType = 0;
};

// This combines EntryDetails with the info on the related Feed
struct EntryFeedDetails : EntryDetails {
    FeedDetails feed;
};

// The following structs are only used in the feed update routine.
// They are mainly the same but contain extra fields needed to diff.
struct AuthorUpdateDetails {
    QString name;
    QString email;
    RecordState state;

    // Fields that are only used in case state == Modified
    QString oldEmail;
};

struct EnclosureUpdateDetails {
    qint64 enclosureuid;
    qint64 duration;
    qint64 size;
    qint64 downloadSize;
    QString type;
    QString url;
    DataTypes::EnclosureStatus downloaded;
    RecordState state;

    // Fields that are only used in case state == Modified
    qint64 oldDuration;
    qint64 oldSize;
    QString oldType;
    QString oldUrl;
};

struct ChapterUpdateDetails {
    qint64 start;
    QString title;
    QString link;
    QString image;
    RecordState state;

    // Fields that are only used in case state == Modified
    QString oldTitle;
    QString oldLink;
    QString oldImage;
};

struct EntryUpdateDetails {
    qint64 entryuid;
    qint64 feeduid;
    QString id;
    QString title;
    QString content;
    qint64 created;
    qint64 updated;
    QString link;
    bool read;
    bool isNew;
    bool favorite;
    qint64 playPosition;
    bool removed;
    bool hasEnclosure;
    QString image;
    RecordState state;
    QHash<QString, AuthorUpdateDetails> authors; // key = author name
    QHash<QString, EnclosureUpdateDetails> enclosures; // key = enclosure url
    QHash<qint64, ChapterUpdateDetails> chapters; // key = start

    // these lists can store a particular order of authors and enclosures when
    // needed
    QList<QString> authorOrder;
    QList<QString> enclosureOrder;

    // Fields that are only used in case state == Modified
    QString oldTitle;
    QString oldContent;
    qint64 oldCreated;
    qint64 oldUpdated;
    QString oldLink;
    bool oldRemoved;
    bool oldHasEnclosure; // TODO: probably don't need this since there is the enclosure QHash anyway
    QString oldImage;
};

struct FeedUpdateDetails {
    qint64 feeduid;
    QString name;
    QString url;
    QString image;
    QString link;
    QString description;
    qint64 subscribed;
    qint64 lastUpdated;
    bool isNew;
    QString dirname;
    QString lastHash;
    int filterType = 0;
    int sortType = 0;
    RecordState state;
    QHash<QString, AuthorUpdateDetails> authors; // key = author name
    QHash<QString, EntryUpdateDetails> entries; // key = id from feed

    // this list can store a particular order of entries when needed
    QList<QString> entryOrder;

    // Fields that are only used in case state == Modified
    QString oldName;
    QString oldUrl;
    QString oldImage;
    QString oldLink;
    QString oldDescription;
    qint64 oldLastUpdated;
    QString oldDirname;
    QString oldLastHash;
};
}
