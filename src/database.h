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

    static bool execute(QSqlQuery &query);
    static bool execute(const QString &query, const QString &connectionName = QLatin1String(QSqlDatabase::defaultConnection));

    static bool transaction(const QString &connectionName = QLatin1String(QSqlDatabase::defaultConnection));
    static bool commit(const QString &connectionName = QLatin1String(QSqlDatabase::defaultConnection));

private:
    Database();
    int version();

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
    void cleanup();
    void setWalMode();

    inline static const QString m_dbName = QStringLiteral("database.db3");
};
