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
#include <QVector>

class EpisodeModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        EntryRole = Qt::UserRole,
        IdRole,
        ReadRole,
        NewRole,
    };
    Q_ENUM(Roles)

    explicit EpisodeModel();
    QVariant data(const QModelIndex &index, int role = Qt::UserRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;

public Q_SLOTS:
    void updateInternalState();

private:
    QStringList m_entryIds;
    QVector<bool> m_read;
    QVector<bool> m_new;
};
