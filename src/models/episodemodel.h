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

class EpisodeModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Type {
        All,
        New,
        Unread,
        Downloading,
        Downloaded,
        PartiallyDownloaded,
    };
    Q_ENUM(Type)

    Q_PROPERTY(Type type READ type WRITE setType)

    explicit EpisodeModel();
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;

    Type type() const;

public Q_SLOTS:
    void setType(Type type);

private:
    Type m_type = Type::All;
};
