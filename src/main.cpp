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

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QUrl>
#include <QStandardPaths>
#include <QDir>

#include "entryListModel.h"
#include "feedListModel.h"

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setApplicationName(QStringLiteral("Alligator"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1"));

    qmlRegisterType<FeedListModel>("org.kde.alligator", 1, 0, "FeedListModel");
    qmlRegisterType<EntryListModel>("org.kde.alligator", 1, 0, "EntryListModel");

    QQmlApplicationEngine engine;

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    QString databasePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir(databasePath).mkpath(databasePath);
    db.setDatabaseName(databasePath + "/database.db3");
    db.open();

    QSqlQuery query = QSqlQuery(db);
    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS Feeds (name TEXT, url TEXT);"));
    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS Entries (feed TEXT, id TEXT UNIQUE, title TEXT, content TEXT);"));
    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS Authors (id TEXT, name TEXT, uri TEXT, email TEXT);"));

    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
