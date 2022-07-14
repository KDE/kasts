/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "database.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStandardPaths>
#include <QUrl>

#define TRUE_OR_RETURN(x)                                                                                                                                      \
    if (!x)                                                                                                                                                    \
        return false;

Database::Database()
{
    Database::openDatabase();

    if (!migrate()) {
        qCritical() << "Failed to migrate the database";
    }

    cleanup();
}

void Database::openDatabase(const QString &connectionName)
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    QString databasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(databasePath).mkpath(databasePath);
    db.setDatabaseName(databasePath + QStringLiteral("/") + m_dbName);
    db.open();
}

void Database::closeDatabase(const QString &connectionName)
{
    QSqlDatabase::database(connectionName).close();
    QSqlDatabase::removeDatabase(connectionName);
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
    if (dbversion < 4)
        TRUE_OR_RETURN(migrateTo4());
    if (dbversion < 5)
        TRUE_OR_RETURN(migrateTo5());
    if (dbversion < 6)
        TRUE_OR_RETURN(migrateTo6());
    return true;
}

bool Database::migrateTo1()
{
    qDebug() << "Migrating database to version 1";
    TRUE_OR_RETURN(transaction());
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
    TRUE_OR_RETURN(commit());
    return true;
}

bool Database::migrateTo2()
{
    qDebug() << "Migrating database to version 2";
    TRUE_OR_RETURN(transaction());
    TRUE_OR_RETURN(execute(QStringLiteral("DROP TABLE Errors;")));
    TRUE_OR_RETURN(execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Errors (type INTEGER, url TEXT, id TEXT, code INTEGER, message TEXT, date INTEGER);")));
    TRUE_OR_RETURN(execute(QStringLiteral("PRAGMA user_version = 2;")));
    TRUE_OR_RETURN(commit());
    return true;
}

bool Database::migrateTo3()
{
    qDebug() << "Migrating database to version 3";
    TRUE_OR_RETURN(transaction());
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
    TRUE_OR_RETURN(commit());
    return true;
}

bool Database::migrateTo4()
{
    qDebug() << "Migrating database to version 4";
    TRUE_OR_RETURN(transaction());
    TRUE_OR_RETURN(execute(QStringLiteral("DROP TABLE Errors;")));
    TRUE_OR_RETURN(
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Errors (type INTEGER, url TEXT, id TEXT, code INTEGER, message TEXT, date INTEGER, title TEXT);")));
    TRUE_OR_RETURN(execute(QStringLiteral("PRAGMA user_version = 4;")));
    TRUE_OR_RETURN(commit());
    return true;
}

bool Database::migrateTo5()
{
    qDebug() << "Migrating database to version 5";
    TRUE_OR_RETURN(transaction());
    TRUE_OR_RETURN(execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Chapters (feed TEXT, id TEXT, start INTEGER, title TEXT, link TEXT, image TEXT);")));
    TRUE_OR_RETURN(execute(QStringLiteral("PRAGMA user_version = 5;")));
    TRUE_OR_RETURN(commit());
    return true;
}

bool Database::migrateTo6()
{
    qDebug() << "Migrating database to version 6";
    TRUE_OR_RETURN(transaction());
    TRUE_OR_RETURN(execute(QStringLiteral("CREATE TABLE IF NOT EXISTS SyncTimestamps (syncservice TEXT, timestamp INTEGER);")));
    TRUE_OR_RETURN(execute(QStringLiteral("CREATE TABLE IF NOT EXISTS FeedActions (url TEXT, action TEXT, timestamp INTEGER);")));
    TRUE_OR_RETURN(
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS EpisodeActions (podcast TEXT, url TEXT, id TEXT, action TEXT, started INTEGER, position INTEGER, "
                               "total INTEGER, timestamp INTEGER);")));
    TRUE_OR_RETURN(execute(QStringLiteral("PRAGMA user_version = 6;")));
    TRUE_OR_RETURN(commit());
    return true;
}

bool Database::execute(const QString &query, const QString &connectionName)
{
    QSqlQuery q(connectionName);
    q.prepare(query);
    return execute(q);
}

bool Database::execute(QSqlQuery &query)
{
    // NOTE that this will execute the query on the database that was specified
    // when the QSqlQuery was created.  There is no way to change that later on.
    if (!query.exec()) {
        qWarning() << "Failed to execute SQL Query";
        qWarning() << query.lastQuery();
        qWarning() << query.lastError();
        return false;
    }
    return true;
}

bool Database::transaction(const QString &connectionName)
{
    // use IMMEDIATE transaction here to avoid deadlocks with writes happening
    // in different threads
    QSqlQuery query(QSqlDatabase::database(connectionName));
    query.prepare(QStringLiteral("BEGIN IMMEDIATE TRANSACTION;"));
    return execute(query);
}

bool Database::commit(const QString &connectionName)
{
    return QSqlDatabase::database(connectionName).commit();
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
