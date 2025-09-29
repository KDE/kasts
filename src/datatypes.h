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
    int subscribed;
    int lastUpdated;
    bool isNew;
    QString dirname;
    QString lastHash;
    int filterType = 0;
    int sortType = 0;
};

struct EntryDetails {
    int entryid = 0;
    int feedid;
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

struct FeedAuthorDetails {
    int feedid;
    QString name;
    QString email;
};

struct EntryAuthorDetails {
    int entryid;
    QString name;
    QString email;
};

struct EnclosureDetails {
    int entryid;
    QString url;
    int duration;
    int size;
    QString type;
    int playPosition;
    Enclosure::Status downloaded;
};

struct ChapterDetails {
    int entryid;
    int start;
    QString title;
    QString link;
    QString image;
};
}
