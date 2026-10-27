/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QQmlEngine>

#include "models/abstractepisodeproxymodel.h"
#include "models/unsubscribedentriesmodel.h"

class Feed;

class UnsubscribedEntriesProxyModel : public AbstractEpisodeProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit UnsubscribedEntriesProxyModel(const QString &url, QObject *parent = nullptr);
    ~UnsubscribedEntriesProxyModel();

private:
    UnsubscribedEntriesModel *m_entriesModel;
};
