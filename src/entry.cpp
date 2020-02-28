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

#include "entry.h"

Entry::Entry(const QString title, const bool bookmark, const bool read)
    : m_title(title)
    , m_bookmark(bookmark)
    , m_read(read)
{
}

Entry::Entry(const Entry &other)
    : m_title(other.title())
    , m_bookmark(other.isBookmark())
    , m_read(other.isRead())
{
}

bool Entry::isRead() const
{
    return m_read;
}

bool Entry::isBookmark() const
{
    return m_bookmark;
}

QString Entry::title() const
{
    return m_title;
}

void Entry::setRead(bool read)
{
    m_read = read;
}

void Entry::setBookmark(bool bookmark)
{
    m_bookmark = bookmark;
}
