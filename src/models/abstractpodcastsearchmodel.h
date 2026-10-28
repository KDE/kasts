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

class AbstractPodcastSearchModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

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

    virtual Q_INVOKABLE QString placeholderText() const = 0;
    virtual Q_INVOKABLE void search(const QString &text) = 0;
};
