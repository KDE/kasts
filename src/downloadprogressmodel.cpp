/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "downloadprogressmodel.h"
#include "datamanager.h"

DownloadProgressModel::DownloadProgressModel()
    : QAbstractListModel(nullptr)
{
    m_entries.clear();
}

QVariant DownloadProgressModel::data(const QModelIndex &index, int role) const
{
    if (role != 0)
        return QVariant();
    return QVariant::fromValue(DataManager::instance().getEntry(m_entries[index.row()]));
}

QHash<int, QByteArray> DownloadProgressModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[0] = "entry";
    return roleNames;
}

int DownloadProgressModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_entries.count();
}

void DownloadProgressModel::monitorDownloadProgress(Entry *entry, Enclosure::Status status)
{
    // qDebug() << "download status changed:" << entry->title() << status;
    if (status == Enclosure::Downloading && !m_entries.contains(entry->id())) {
        // qDebug() << "inserting dowloading entry" << entry->id() << "in position" << m_entries.count();
        beginInsertRows(QModelIndex(), m_entries.count(), m_entries.count());
        m_entries += entry->id();
        endInsertRows();
    }
    if (status != Enclosure::Downloading && m_entries.contains(entry->id())) {
        int index = m_entries.indexOf(entry->id());
        // qDebug() << "removing dowloading entry" << entry->id() << "in position" << index;
        beginRemoveRows(QModelIndex(), index, index);
        m_entries.removeAt(index);
        endRemoveRows();
    }
}
