/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QQmlEngine>
#include <QString>

#include "datatypes.h"

class AbstractPodcastSearchModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString placeholderText READ placeholderText CONSTANT)

public:
    enum Roles {
        NameRole = Qt::DisplayRole,
        UrlRole = Qt::UserRole,
        ImageRole,
        LinkRole,
        DescriptionRole,
        AuthorRole,
        LastUpdatedRole,
    };

    explicit AbstractPodcastSearchModel(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual int rowCount(const QModelIndex &parent) const override;

    virtual QString name() const = 0;
    virtual QString placeholderText() const = 0;
    virtual Q_INVOKABLE void search(const QString &text) = 0;

protected:
    QList<DataTypes::FeedDetails> m_feeds;
};
