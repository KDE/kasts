/**
 * SPDX-FileCopyrightText: 2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "chapter.h"

#include "fetcher.h"

Chapter::Chapter(Entry *entry, const QString &title, const QString &link, const QString &image, const int &start, QObject *parent)
    : QObject(parent)
    , m_entry(entry)
    , m_title(title)
    , m_link(link)
    , m_image(image)
    , m_start(start)
{
    connect(&Fetcher::instance(), &Fetcher::downloadFinished, this, [this](QString url) {
        if (url == m_image) {
            Q_EMIT imageChanged(url);
            Q_EMIT cachedImageChanged(cachedImage());
        }
    });
}

Chapter::~Chapter()
{
}

Entry *Chapter::entry() const
{
    return m_entry;
}

QString Chapter::title() const
{
    return m_title;
}

QString Chapter::link() const
{
    return m_link;
}

QString Chapter::image() const
{
    if (!m_image.isEmpty()) {
        return m_image;
    } else if (m_entry) {
        // fall back to entry image
        return m_entry->image();
    } else {
        return QStringLiteral("no-image");
    }
}

QString Chapter::cachedImage() const
{
    // First check for the feed image, fall back if needed
    QString image = m_image;
    if (image.isEmpty()) {
        if (m_entry) {
            return m_entry->cachedImage();
        } else {
            return QStringLiteral("no-image");
        }
    }

    return Fetcher::instance().image(image);
}

int Chapter::start() const
{
    return m_start;
}

void Chapter::setTitle(const QString &title, bool emitSignal)
{
    if (m_title != title) {
        m_title = title;
        if (emitSignal) {
            Q_EMIT titleChanged(m_title);
        }
    }
}

void Chapter::setLink(const QString &link, bool emitSignal)
{
    if (m_link != link) {
        m_link = link;
        if (emitSignal) {
            Q_EMIT linkChanged(m_link);
        }
    }
}

void Chapter::setImage(const QString &image, bool emitSignal)
{
    if (m_image != image) {
        m_image = image;
        if (emitSignal) {
            Q_EMIT imageChanged(m_image);
            Q_EMIT cachedImageChanged(cachedImage());
        }
    }
}

void Chapter::setStart(const int &start, bool emitSignal)
{
    if (m_start != start) {
        m_start = start;
        if (emitSignal) {
            Q_EMIT startChanged(m_start);
        }
    }
}
