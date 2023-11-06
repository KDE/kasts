/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <KLocalizedString>
#include <QDateTime>
#include <QObject>
#include <QString>

#include "datamanager.h"
#include "entry.h"
#include "error.h"
#include "feed.h"

Error::Error(const Type type, const QString url, const QString id, const int code, const QString message, const QDateTime date, const QString title)
    : QObject(nullptr)
{
    this->type = type;
    this->url = url;
    this->id = id;
    this->code = code;
    this->message = message;
    this->date = date;
    this->m_title = title;
}

QString Error::title() const
{
    QString title = m_title;
    if (title.isEmpty()) {
        if (!id.isEmpty()) {
            if (DataManager::instance().getEntry(id))
                title = DataManager::instance().getEntry(id)->title();
        } else if (!url.isEmpty()) {
            if (DataManager::instance().getFeed(url))
                title = DataManager::instance().getFeed(url)->name();
        }
    }
    return title;
}

QString Error::description() const
{
    switch (type) {
    case Error::Type::FeedUpdate:
        return i18n("Podcast update error");
    case Error::Type::MediaDownload:
        return i18n("Media download error");
    case Error::Type::MeteredNotAllowed:
        return i18n("Update not allowed on metered connection");
    case Error::Type::InvalidMedia:
        return i18n("Invalid media file");
    case Error::Type::DiscoverError:
        return i18n("Nothing found");
    case Error::Type::StorageMoveError:
        return i18n("Error moving storage path");
    case Error::Type::SyncError:
        return i18n("Error syncing feed and/or episode status");
    case Error::Type::MeteredStreamingNotAllowed:
        return i18n("Streaming not allowed on metered connection");
    case Error::Type::NoNetwork:
        return i18n("No network connection");
    default:
        return QString();
    }
}

int Error::typeToDb(Error::Type type)
{
    switch (type) {
    case Error::Type::FeedUpdate:
        return 0;
    case Error::Type::MediaDownload:
        return 1;
    case Error::Type::MeteredNotAllowed:
        return 2;
    case Error::Type::InvalidMedia:
        return 3;
    case Error::Type::DiscoverError:
        return 4;
    case Error::Type::StorageMoveError:
        return 5;
    case Error::Type::SyncError:
        return 6;
    case Error::Type::MeteredStreamingNotAllowed:
        return 7;
    case Error::Type::NoNetwork:
        return 8;
    default:
        return -1;
    }
}

Error::Type Error::dbToType(int value)
{
    switch (value) {
    case 0:
        return Error::Type::FeedUpdate;
    case 1:
        return Error::Type::MediaDownload;
    case 2:
        return Error::Type::MeteredNotAllowed;
    case 3:
        return Error::Type::InvalidMedia;
    case 4:
        return Error::Type::DiscoverError;
    case 5:
        return Error::Type::StorageMoveError;
    case 6:
        return Error::Type::SyncError;
    case 7:
        return Error::Type::MeteredStreamingNotAllowed;
    case 8:
        return Error::Type::NoNetwork;
    default:
        return Error::Type::Unknown;
    }
}
