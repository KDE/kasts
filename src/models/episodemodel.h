/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

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

private:
    explicit EpisodeModel(QObject *parent = nullptr);
};
