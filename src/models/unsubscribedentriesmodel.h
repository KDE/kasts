/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QQmlEngine>

#include "datatypes.h"
#include "models/abstractepisodemodel.h"

class UnsubscribedEntriesModel : public AbstractEpisodeModel
{
    Q_OBJECT

public:
    explicit UnsubscribedEntriesModel(const QString &url, QObject *parent = nullptr);
    ~UnsubscribedEntriesModel();
    virtual QVariant data(const QModelIndex &index, int role = Qt::UserRole) const override;
    virtual int rowCount(const QModelIndex &parent) const override;

protected:
    void updateEntries(const DataTypes::FeedUpdateDetails &feedDetails);

    QString m_url;

    // override the data members to use the *update* versions of the types
    DataTypes::FeedUpdateDetails m_feed;
};
