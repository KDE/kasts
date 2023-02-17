/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QHash>
#include <QModelIndex>
#include <QObject>
#include <QVariant>

#include "models/abstractepisodemodel.h"

class Feed;

class EntriesModel : public AbstractEpisodeModel
{
    Q_OBJECT

public:
    explicit EntriesModel(Feed *feed);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent) const override;

    Feed *feed() const;

public Q_SLOTS:
    void updateInternalState() override;

protected:
    Feed *m_feed;
};
