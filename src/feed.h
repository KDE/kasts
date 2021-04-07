/*
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FEED_H
#define FEED_H

#include <QDateTime>
#include <QObject>

#include "author.h"

class EntriesModel;

class Feed : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString url READ url CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString image READ image WRITE setImage NOTIFY imageChanged)
    Q_PROPERTY(QString link READ link WRITE setLink NOTIFY linkChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QVector<Author *> authors READ authors WRITE setAuthors NOTIFY authorsChanged)
    Q_PROPERTY(bool refreshing READ refreshing WRITE setRefreshing NOTIFY refreshingChanged)
    Q_PROPERTY(int deleteAfterCount READ deleteAfterCount WRITE setDeleteAfterCount NOTIFY deleteAfterCountChanged)
    Q_PROPERTY(int deleteAfterType READ deleteAfterType WRITE setDeleteAfterType NOTIFY deleteAfterTypeChanged)
    Q_PROPERTY(QDateTime subscribed READ subscribed CONSTANT)
    Q_PROPERTY(QDateTime lastUpdated READ lastUpdated WRITE setLastUpdated NOTIFY lastUpdatedChanged)
    Q_PROPERTY(bool notify READ notify WRITE setNotify NOTIFY notifyChanged)
    Q_PROPERTY(int entryCount READ entryCount NOTIFY entryCountChanged)
    Q_PROPERTY(int unreadEntryCount READ unreadEntryCount NOTIFY unreadEntryCountChanged)
    Q_PROPERTY(int newEntryCount READ newEntryCount NOTIFY newEntryCountChanged)
    Q_PROPERTY(int errorId READ errorId WRITE setErrorId NOTIFY errorIdChanged)
    Q_PROPERTY(QString errorString READ errorString WRITE setErrorString NOTIFY errorStringChanged)
    Q_PROPERTY(EntriesModel *entries MEMBER m_entries CONSTANT)

public:
    Feed(int index);
    Feed(QString const feedurl);

    ~Feed();

    QString url() const;
    QString name() const;
    QString image() const;
    QString link() const;
    QString description() const;
    QVector<Author *> authors() const;
    int deleteAfterCount() const;
    int deleteAfterType() const;
    QDateTime subscribed() const;
    QDateTime lastUpdated() const;
    bool notify() const;
    int entryCount() const;
    int unreadEntryCount() const;
    int newEntryCount() const;
    bool read() const;
    int errorId() const;
    QString errorString() const;

    bool refreshing() const;

    void setName(const QString &name);
    void setImage(const QString &image);
    void setLink(const QString &link);
    void setDescription(const QString &description);
    void setAuthors(const QVector<Author *> &authors);
    void setDeleteAfterCount(int count);
    void setDeleteAfterType(int type);
    void setLastUpdated(const QDateTime &lastUpdated);
    void setNotify(bool notify);
    void setRefreshing(bool refreshing);
    void setErrorId(int errorId);
    void setErrorString(const QString &errorString);

    Q_INVOKABLE void refresh();

Q_SIGNALS:
    void nameChanged(const QString &name);
    void imageChanged(const QString &image);
    void linkChanged(const QString &link);
    void descriptionChanged(const QString &description);
    void authorsChanged(const QVector<Author *> &authors);
    void deleteAfterCountChanged(int count);
    void deleteAfterTypeChanged(int type);
    void lastUpdatedChanged(const QDateTime &lastUpdated);
    void notifyChanged(bool notify);
    void entryCountChanged();
    void unreadEntryCountChanged();
    void newEntryCountChanged();
    void errorIdChanged(int &errorId);
    void errorStringChanged(const QString &errorString);

    void refreshingChanged(bool refreshing);

private:
    QString m_url;
    QString m_name;
    QString m_image;
    QString m_link;
    QString m_description;
    QVector<Author *> m_authors;
    int m_deleteAfterCount;
    int m_deleteAfterType;
    QDateTime m_subscribed;
    QDateTime m_lastUpdated;
    bool m_notify;
    int m_errorId;
    QString m_errorString;
    EntriesModel *m_entries;

    bool m_refreshing = false;
};

#endif // FEED_H
