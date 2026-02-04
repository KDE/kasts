/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QQmlEngine>

#include "models/abstractepisodeproxymodel.h"
#include "models/entriesmodel.h"

class Feed;

class EntriesProxyModel : public AbstractEpisodeProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit EntriesProxyModel(const qint64 feeduid, QObject *parent = nullptr);

private:
    EntriesModel *m_entriesModel;
};
