/*
 * Copyright 2020 Tobias Fella <fella@posteo.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FEED_H
#define FEED_H

#include <QDateTime>
#include <QObject>

#include "author.h"

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
    Q_PROPERTY(int autoUpdateCount READ autoUpdateCount WRITE setAutoUpdateCount NOTIFY autoUpdateCountChanged)
    Q_PROPERTY(int autoUpdateType READ autoUpdateType WRITE setAutoUpdateType NOTIFY autoUpdateCountChanged)
    Q_PROPERTY(bool notify READ notify WRITE setNotify NOTIFY notifyChanged)
    Q_PROPERTY(int entryCount READ entryCount NOTIFY entryCountChanged)
    Q_PROPERTY(int unreadEntryCount READ unreadEntryCount NOTIFY unreadEntryCountChanged)
    Q_PROPERTY(int errorId READ errorId WRITE setErrorId NOTIFY errorIdChanged)
    Q_PROPERTY(QString errorString READ errorString WRITE setErrorString NOTIFY errorStringChanged)

public:
    Feed(int index);

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
    int autoUpdateCount() const;
    int autoUpdateType() const;
    bool notify() const;
    int entryCount() const;
    int unreadEntryCount() const;
    bool read() const;
    int errorId() const;
    QString errorString() const;

    bool refreshing() const;

    void setName(QString name);
    void setImage(QString image);
    void setLink(QString link);
    void setDescription(QString description);
    void setAuthors(QVector<Author *> authors);
    void setDeleteAfterCount(int count);
    void setDeleteAfterType(int type);
    void setLastUpdated(QDateTime lastUpdated);
    void setAutoUpdateCount(int count);
    void setAutoUpdateType(int type);
    void setNotify(bool notify);
    void setRefreshing(bool refreshing);
    void setErrorId(int errorId);
    void setErrorString(QString errorString);

    Q_INVOKABLE void refresh();
    void remove();

Q_SIGNALS:
    void nameChanged(QString &name);
    void imageChanged(QString &image);
    void linkChanged(QString &link);
    void descriptionChanged(QString &description);
    void authorsChanged(QVector<Author *> &authors);
    void deleteAfterCountChanged(int count);
    void deleteAfterTypeChanged(int type);
    void lastUpdatedChanged(QDateTime lastUpdated);
    void autoUpdateCountChanged(int count);
    void autoUpdateTypeChanged(int type);
    void notifyChanged(bool notify);
    void entryCountChanged();
    void unreadEntryCountChanged();
    void errorIdChanged(int &errorId);
    void errorStringChanged(QString &errorString);

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
    int m_autoUpdateCount;
    int m_autoUpdateType;
    bool m_notify;
    int m_errorId;
    QString m_errorString;

    bool m_refreshing = false;
};

#endif // FEED_H
