/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QHash>
#include <QModelIndex>
#include <QObject>
#include <QVariant>
#include <QVector>

#include "models/abstractepisodemodel.h"

class EpisodeModel : public AbstractEpisodeModel
{
    Q_OBJECT

public:
    static EpisodeModel &instance()
    {
        static EpisodeModel _instance;
        return _instance;
    }
    explicit EpisodeModel(QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::UserRole) const override;
    int rowCount(const QModelIndex &parent) const override;

public Q_SLOTS:
    void updateInternalState() override;

private:
    QStringList m_entryIds;
    QVector<bool> m_read;
    QVector<bool> m_new;
    QVector<bool> m_favorite;
    QStringList m_titles;
    QStringList m_contents;
    QStringList m_feedNames;
};
