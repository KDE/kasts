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
    RemovedFromFeed,
};
Q_ENUM_NS(RecordState)

// structs
struct AuthorDetails {
    QString name;
    QString email;
    RecordState state;
};

struct EnclosureDetails {
    int duration;
    int size;
    QString title;
    QString type;
    QString url;
    int playPosition;
    Enclosure::Status downloaded;
    RecordState state;
};

struct ChapterDetails {
    int start;
    QString title;
    QString link;
    QString image;
    RecordState state;
};

struct EntryDetails {
    QString feed;
    QString id;
    QString title;
    QString content;
    int created;
    int updated;
    QString link;
    bool read;
    bool isNew;
    bool hasEnclosure;
    QString image;
    QHash<QString, AuthorDetails> authors; // key = author name
    QHash<QString, EnclosureDetails> enclosures; // key = enclosure url
    QHash<int, ChapterDetails> chapters; // key = start
    RecordState state;
};

struct FeedDetails {
    QString name;
    QString url;
    QString image;
    QString link;
    QString description;
    int deleteAfterCount = 0;
    int deleteAfterType = 0;
    int subscribed;
    int lastUpdated;
    bool isNew;
    bool notify;
    QString dirname;
    QString lastHash;
    int filterType = 0;
    int sortType = 0;
    QHash<QString, AuthorDetails> authors; // key = author name
    QHash<QString, EntryDetails> entries; // key = id from feed
    RecordState state;
};
}
