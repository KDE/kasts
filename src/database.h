/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QSqlQuery>

class Database : public QObject
{
    Q_OBJECT

public:
    static Database &instance()
    {
        static Database _instance;
        return _instance;
    }
    bool execute(QSqlQuery &query);
    bool execute(QString query);
    Q_INVOKABLE void addFeed(QString url);
    Q_INVOKABLE void importFeeds(QString path);
    Q_INVOKABLE void exportFeeds(QString path);

Q_SIGNALS:
    void feedAdded(QString url);

private:
    Database();
    int version();

    bool migrate();
    bool migrateTo1();
    void cleanup();
    bool feedExists(QString url);
};
