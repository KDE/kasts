/**
 * Copyright 2020 Tobias Fella <fella@posteo.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <QVariant>

#include "feed.h"

Feed::Feed()
    : m_url(QStringLiteral(""))
    , m_name(QStringLiteral(""))
    , m_image(QStringLiteral(""))
{
}
Feed::Feed(const QString url)
    : m_url(url)
    , m_name(url)
    , m_image(QStringLiteral(""))
{
}

Feed::Feed(const QString url, const QString name, const QString image)
    : m_url(url)
    , m_name(name)
    , m_image(image)
{
}

Feed::Feed(const Feed &other)
    : m_url(other.url())
    , m_name(other.name())
    , m_image(other.image())
{
}

Feed::~Feed()
{
}

QString Feed::url() const
{
    return m_url;
}

QString Feed::name() const
{
    return m_name;
}

QString Feed::image() const
{
    return m_image;
}

void Feed::setName(QString name)
{
    m_name = name;
}

void Feed::setImage(QString image)
{
    m_image = image;
}
