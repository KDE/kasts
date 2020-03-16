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

#include <QDir>
#include <QStandardPaths>
#include <QSqlError>
#include <QSqlDatabase>

#include "database.h"
#include "alligator-debug.h"

#define TRUE_OR_RETURN(x) if(!x) return false;

Database::Database()
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    QString databasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(databasePath).mkpath(databasePath);
    db.setDatabaseName(databasePath + QStringLiteral("/database.db3"));
    qCDebug(ALLIGATOR) << "Opening database " << databasePath << "/database.db3";
    db.open();

    if(!migrate()) {
        qCCritical(ALLIGATOR) << "Failed to migrate the database";
    }
}

bool Database::migrate() {
    qCDebug(ALLIGATOR) << "Migrating database";
    if(version() < 1) TRUE_OR_RETURN(migrateTo1());
    return true;
}

bool Database::migrateTo1() {
    QSqlQuery query(QSqlDatabase::database());
    TRUE_OR_RETURN(execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Feeds (name TEXT, url TEXT);")));
    TRUE_OR_RETURN(execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Entries (feed TEXT, id TEXT, title TEXT, content TEXT);")));
    TRUE_OR_RETURN(execute(QStringLiteral("CREATE TABLE IF NOT EXISTS Authors (id TEXT, name TEXT, uri TEXT, email TEXT);")));
    TRUE_OR_RETURN(execute(QStringLiteral("PRAGMA user_version = 1;")));
    return true;
}

bool Database::execute(QString query) {
    QSqlQuery q;
    q.prepare(query);
    return execute(q);
}

bool Database::execute(QSqlQuery &query) {
    if(!query.exec()) {
        qCWarning(ALLIGATOR) << "Failed to execute SQL Query";
        qCWarning(ALLIGATOR) << query.lastQuery();
        qCWarning(ALLIGATOR) << query.lastError();
        return false;
    }
    return true;
}

int Database::version() {
    QSqlQuery query;
    query.prepare(QStringLiteral("PRAGMA user_version;"));
    execute(query);
    if(query.next()) {
        bool ok;
        int value = query.value(0).toInt(&ok);
        qCDebug(ALLIGATOR) << "Database version " << value;
        if(ok) return value;
    } else {
        qCCritical(ALLIGATOR) << "Failed to check database version";
    }
    return -1;
}
