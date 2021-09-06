/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QItemSelection>
#include <QObject>
#include <QVariant>

#include "enclosure.h"
#include "entry.h"

class DownloadModel : public QAbstractListModel
{
    Q_OBJECT

public:
    static DownloadModel &instance()
    {
        static DownloadModel _instance;
        return _instance;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;

    Q_INVOKABLE QItemSelection createSelection(int rowa, int rowb);

public Q_SLOTS:
    void monitorDownloadStatus();

private:
    explicit DownloadModel();

    void updateInternalState();

    QStringList m_downloadingIds;
    QStringList m_partiallyDownloadedIds;
    QStringList m_downloadedIds;
    QStringList m_entryIds;

    int m_downloadingCount = 0;
    int m_partiallyDownloadedCount = 0;
    int m_downloadedCount = 0;
    int m_entryCount = 0;
};
