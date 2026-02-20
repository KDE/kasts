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

#include "models/entriesproxymodel.h"

class Feed : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(qint64 feeduid READ feeduid CONSTANT)
    Q_PROPERTY(QString url READ url CONSTANT)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString image READ image NOTIFY imageChanged)
    Q_PROPERTY(QString cachedImage READ cachedImage NOTIFY cachedImageChanged)
    Q_PROPERTY(QString link READ link NOTIFY linkChanged)
    Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)
    Q_PROPERTY(QString authors READ authors NOTIFY authorsChanged)
    Q_PROPERTY(bool refreshing READ refreshing WRITE setRefreshing NOTIFY refreshingChanged)
    Q_PROPERTY(QDateTime subscribed READ subscribed CONSTANT)
    Q_PROPERTY(QDateTime lastUpdated READ lastUpdated WRITE setLastUpdated NOTIFY lastUpdatedChanged)
    Q_PROPERTY(int unreadEntryCount READ unreadEntryCount NOTIFY unreadEntryCountChanged)
    Q_PROPERTY(int newEntryCount READ newEntryCount NOTIFY newEntryCountChanged)
    Q_PROPERTY(int favoriteEntryCount READ favoriteEntryCount NOTIFY favoriteEntryCountChanged)
    Q_PROPERTY(int errorId READ errorId WRITE setErrorId NOTIFY errorIdChanged)
    Q_PROPERTY(QString errorString READ errorString WRITE setErrorString NOTIFY errorStringChanged)
    Q_PROPERTY(EntriesProxyModel *entries MEMBER m_entries CONSTANT)

public:
    explicit Feed(const qint64 feeduid, QObject *parent = nullptr);
    ~Feed();

    void updateAuthors();

    qint64 feeduid() const;
    QString url() const;
    QString name() const;
    QString image() const;
    QString cachedImage() const;
    QString link() const;
    QString description() const;
    QString authors() const;
    QDateTime subscribed() const;
    QDateTime lastUpdated() const;
    QString dirname() const;
    int unreadEntryCount() const;
    int newEntryCount() const;
    int favoriteEntryCount() const;
    bool read() const;
    int errorId() const;
    QString errorString() const;

    bool refreshing() const;

    void setName(const QString &name);
    void setImage(const QString &image);
    void setLink(const QString &link);
    void setDescription(const QString &description);
    void setLastUpdated(const QDateTime &lastUpdated);
    void setDirname(const QString &dirname);
    void setRefreshing(bool refreshing);
    void setErrorId(int errorId);
    void setErrorString(const QString &errorString);

    Q_INVOKABLE void refresh();

Q_SIGNALS:
    void nameChanged(const QString &name);
    void imageChanged(const QString &image);
    void cachedImageChanged(const QString &imagePath);
    void linkChanged(const QString &link);
    void descriptionChanged(const QString &description);
    void authorsChanged(const QString &authors);
    void lastUpdatedChanged(const QDateTime &lastUpdated);
    void dirnameChanged(const QString &dirname);
    void entryCountChanged();
    void unreadEntryCountChanged();
    void newEntryCountChanged();
    void favoriteEntryCountChanged();
    void errorIdChanged(int errorId);
    void errorStringChanged(const QString &errorString);

    void refreshingChanged(bool refreshing);

private:
    void updateUnreadEntryCountFromDB();
    void updateNewEntryCountFromDB();
    void updateFavoriteEntryCountFromDB();
    void initFilterType(int value);
    void initSortType(int value);

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
    int m_errorId;
    QString m_errorString;
    int m_unreadEntryCount = -1;
    int m_newEntryCount = -1;
    int m_favoriteEntryCount = -1;
    bool m_refreshing = false;
    EntriesProxyModel *m_entries;
};
