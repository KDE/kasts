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

// structs
struct FeedDetails {
    int feedid;
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
};

struct EntryDetails {
    int feed;
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
};

struct AuthorDetails {
    int feed;
    QString id;
    QString name;
    QString uri;
    QString email;
};

struct EnclosureDetails {
    int feed;
    QString id;
    int duration;
    int size;
    QString title;
    QString type;
    QString url;
    int playPosition;
    Enclosure::Status downloaded;
};

struct ChapterDetails {
    int feed;
    QString id;
    int start;
    QString title;
    QString link;
    QString image;
};
}
