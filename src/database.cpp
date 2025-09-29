/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "database.h"
#include "databaselogging.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>

#include "error.h"
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
    if (dbversion < 9)
        TRUE_OR_RETURN(migrateTo9());
    if (dbversion < 10)
        TRUE_OR_RETURN(migrateTo10());
    if (dbversion < 11)
        TRUE_OR_RETURN(migrateTo11());
    if (dbversion < 12)
        TRUE_OR_RETURN(migrateTo12());
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
    static QRegularExpression asciiRegexp(QStringLiteral("[^a-zA-Z0-9 ._()-]"));

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
        QString dirBaseName = name.remove(asciiRegexp).simplified().left(maxFilenameLength);
        dirBaseName = dirBaseName.isEmpty() ? QStringLiteral("Noname") : dirBaseName;
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
        QString queryUrl = query.value(QStringLiteral("url")).toString();
        QString feedDirName = query.value(QStringLiteral("dirname")).toString();

        // Rename any existing files with the new filename generated above
        QString legacyPath = enclosurePath + QString::fromStdString(QCryptographicHash::hash(queryUrl.toUtf8(), QCryptographicHash::Md5).toHex().toStdString());

        if (QFileInfo::exists(legacyPath) && QFileInfo(legacyPath).isFile()) {
            // Generate filename based on episode name and url hash with feedname as subdirectory
            QString enclosureFilenameBase = queryTitle.remove(asciiRegexp).simplified().left(maxFilenameLength);
            enclosureFilenameBase = enclosureFilenameBase.isEmpty() ? QStringLiteral("Noname") : enclosureFilenameBase;
            enclosureFilenameBase += QStringLiteral(".")
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

bool Database::migrateTo9()
{
    qDebug() << "Migrating database to version 9";
    TRUE_OR_RETURN(transaction());
    TRUE_OR_RETURN(execute(QStringLiteral("ALTER TABLE Feeds ADD COLUMN lastHash TEXT;")));
    TRUE_OR_RETURN(execute(QStringLiteral("PRAGMA user_version = 9;")));
    TRUE_OR_RETURN(commit());
    return true;
}

bool Database::migrateTo10()
{
    qDebug() << "Migrating database to version 10";
    TRUE_OR_RETURN(transaction());
    TRUE_OR_RETURN(execute(QStringLiteral("ALTER TABLE Feeds ADD COLUMN filterType INTEGER DEFAULT 0;")));
    TRUE_OR_RETURN(execute(QStringLiteral("PRAGMA user_version = 10;")));
    TRUE_OR_RETURN(commit());
    return true;
}

bool Database::migrateTo11()
{
    qDebug() << "Migrating database to version 11";
    TRUE_OR_RETURN(transaction());
    TRUE_OR_RETURN(execute(QStringLiteral("ALTER TABLE Feeds ADD COLUMN sortType INTEGER DEFAULT 0;")));
    TRUE_OR_RETURN(execute(QStringLiteral("PRAGMA user_version = 11;")));
    TRUE_OR_RETURN(commit());
    return true;
}

bool Database::migrateTo12()
{
    qDebug() << "Migrating database to version 12";
    // TODO: re-enable TRUE_OR_RETURN(transaction());

    //  Update Feeds table (need to recreate a new one to drop columns)
    TRUE_OR_RETURN(
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Feedstemp ("
                               "    feedid INTEGER PRIMARY KEY AUTOINCREMENT,"
                               "    name TEXT,"
                               "    url TEXT,"
                               "    image TEXT,"
                               "    link TEXT,"
                               "    description TEXT,"
                               "    subscribed INTEGER,"
                               "    lastUpdated INTEGER,"
                               "    new BOOL,"
                               "    dirname TEXT,"
                               "    lastHash TEXT,"
                               "    filterType INTEGER DEFAULT 0,"
                               "    sortType INTEGER DEFAULT 0"
                               ");")));

    TRUE_OR_RETURN(
        execute(QStringLiteral("INSERT INTO Feedstemp ("
                               "    name,"
                               "    url,"
                               "    image,"
                               "    link,"
                               "    description,"
                               "    subscribed,"
                               "    lastUpdated,"
                               "    new,"
                               "    dirname,"
                               "    lastHash,"
                               "    filterType,"
                               "    sortType) "
                               "SELECT"
                               "    name,"
                               "    url,"
                               "    image,"
                               "    link,"
                               "    description,"
                               "    subscribed,"
                               "    lastUpdated,"
                               "    new,"
                               "    dirname,"
                               "    lastHash,"
                               "    filterType,"
                               "    sortType "
                               "FROM Feeds;")));

    TRUE_OR_RETURN(execute(QStringLiteral("DROP TABLE Feeds;")));
    TRUE_OR_RETURN(execute(QStringLiteral("ALTER TABLE Feedstemp RENAME TO Feeds;")));

    // Update FeedActions table
    TRUE_OR_RETURN(
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS FeedActionstemp ("
                               "    feedid INTEGER,"
                               "    url TEXT,"
                               "    action TEXT,"
                               "    timestamp INTEGER,"
                               "    FOREIGN KEY(feedid) REFERENCES Feeds(feedid));")));

    TRUE_OR_RETURN(
        execute(QStringLiteral("INSERT INTO FeedActionstemp ("
                               "    feedid,"
                               "    url,"
                               "    action,"
                               "    timestamp) "
                               "SELECT"
                               "    Feeds.feedid,"
                               "    FeedActions.url,"
                               "    FeedActions.action,"
                               "    FeedActions.timestamp "
                               "FROM FeedActions"
                               "    JOIN Feeds ON Feeds.url = FeedActions.url;")));

    TRUE_OR_RETURN(execute(QStringLiteral("DROP TABLE FeedActions;")));
    TRUE_OR_RETURN(execute(QStringLiteral("ALTER TABLE FeedActionstemp RENAME TO FeedActions;")));

    // Update Entries table (need to create a new one to remove UNIQUE constraint)
    TRUE_OR_RETURN(
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Entriestemp ("
                               "    entryid INTEGER PRIMARY KEY AUTOINCREMENT,"
                               "    feedid INTEGER,"
                               "    id TEXT,"
                               "    title TEXT,"
                               "    content TEXT,"
                               "    created INTEGER,"
                               "    updated INTEGER,"
                               "    link TEXT,"
                               "    read BOOL,"
                               "    new BOOL,"
                               "    hasEnclosure BOOL,"
                               "    image TEXT,"
                               "    favorite BOOL DEFAULT 0,"
                               "    FOREIGN KEY(feedid) REFERENCES Feeds(feedid));")));

    TRUE_OR_RETURN(
        execute(QStringLiteral("INSERT INTO Entriestemp ("
                               "    feedid,"
                               "    id,"
                               "    title,"
                               "    content,"
                               "    created,"
                               "    updated,"
                               "    link,"
                               "    read,"
                               "    new,"
                               "    hasEnclosure,"
                               "    image,"
                               "    favorite) "
                               "SELECT"
                               "    Feeds.feedid,"
                               "    Entries.id,"
                               "    Entries.title,"
                               "    Entries.content,"
                               "    Entries.created,"
                               "    Entries.updated,"
                               "    Entries.link,"
                               "    Entries.read,"
                               "    Entries.new,"
                               "    Entries.hasEnclosure,"
                               "    Entries.image,"
                               "    Entries.favorite "
                               "FROM Entries"
                               "    JOIN Feeds ON Feeds.url = Entries.feed;")));

    TRUE_OR_RETURN(execute(QStringLiteral("DROP TABLE Entries;")));
    TRUE_OR_RETURN(execute(QStringLiteral("ALTER TABLE Entriestemp RENAME TO Entries;")));

    // Update FeedActions table
    TRUE_OR_RETURN(
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS EpisodeActionstemp ("
                               "    entryid INTEGER,"
                               "    podcast TEXT,"
                               "    url TEXT,"
                               "    id TEXT,"
                               "    action TEXT,"
                               "    started INTEGER,"
                               "    position INTEGER,"
                               "    total INTEGER,"
                               "    timestamp INTEGER,"
                               "    FOREIGN KEY(entryid) REFERENCES Entries(entryid));")));

    TRUE_OR_RETURN(
        execute(QStringLiteral("INSERT INTO EpisodeActionstemp ("
                               "    entryid,"
                               "    podcast,"
                               "    url,"
                               "    id,"
                               "    action,"
                               "    started,"
                               "    position,"
                               "    total,"
                               "    timestamp) "
                               "SELECT"
                               "    Entries.entryid,"
                               "    EpisodeActions.podcast,"
                               "    EpisodeActions.url,"
                               "    EpisodeActions.id,"
                               "    EpisodeActions.action,"
                               "    EpisodeActions.started,"
                               "    EpisodeActions.position,"
                               "    EpisodeActions.total,"
                               "    EpisodeActions.timestamp "
                               "FROM EpisodeActions"
                               "    JOIN Entries ON Entries.id = EpisodeActions.id;")));

    TRUE_OR_RETURN(execute(QStringLiteral("DROP TABLE EpisodeActions;")));
    TRUE_OR_RETURN(execute(QStringLiteral("ALTER TABLE EpisodeActionstemp RENAME TO EpisodeActions;")));

    // Update Enclosures table (need to drop columns)
    TRUE_OR_RETURN(
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Enclosurestemp ("
                               "    enclosureid INTEGER PRIMARY KEY AUTOINCREMENT,"
                               "    entryid INTEGER,"
                               "    url TEXT, "
                               "    duration INTEGER,"
                               "    size INTEGER,"
                               "    type TEXT,"
                               "    playposition INTEGER,"
                               "    downloaded BOOL,"
                               "    FOREIGN KEY(entryid) REFERENCES Entries(entryid);")));

    TRUE_OR_RETURN(
        execute(QStringLiteral("INSERT INTO Enclosurestemp ("
                               "    entryid,"
                               "    url,"
                               "    duration,"
                               "    size,"
                               "    type,"
                               "    playposition,"
                               "    downloaded) "
                               "SELECT"
                               "    Entries.entryid,"
                               "    Enclosures.url,"
                               "    Enclosures.duration,"
                               "    Enclosures.size,"
                               "    Enclosures.type,"
                               "    Enclosures.playposition,"
                               "    Enclosures.downloaded "
                               "FROM Enclosures"
                               "    JOIN Entries ON Entries.id = Enclosures.id;")));

    TRUE_OR_RETURN(execute(QStringLiteral("DROP TABLE Enclosures;")));
    TRUE_OR_RETURN(execute(QStringLiteral("ALTER TABLE Enclosurestemp RENAME TO Enclosures;")));

    // Update Chapters table
    TRUE_OR_RETURN(
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Chapterstemp ("
                               "    entryid INTEGER,"
                               "    start INTEGER,"
                               "    title TEXT,"
                               "    link TEXT,"
                               "    image TEXT,"
                               "    FOREIGN KEY(entryid) REFERENCES Entries(entryid));")));

    TRUE_OR_RETURN(
        execute(QStringLiteral("INSERT INTO Chapterstemp ("
                               "    entryid,"
                               "    start,"
                               "    title,"
                               "    link,"
                               "    image) "
                               "SELECT"
                               "    Entries.entryid,"
                               "    Chapters.start,"
                               "    Chapters.title,"
                               "    Chapters.link,"
                               "    Chapters.image "
                               "FROM Chapters"
                               "    JOIN Entries ON Entries.id = Chapters.id;")));

    TRUE_OR_RETURN(execute(QStringLiteral("DROP TABLE Chapters;")));
    TRUE_OR_RETURN(execute(QStringLiteral("ALTER TABLE Chapterstemp RENAME TO Chapters;")));

    // Update Queue table
    TRUE_OR_RETURN(
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Queuetemp ("
                               "    listnr INTEGER,"
                               "    entryid INTEGER,"
                               "    playing BOOL,"
                               "    FOREIGN KEY(entryid) REFERENCES Entries(entryid));")));

    TRUE_OR_RETURN(
        execute(QStringLiteral("INSERT INTO Queuetemp ("
                               "    listnr,"
                               "    entryid,"
                               "    playing) "
                               "SELECT"
                               "    Queue.listnr,"
                               "    Entries.entryid,"
                               "    Queue.playing "
                               "FROM Queue"
                               "    JOIN Entries ON Entries.id = Queue.id;")));

    TRUE_OR_RETURN(execute(QStringLiteral("DROP TABLE Queue;")));
    TRUE_OR_RETURN(execute(QStringLiteral("ALTER TABLE Queuetemp RENAME TO Queue;")));

    // Split Authors table into FeedAuthors and EntryAuthors
    TRUE_OR_RETURN(
        execute(QStringLiteral("CREATE TABLE FeedAuthors ("
                               "    feedid INTEGER,"
                               "    name TEXT,"
                               "    email TEXT,"
                               "    FOREIGN KEY(feedid) REFERENCES Feeds(feedid));")));
    TRUE_OR_RETURN(
        execute(QStringLiteral("INSERT INTO FeedAuthors ("
                               "    feedid,"
                               "    name,"
                               "    email) "
                               "SELECT"
                               "    Feeds.feedid,"
                               "    Authors.name,"
                               "    Authors.email "
                               "FROM Authors"
                               "    JOIN Feeds ON Feeds.url = Authors.feed "
                               "WHERE"
                               "    Authors.id IS NULL OR Authors.id = '';")));

    TRUE_OR_RETURN(
        execute(QStringLiteral("CREATE TABLE EntryAuthors ("
                               "    entryid INTEGER,"
                               "    name TEXT,"
                               "    email TEXT,"
                               "    FOREIGN KEY(entryid) REFERENCES Entries(entryid));")));
    TRUE_OR_RETURN(
        execute(QStringLiteral("INSERT INTO EntryAuthors ("
                               "    entryid,"
                               "    name,"
                               "    email) "
                               "SELECT"
                               "    Entries.entryid,"
                               "    Authors.name,"
                               "    Authors.email "
                               "FROM Authors"
                               "    JOIN Entries ON Entries.id = Authors.id "
                               "WHERE"
                               "    Authors.id IS NOT NULL AND Authors.id != '';")));

    TRUE_OR_RETURN(execute(QStringLiteral("DROP TABLE Authors;")));

    // Update Error table
    TRUE_OR_RETURN(
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Errorstemp ("
                               "    type INTEGER,"
                               "    feedid INTEGER,"
                               "    entryid INTEGER,"
                               "    code INTEGER,"
                               "    message TEXT,"
                               "    date INTEGER,"
                               "    title TEXT);")));

    TRUE_OR_RETURN(
        execute(QStringLiteral("INSERT INTO Errorstemp ("
                               "    type,"
                               "    feedid,"
                               "    entryid,"
                               "    code,"
                               "    message,"
                               "    date,"
                               "    title) "
                               "SELECT"
                               "    Errors.type,"
                               "    Feeds.feedid,"
                               "    Entries.entryid,"
                               "    Errors.code,"
                               "    Errors.message,"
                               "    Errors.date,"
                               "    Errors.title "
                               "FROM Errors"
                               "    JOIN Entries ON Entries.id = Errors.id"
                               "    JOIN Feeds ON Feeds.url = Errors.url;")));

    TRUE_OR_RETURN(execute(QStringLiteral("DROP TABLE Errors;")));
    TRUE_OR_RETURN(execute(QStringLiteral("ALTER TABLE Errorstemp RENAME TO Errors;")));

    TRUE_OR_RETURN(execute(QStringLiteral("PRAGMA user_version = 12;")));
    // TODO: re-enable TRUE_OR_RETURN(commit());
    return true;
}

bool Database::execute(const QString &queryString)
{
    QSqlQuery q;
    q.prepare(queryString);
    return execute(q);
}

bool Database::execute(QSqlQuery &query)
{
    bool state = executeThread(query);

    if (!state) {
        Q_EMIT Database::instance().error(Error::Type::Database, 0, 0, query.lastError().type(), query.lastQuery(), query.lastError().text());
    }

    return state;
}

bool Database::transaction()
{
    // use raw sqlite query to benefit from automatic retries on execute
    QSqlQuery query;
    query.prepare(QStringLiteral("BEGIN TRANSACTION;"));
    return execute(query);
}

bool Database::commit()
{
    // use raw sqlite query to benefit from automatic retries on execute
    QSqlQuery query;
    query.prepare(QStringLiteral("COMMIT TRANSACTION;"));
    return execute(query);
}

bool Database::executeThread(QSqlQuery &query)
{
    int retries = 0;

    // NOTE that this will execute the query on the database that was specified
    // when the QSqlQuery was created.  There is no way to change that later on.
    while (!query.exec()) {
        // only retry if it failed due to the db being locked, see bug 500697
        if (query.lastError().nativeErrorCode() == QStringLiteral("5") && retries < m_maxRetries) {
            retries++;
            qCDebug(kastsDatabase) << "Failed to execute SQL Query; retrying (attempt" << retries << " of" << m_maxRetries << ")";
            qCDebug(kastsDatabase) << query.lastQuery();
            qCDebug(kastsDatabase) << query.lastError();
            QThread::usleep(retries * m_timeout + QRandomGenerator::global()->bounded(-m_timeoutRandomness, m_timeoutRandomness));
        } else {
            qCDebug(kastsDatabase) << "Failed to execute SQL Query";
            qCDebug(kastsDatabase) << query.lastQuery();
            qCDebug(kastsDatabase) << query.lastError();
            return false;
        }
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
    // delete rows with empty feed urls, as this should never happen, but could
    // occur when something goes wrong (like a crash) when trying to add a new
    // feed
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM Feeds WHERE url is NULL or url='';"));
    execute(query);
}
