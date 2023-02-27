/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include "models/abstractepisodeproxymodel.h"
#include "models/entriesmodel.h"

class Feed;

class EntriesProxyModel : public AbstractEpisodeProxyModel
{
    Q_OBJECT

    Q_PROPERTY(Feed *feed READ feed CONSTANT)

public:
    explicit EntriesProxyModel(Feed *feed);

    Feed *feed() const;

private:
    EntriesModel *m_entriesModel;
};
