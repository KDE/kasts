/**
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include "kmediasession_export.h"

#include <QObject>
#include <QString>
#include <QUrl>

class KMEDIASESSION_EXPORT MetaData : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString artist READ artist WRITE setArtist NOTIFY artistChanged)
    Q_PROPERTY(QString album READ album WRITE setAlbum NOTIFY albumChanged)
    Q_PROPERTY(QUrl artworkUrl READ artworkUrl WRITE setArtworkUrl NOTIFY artworkUrlChanged)

public:
    explicit MetaData(QObject *parent = nullptr);
    ~MetaData();

    QString title() const;
    QString artist() const;
    QString album() const;
    QUrl artworkUrl() const;

Q_SIGNALS:
    void titleChanged(const QString &title);
    void artistChanged(const QString &artist);
    void albumChanged(const QString &album);
    void artworkUrlChanged(const QUrl &artworkUrl);

    void metaDataChanged(MetaData *metaData);

public Q_SLOTS:
    void setTitle(const QString &title);
    void setArtist(const QString &artist);
    void setAlbum(const QString &album);
    void setArtworkUrl(const QUrl &artworkUrl);

    Q_INVOKABLE void clear();

private:
    QString m_title;
    QString m_artist;
    QString m_album;
    QUrl m_artworkUrl;

    void signalMetaDataChanged();
};
