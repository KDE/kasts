/**
 * SPDX-FileCopyrightText: 2021 Tobias Fella <tobias.fella@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>
#include <QVariant>

class PodcastSearchModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        NameRole = Qt::DisplayRole,
        IdRole = Qt::UserRole,
        UrlRole,
        OriginalUrlRole,
        LinkRole,
        DescriptionRole,
        AuthorRole,
        OwnerNameRole,
        ImageRole,
        ArtworkRole,
        LastUpdatedRole,
        LastCrawlTimeRole,
        LastParseTimeRole,
        LastGoodHttpStatusTimeRole,
        LastHttpStatusRole,
        ContentTypeRole,
        ItunesIdRole,
        GeneratorRole,
        LanguageRole,
        TypeRole,
        DeadRole,
        CrawlErrorsRole,
        ParseErrorsRole,
        CategoriesRole,
        LockedRole,
        ImageUrlHashRole,
    };
    explicit PodcastSearchModel(QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;

    Q_INVOKABLE void search(const QString &text);

private:
    QJsonObject m_data;
};
