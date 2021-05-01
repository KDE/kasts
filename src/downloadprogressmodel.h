/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QVariant>

#include "enclosure.h"
#include "entry.h"

class DownloadProgressModel : public QAbstractListModel
{
    Q_OBJECT

public:
    static DownloadProgressModel &instance()
    {
        static DownloadProgressModel _instance;
        return _instance;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;

public Q_SLOTS:
    void monitorDownloadProgress(Entry *entry, Enclosure::Status status);

private:
    explicit DownloadProgressModel();
    QStringList m_entries;
};
