/**
 * SPDX-FileCopyrightText: 2021 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QVariant>

#include "errorlogmodel.h"
#include "feed.h"

class PodcastSearchModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        Id,
        Title,
        Url,
        OriginalUrl,
        Link,
        Description,
        Author,
        OwnerName,
        Image,
        Artwork,
        LastUpdateTime,
        LastCrawlTime,
        LastParseTime,
        LastGoodHttpStatusTime,
        LastHttpStatus,
        ContentType,
        ItunesId,
        Generator,
        Language,
        Type,
        Dead,
        CrawlErrors,
        ParseErrors,
        Categories,
        Locked,
        ImageUrlHash,
    };
    explicit PodcastSearchModel(QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;

    Q_INVOKABLE void search(const QString &text);

private:
    QJsonObject m_data;
};
