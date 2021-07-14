/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QVariant>

#include "error.h"

class ErrorLogModel : public QAbstractListModel
{
    Q_OBJECT

public:
    static ErrorLogModel &instance()
    {
        static ErrorLogModel _instance;
        return _instance;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;

    Q_INVOKABLE void clearAll();

public:
    void
    monitorErrorMessages(const Error::Type type, const QString &url, const QString &id, const int errorCode, const QString &errorString, const QString &title);

Q_SIGNALS:
    void newErrorLogged(Error *error);

private:
    explicit ErrorLogModel();

    QList<Error *> m_errors;
};
