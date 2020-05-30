/*
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

#include "database.h"
#include "feed.h"

Feed::Feed(QString url, QString name, QString image, QString link, QString description, QVector<Author *> authors, QObject *parent)
    : QObject(parent)
    , m_url(url)
    , m_name(name)
    , m_image(image)
    , m_link(link)
    , m_description(description)
    , m_authors(authors)
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

QString Feed::link() const
{
    return m_link;
}

QString Feed::description() const
{
    return m_description;
}

QVector<Author *> Feed::authors() const
{
    return m_authors;
}

void Feed::setName(QString name)
{
    m_name = name;
    emit nameChanged(m_name);
}

void Feed::setImage(QString image)
{
    m_image = image;
    emit imageChanged(m_image);
}

void Feed::setLink(QString link)
{
    m_link = link;
    emit linkChanged(m_link);
}

void Feed::setDescription(QString description)
{
    m_description = description;
    emit descriptionChanged(m_description);
}

void Feed::setAuthors(QVector<Author *> authors)
{
    m_authors = authors;
    emit authorsChanged(m_authors);
}

void Feed::remove()
{
    // Delete Authors
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM Authors WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    Database::instance().execute(query);

    // Delete Entries
    query.prepare(QStringLiteral("DELETE FROM Entries WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    Database::instance().execute(query);

    // TODO Delete Enclosures

    // Delete Feed
    query.prepare(QStringLiteral("DELETE FROM Feeds WHERE url=:url;"));
    query.bindValue(QStringLiteral(":url"), m_url);
    Database::instance().execute(query);
}
