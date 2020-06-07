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

#ifndef ENTRY_H
#define ENTRY_H

#include <QDateTime>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QStringList>

#include "author.h"
#include "feed.h"

class Entry : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QString content READ content CONSTANT)
    Q_PROPERTY(QVector<Author *> authors READ authors CONSTANT)
    Q_PROPERTY(QDateTime created READ created CONSTANT)
    Q_PROPERTY(QDateTime updated READ updated CONSTANT)
    Q_PROPERTY(QString link READ link CONSTANT)
    Q_PROPERTY(QString baseUrl READ baseUrl CONSTANT)
    Q_PROPERTY(bool read READ read WRITE setRead NOTIFY readChanged);

public:
    Entry(Feed *feed, int index);
    ~Entry();

    QString id() const;
    QString title() const;
    QString content() const;
    QVector<Author *> authors() const;
    QDateTime created() const;
    QDateTime updated() const;
    QString link() const;
    bool read() const;

    QString baseUrl() const;

    void setRead(bool read);

    Q_INVOKABLE QString adjustedContent(int width, int fontSize);

Q_SIGNALS:
    void readChanged(bool read);

private:
    Feed *m_feed;
    QString m_id;
    QString m_title;
    QString m_content;
    QVector<Author *> m_authors;
    QDateTime m_created;
    QDateTime m_updated;
    QString m_link;
    bool m_read;
};

#endif // ENTRY_H
