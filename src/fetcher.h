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

#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>
#include <Syndication/Syndication>

class Fetcher : public QObject
{
    Q_OBJECT
public:
    static Fetcher &instance()
    {
        static Fetcher _instance;
        return _instance;
    }
    Q_INVOKABLE void fetch(QString url);
    Q_INVOKABLE QString image(QString);
    void removeImage(QString);
    Q_INVOKABLE void download(QString url);

private:
    Fetcher();

    QString filePath(QString);
    void processFeed(Syndication::FeedPtr feed, QString url);
    void processEntry(Syndication::ItemPtr entry, QString url);
    void processAuthor(Syndication::PersonPtr author, QString entryId, QString url);
    void processEnclosure(Syndication::EnclosurePtr enclosure, Syndication::ItemPtr entry, QString feedUrl);

    QNetworkAccessManager *manager;

Q_SIGNALS:
    void feedUpdated(QString url);
    void feedDetailsUpdated(QString url, QString name, QString image, QString link, QString description);
};
