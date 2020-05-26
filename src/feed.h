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

#ifndef FEED_H
#define FEED_H

#include <QObject>

class Feed : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString url READ url CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString image READ image WRITE setImage NOTIFY imageChanged)

public:
    Feed(QString url, QString name, QString image, QObject *parent = nullptr);

    ~Feed();

    QString url() const;
    QString name() const;
    QString image() const;

    void setName(QString name);
    void setImage(QString image);

    void remove();

Q_SIGNALS:
    void nameChanged(QString &name);
    void imageChanged(QString &image);

private:
    QString m_url;
    QString m_name;
    QString m_image;
};

#endif // FEED_H
