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

#include <QDateTime>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStandardPaths>

#include "alligatorsettings.h"
#include "database.h"
#include "fetcher.h"

#define TRUE_OR_RETURN(x)                                                                                                                                                                                                                      \
    if (!x)                                                                                                                                                                                                                                    \
        return false;

Database::Database()
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    QString databasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(databasePath).mkpath(databasePath);
    db.setDatabaseName(databasePath + QStringLiteral("/database.db3"));
    db.open();

    if (!migrate()) {
        qCritical() << "Failed to migrate the database";
    }

    cleanup();
}

bool Database::migrate()
{
    if (version() < 1)
        TRUE_OR_RETURN(migrateTo1());
    return true;
}

bool Database::migrateTo1()
{
    qDebug() << "Migrating database to version 1";
    TRUE_OR_RETURN(execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Feeds (name TEXT, url TEXT, image TEXT, link TEXT, description TEXT);")));
    TRUE_OR_RETURN(execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Entries (feed TEXT, id TEXT UNIQUE, title TEXT, content TEXT, created INTEGER, updated INTEGER, link TEXT);")));
    TRUE_OR_RETURN(execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Authors (feed TEXT, id TEXT, name TEXT, uri TEXT, email TEXT);")));
    TRUE_OR_RETURN(execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Enclosures (feed TEXT, id TEXT, duration INTEGER, size INTEGER, title TEXT, type STRING, url STRING);")));
    TRUE_OR_RETURN(execute(QStringLiteral("PRAGMA user_version = 1;")));
    return true;
}

bool Database::execute(QString query)
{
    QSqlQuery q;
    q.prepare(query);
    return execute(q);
}

bool Database::execute(QSqlQuery &query)
{
    if (!query.exec()) {
        qWarning() << "Failed to execute SQL Query";
        qWarning() << query.lastQuery();
        qWarning() << query.lastError();
        return false;
    }
    return true;
}

int Database::version()
{
    QSqlQuery query;
    query.prepare(QStringLiteral("PRAGMA user_version;"));
    execute(query);
    if (query.next()) {
        bool ok;
        int value = query.value(0).toInt(&ok);
        qDebug() << "Database version " << value;
        if (ok)
            return value;
    } else {
        qCritical() << "Failed to check database version";
    }
    return -1;
}

void Database::cleanup()
{
    AlligatorSettings settings;
    int count = settings.deleteAfterCount();
    int type = settings.deleteAfterType();

    if(type == 0) { //Never delete Entries
        return;
    }

    if (type == 1) { // Delete after <count> posts per feed
                     // TODO
    } else {
        QDateTime dateTime = QDateTime::currentDateTime();
        if (type == 2)
            dateTime = dateTime.addDays(-count);
        else if (type == 3)
            dateTime = dateTime.addDays(-7 * count);
        else if (type == 4)
            dateTime = dateTime.addMonths(-count);
        qint64 sinceEpoch = dateTime.toSecsSinceEpoch();

        QSqlQuery query;
        query.prepare(QStringLiteral("DELETE FROM Entries WHERE updated < :sinceEpoch;"));
        query.bindValue(QStringLiteral(":sinceEpoch"), sinceEpoch);
        execute(query);
    }
}

bool Database::feedExists(QString url)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (url) FROM Feeds WHERE url=:url;"));
    query.bindValue(QStringLiteral(":url"), url);
    Database::instance().execute(query);
    query.next();
    return query.value(0).toInt() != 0;
}

void Database::addFeed(QString url)
{
    qDebug() << "Adding feed";
    if (feedExists(url)) {
        qDebug() << "Feed already exists";
        return;
    }
    qDebug() << "Feed does not yet exist";

    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO Feeds VALUES (:name, :url, :image, :link, :description);"));
    query.bindValue(QStringLiteral(":name"), url);
    query.bindValue(QStringLiteral(":url"), url);
    query.bindValue(QStringLiteral(":image"), QLatin1String(""));
    query.bindValue(QStringLiteral(":link"), QLatin1String(""));
    query.bindValue(QStringLiteral(":description"), QLatin1String(""));
    execute(query);

    Q_EMIT feedAdded(url);

    Fetcher::instance().fetch(url);
}
