/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include "models/abstractepisodemodel.h"

class EntriesModel : public AbstractEpisodeModel
{
    Q_OBJECT

public:
    explicit EntriesModel(const qint64 feeduid, QObject *parent = nullptr);
    ~EntriesModel();

protected:
    const qint64 m_feeduid;
};
