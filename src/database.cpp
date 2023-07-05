/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "database.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStandardPaths>
#include <QUrl>

#include "settingsmanager.h"

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
    setWalMode();

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
    if (dbversion < 7)
        TRUE_OR_RETURN(migrateTo7());
    if (dbversion < 8)
        TRUE_OR_RETURN(migrateTo8());
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

bool Database::migrateTo7()
{
    qDebug() << "Migrating database to version 7";
    TRUE_OR_RETURN(transaction());
    TRUE_OR_RETURN(execute(QStringLiteral("ALTER TABLE Entries ADD COLUMN favorite BOOL DEFAULT 0;")));
    TRUE_OR_RETURN(execute(QStringLiteral("PRAGMA user_version = 7;")));
    TRUE_OR_RETURN(commit());
    return true;
}

bool Database::migrateTo8()
{
    qDebug() << "Migrating database to version 8; this can take a while";

    const int maxFilenameLength = 200;
    QString enclosurePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!SettingsManager::self()->storagePath().isEmpty()) {
        enclosurePath = SettingsManager::self()->storagePath().toLocalFile();
    }
    enclosurePath += QStringLiteral("/enclosures/");

    TRUE_OR_RETURN(transaction());
    TRUE_OR_RETURN(execute(QStringLiteral("ALTER TABLE Feeds ADD COLUMN dirname TEXT;")));

    QStringList dirNameList;
    QSqlQuery query(QStringLiteral("SELECT url, name FROM Feeds;"));
    while (query.next()) {
        QString url = query.value(QStringLiteral("url")).toString();
        QString name = query.value(QStringLiteral("name")).toString();

        // Generate directory name for enclosures based on feed name
        QString dirBaseName = name.left(maxFilenameLength);
        QString dirName = dirBaseName;

        // Check for duplicate names
        int numDups = 1; // Minimum to append is " (1)" if file already exists
        while (dirNameList.contains(dirName)) {
            dirName = QStringLiteral("%1 (%2)").arg(dirBaseName, QString::number(numDups));
            numDups++;
        }

        dirNameList << dirName;

        QSqlQuery writeQuery;
        writeQuery.prepare(QStringLiteral("UPDATE Feeds SET dirname=:dirname WHERE url=:url;"));
        writeQuery.bindValue(QStringLiteral(":dirname"), dirName);
        writeQuery.bindValue(QStringLiteral(":url"), url);
        TRUE_OR_RETURN(execute(writeQuery));
    }

    // Rename enclosures to new filename convention
    query.prepare(
        QStringLiteral("SELECT entry.title, enclosure.id, enclosure.url, feed.dirname FROM Enclosures enclosure JOIN Entries entry ON enclosure.id = entry.id "
                       "JOIN Feeds feed ON enclosure.feed = feed.url;"));
    TRUE_OR_RETURN(execute(query));
    while (query.next()) {
        QString queryTitle = query.value(QStringLiteral("title")).toString();
        QString queryId = query.value(QStringLiteral("id")).toString();
        QString queryUrl = query.value(QStringLiteral("url")).toString();
        QString feedDirName = query.value(QStringLiteral("dirname")).toString();

        // Rename any existing files with the new filename generated above
        QString legacyPath = enclosurePath + QString::fromStdString(QCryptographicHash::hash(queryUrl.toUtf8(), QCryptographicHash::Md5).toHex().toStdString());

        if (QFileInfo::exists(legacyPath) && QFileInfo(legacyPath).isFile()) {
            // Generate filename based on episode name and url hash with feedname as subdirectory
            QString enclosureFilenameBase = queryTitle.left(maxFilenameLength) + QStringLiteral(".")
                + QString::fromStdString(QCryptographicHash::hash(queryUrl.toUtf8(), QCryptographicHash::Md5).toHex().toStdString()).left(6);
            QString enclosureFilenameExt = QFileInfo(QUrl::fromUserInput(queryUrl).fileName()).suffix();

            QString enclosureFilename =
                !enclosureFilenameExt.isEmpty() ? enclosureFilenameBase + QStringLiteral(".") + enclosureFilenameExt : enclosureFilenameBase;

            QString newDirPath = enclosurePath + feedDirName + QStringLiteral("/");
            QString newFilePath = newDirPath + enclosureFilename;

            QFileInfo().absoluteDir().mkpath(newDirPath);
            QFile::rename(legacyPath, newFilePath);
        }
    }

    TRUE_OR_RETURN(execute(QStringLiteral("PRAGMA user_version = 8;")));
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

void Database::setWalMode()
{
    bool ok = false;
    QSqlQuery query;
    query.prepare(QStringLiteral("PRAGMA journal_mode;"));
    execute(query);
    if (query.next()) {
        ok = (query.value(0).toString() == QStringLiteral("wal"));
    }

    if (!ok) {
        query.prepare(QStringLiteral("PRAGMA journal_mode=WAL;"));
        execute(query);
        if (query.next()) {
            ok = (query.value(0).toString() == QStringLiteral("wal"));
        }
        qDebug() << "Activating WAL mode on database:" << (ok ? "ok" : "not ok!");
    }
}

void Database::cleanup()
{
    // TODO: create database sanity checks, or, alternatively, create database scrub routine
}
