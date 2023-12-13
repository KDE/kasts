/**
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "metadata.h"
#include "metadatalogging.h"

#include <QTimer>

MetaData::MetaData(QObject *parent)
    : QObject(parent)
{
    qCDebug(MetaDataLog) << "MetaData::MetaData begin";
    connect(this, &MetaData::titleChanged, this, &MetaData::signalMetaDataChanged);
    connect(this, &MetaData::artistChanged, this, &MetaData::signalMetaDataChanged);
    connect(this, &MetaData::albumChanged, this, &MetaData::signalMetaDataChanged);
    connect(this, &MetaData::artworkUrlChanged, this, &MetaData::signalMetaDataChanged);
}

QString MetaData::title() const
{
    qCDebug(MetaDataLog) << "MetaData::title()";
    return m_title;
}

QString MetaData::artist() const
{
    qCDebug(MetaDataLog) << "MetaData::artist()";
    return m_artist;
}

QString MetaData::album() const
{
    qCDebug(MetaDataLog) << "MetaData::album()";
    return m_album;
}

QUrl MetaData::artworkUrl() const
{
    qCDebug(MetaDataLog) << "MetaData::artworkUrl()";
    return m_artworkUrl;
}

void MetaData::setTitle(const QString &title)
{
    qCDebug(MetaDataLog) << "MetaData::setTitle(" << title << ")";
    if (title != m_title) {
        m_title = title;
        Q_EMIT titleChanged(title);
    }
}

void MetaData::setArtist(const QString &artist)
{
    qCDebug(MetaDataLog) << "MetaData::setArtist(" << artist << ")";
    if (artist != m_artist) {
        m_artist = artist;
        Q_EMIT artistChanged(artist);
    }
}

void MetaData::setAlbum(const QString &album)
{
    qCDebug(MetaDataLog) << "MetaData::setAlbum(" << album << ")";
    if (album != m_album) {
        m_album = album;
        Q_EMIT albumChanged(album);
    }
}

void MetaData::setArtworkUrl(const QUrl &artworkUrl)
{
    qCDebug(MetaDataLog) << "MetaData::setArtworkUrl(" << artworkUrl << ")";
    if (artworkUrl != m_artworkUrl) {
        m_artworkUrl = artworkUrl;
        Q_EMIT artworkUrlChanged(artworkUrl);
    }
}

void MetaData::clear()
{
    qCDebug(MetaDataLog) << "MetaData::clear()";
    m_title.clear();
    m_artist.clear();
    m_album.clear();
    m_artworkUrl.clear();
    Q_EMIT titleChanged(m_title);
    Q_EMIT artistChanged(m_artist);
    Q_EMIT albumChanged(m_album);
    Q_EMIT artworkUrlChanged(m_artworkUrl);
}

void MetaData::signalMetaDataChanged()
{
    QTimer::singleShot(0, this, [this]() {
        Q_EMIT metaDataChanged(this);
    });
}
