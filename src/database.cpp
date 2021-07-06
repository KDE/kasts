/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QDateTime>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStandardPaths>
#include <QUrl>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "database.h"
#include "fetcher.h"
#include "settingsmanager.h"

#define TRUE_OR_RETURN(x)                                                                                                                                      \
    if (!x)                                                                                                                                                    \
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
    int dbversion = version();
    if (dbversion < 1)
        TRUE_OR_RETURN(migrateTo1());
    if (dbversion < 2)
        TRUE_OR_RETURN(migrateTo2());
    if (dbversion < 3)
        TRUE_OR_RETURN(migrateTo3());
    return true;
}

bool Database::migrateTo1()
{
    qDebug() << "Migrating database to version 1";
    TRUE_OR_RETURN(
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Feeds (name TEXT, url TEXT, image TEXT, link TEXT, description TEXT, deleteAfterCount INTEGER, "
                               "deleteAfterType INTEGER, subscribed INTEGER, lastUpdated INTEGER, new BOOL, notify BOOL);")));
    TRUE_OR_RETURN(
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Entries (feed TEXT, id TEXT UNIQUE, title TEXT, content TEXT, created INTEGER, updated INTEGER, "
                               "link TEXT, read bool, new bool, hasEnclosure BOOL, image TEXT);")));
    TRUE_OR_RETURN(execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Authors (feed TEXT, id TEXT, name TEXT, uri TEXT, email TEXT);")));
    TRUE_OR_RETURN(
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Enclosures (feed TEXT, id TEXT, duration INTEGER, size INTEGER, title TEXT, type TEXT, url TEXT, "
                               "playposition INTEGER, downloaded BOOL);")));
    TRUE_OR_RETURN(execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Queue (listnr INTEGER, feed TEXT, id TEXT, playing BOOL);")));
    TRUE_OR_RETURN(execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Errors (url TEXT, id TEXT, code INTEGER, message TEXT, date INTEGER);")));
    TRUE_OR_RETURN(execute(QStringLiteral("PRAGMA user_version = 1;")));
    return true;
}

bool Database::migrateTo2()
{
    qDebug() << "Migrating database to version 2";
    TRUE_OR_RETURN(execute(QStringLiteral("BEGIN TRANSACTION;")));
    TRUE_OR_RETURN(execute(QStringLiteral("DROP TABLE Errors;")));
    TRUE_OR_RETURN(execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Errors (type INTEGER, url TEXT, id TEXT, code INTEGER, message TEXT, date INTEGER);")));
    TRUE_OR_RETURN(execute(QStringLiteral("PRAGMA user_version = 2;")));
    TRUE_OR_RETURN(execute(QStringLiteral("COMMIT;")));
    return true;
}

bool Database::migrateTo3()
{
    qDebug() << "Migrating database to version 3";
    TRUE_OR_RETURN(execute(QStringLiteral("BEGIN TRANSACTION;")));
    TRUE_OR_RETURN(
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Enclosurestemp (feed TEXT, id TEXT, duration INTEGER, size INTEGER, title TEXT, type TEXT, url "
                               "TEXT, playposition INTEGER, downloaded INTEGER);")));
    TRUE_OR_RETURN(
        execute(QStringLiteral("INSERT INTO Enclosurestemp (feed, id, duration, size, title, type, url, playposition, downloaded) SELECT feed, id, duration, "
                               "size, title, type, url, playposition, downloaded FROM Enclosures;")));
    TRUE_OR_RETURN(execute(QStringLiteral("UPDATE Enclosurestemp SET downloaded=3 WHERE downloaded=1;")));
    TRUE_OR_RETURN(execute(QStringLiteral("DROP TABLE Enclosures;")));
    TRUE_OR_RETURN(execute(QStringLiteral("ALTER TABLE Enclosurestemp RENAME TO Enclosures;")));
    TRUE_OR_RETURN(execute(QStringLiteral("PRAGMA user_version = 3;")));
    TRUE_OR_RETURN(execute(QStringLiteral("COMMIT;")));
    return true;
}

bool Database::execute(const QString &query)
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
        qDebug() << "Database version" << value;
        if (ok)
            return value;
    } else {
        qCritical() << "Failed to check database version";
    }
    return -1;
}

void Database::cleanup()
{
    // TODO: create database sanity checks, or, alternatively, create database scrub routine
}
