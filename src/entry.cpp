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

#include <QSqlQuery>
#include <QUrl>

#include "database.h"

Entry::Entry(Feed *feed, int index)
    : QObject(nullptr)
    , m_feed(feed)
{
    QSqlQuery entryQuery;
    entryQuery.prepare(QStringLiteral("SELECT * FROM Entries WHERE feed=:feed ORDER BY updated DESC LIMIT 1 OFFSET :index;"));
    entryQuery.bindValue(QStringLiteral(":feed"), m_feed->url());
    entryQuery.bindValue(QStringLiteral(":index"), index);
    Database::instance().execute(entryQuery);
    if (!entryQuery.next())
        qWarning() << "No element with index" << index << "found in feed" << m_feed->url();

    QSqlQuery authorQuery;
    authorQuery.prepare(QStringLiteral("SELECT * FROM Authors WHERE id=:id"));
    authorQuery.bindValue(QStringLiteral(":id"), entryQuery.value(QStringLiteral("id")).toString());
    Database::instance().execute(authorQuery);

    while (authorQuery.next()) {
        m_authors += new Author(authorQuery.value(QStringLiteral("name")).toString(), authorQuery.value(QStringLiteral("email")).toString(), authorQuery.value(QStringLiteral("uri")).toString(), nullptr);
    }

    m_created.setSecsSinceEpoch(entryQuery.value(QStringLiteral("created")).toInt());
    m_updated.setSecsSinceEpoch(entryQuery.value(QStringLiteral("updated")).toInt());

    m_id = entryQuery.value(QStringLiteral("id")).toString();
    m_title = entryQuery.value(QStringLiteral("title")).toString();
    m_content = entryQuery.value(QStringLiteral("content")).toString();
    m_link = entryQuery.value(QStringLiteral("link")).toString();
    m_read = entryQuery.value(QStringLiteral("read")).toBool();
}

Entry::~Entry()
{
}

QString Entry::id() const
{
    return m_id;
}

QString Entry::title() const
{
    return m_title;
}

QString Entry::content() const
{
    return m_content;
}

QVector<Author *> Entry::authors() const
{
    return m_authors;
}

QDateTime Entry::created() const
{
    return m_created;
}

QDateTime Entry::updated() const
{
    return m_updated;
}

QString Entry::link() const
{
    return m_link;
}

bool Entry::read() const
{
    return m_read;
}

QString Entry::baseUrl() const
{
    return QUrl(m_link).adjusted(QUrl::RemovePath).toString();
}

void Entry::setRead(bool read)
{
    m_read = read;
    Q_EMIT readChanged(m_read);
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE Entries SET read=:read WHERE id=:id AND feed=:feed"));
    query.bindValue(QStringLiteral(":id"), m_id);
    query.bindValue(QStringLiteral(":feed"), m_feed->url());
    query.bindValue(QStringLiteral(":read"), m_read);
    Database::instance().execute(query);
}
