/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QSqlQuery>
#include <QString>

#include "error.h"

class Database : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static Database &instance()
    {
        static Database _instance;
        return _instance;
    }
    static Database *create(QQmlEngine *engine, QJSEngine *)
    {
        engine->setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
        return &instance();
    }

    static void openDatabase(const QString &connectionName = QLatin1String(QSqlDatabase::defaultConnection));
    static void closeDatabase(const QString &connectionName = QLatin1String(QSqlDatabase::defaultConnection));

    bool execute(QSqlQuery &query);
    bool transaction();
    bool commit();

    // to be used in separate threads; error reporting has to be done manually in thread!
    static bool executeThread(QSqlQuery &query);

Q_SIGNALS:
    void error(Error::Type type, const QString &url, const QString &id, const int errorId, const QString &errorString, const QString &title);

private:
    Database();
    int version();

    bool execute(const QString &queryString);

    bool migrate();
    bool migrateTo1();
    bool migrateTo2();
    bool migrateTo3();
    bool migrateTo4();
    bool migrateTo5();
    bool migrateTo6();
    bool migrateTo7();
    bool migrateTo8();
    bool migrateTo9();
    bool migrateTo10();
    bool migrateTo11();
    void cleanup();
    void setWalMode();

    inline static const int m_timeout = 500000; // retry timeout for db retries in microseconds
    inline static const int m_timeoutRandomness = 100000; // some randomness on top of retry interval
    inline static const int m_maxRetries = 5; // maximum amount of db retries
    inline static const QString m_dbName = QStringLiteral("database.db3");
};
