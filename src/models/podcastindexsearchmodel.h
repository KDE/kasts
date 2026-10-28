/**
 * SPDX-FileCopyrightText: 2021 Tobias Fella <tobias.fella@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>
#include <QVariant>

#include "models/abstractpodcastsearchmodel.h"

class PodcastIndexSearchModel : public AbstractPodcastSearchModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit PodcastIndexSearchModel(QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent) const override;

    Q_INVOKABLE QString placeholderText() const override;
    Q_INVOKABLE void search(const QString &text) override;

private:
    QJsonObject m_data;
};
