/*
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVector>
#include <qqmlintegration.h>

class Feed : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(qint64 feeduid MEMBER m_feeduid CONSTANT)
    Q_PROPERTY(QString url MEMBER m_url NOTIFY urlChanged)
    Q_PROPERTY(QString name MEMBER m_name NOTIFY nameChanged)
    Q_PROPERTY(QString image MEMBER m_image NOTIFY imageChanged)
    Q_PROPERTY(QString link MEMBER m_link NOTIFY linkChanged)
    Q_PROPERTY(QString description MEMBER m_description NOTIFY descriptionChanged)
    Q_PROPERTY(QString authors MEMBER m_authors NOTIFY authorsChanged)
    Q_PROPERTY(QDateTime subscribed MEMBER m_subscribed CONSTANT)
    Q_PROPERTY(QDateTime lastUpdated MEMBER m_lastUpdated NOTIFY lastUpdatedChanged)
    Q_PROPERTY(bool refreshing MEMBER m_refreshing NOTIFY refreshingChanged)
    Q_PROPERTY(bool isSubscribed MEMBER m_isSubscribed CONSTANT)
    Q_PROPERTY(qint64 entryCount READ entryCount NOTIFY entryCountChanged)
    Q_PROPERTY(qint64 unreadCount READ unreadCount NOTIFY unreadCountChanged)
    Q_PROPERTY(qint64 newCount READ newCount NOTIFY newCountChanged)
    Q_PROPERTY(qint64 favoriteCount READ favoriteCount NOTIFY favoriteCountChanged)

public:
    explicit Feed(const qint64 feeduid, QObject *parent = nullptr);
    explicit Feed(const QString &url,
                  const QString &name,
                  const QString &image,
                  const QString &link,
                  const QString &description,
                  const QString &authors,
                  const QDateTime &lastUpdated,
                  QObject *parent = nullptr);
    ~Feed();

    qint64 entryCount() const;
    qint64 unreadCount() const;
    qint64 newCount() const;
    qint64 favoriteCount() const;

Q_SIGNALS:
    void urlChanged(const QString &url);
    void nameChanged(const QString &name);
    void imageChanged(const QString &image);
    void linkChanged(const QString &link);
    void descriptionChanged(const QString &description);
    void authorsChanged(const QString &authors);
    void lastUpdatedChanged(const QDateTime &lastUpdated);
    void dirnameChanged(const QString &dirname);
    void refreshingChanged(const bool status);
    void entryCountChanged();
    void unreadCountChanged();
    void newCountChanged();
    void favoriteCountChanged();

private:
    void updateFeed();

    qint64 m_feeduid;
    QString m_url;
    QString m_name;
    QString m_image;
    QString m_link;
    QString m_description;
    QString m_authors;
    QDateTime m_subscribed;
    QDateTime m_lastUpdated;
    QString m_dirname;
    bool m_refreshing;
    bool m_isSubscribed;
};
