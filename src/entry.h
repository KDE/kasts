/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QDateTime>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QStringList>

#include "author.h"
#include "enclosure.h"
#include "feed.h"

class Entry : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Feed *feed READ feed CONSTANT)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString content READ content NOTIFY contentChanged)
    Q_PROPERTY(QVector<Author *> authors READ authors NOTIFY authorsChanged)
    Q_PROPERTY(QDateTime created READ created NOTIFY createdChanged)
    Q_PROPERTY(QDateTime updated READ updated NOTIFY updatedChanged)
    Q_PROPERTY(QString link READ link NOTIFY linkChanged)
    Q_PROPERTY(QString baseUrl READ baseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(bool read READ read WRITE setRead NOTIFY readChanged)
    Q_PROPERTY(bool new READ getNew WRITE setNew NOTIFY newChanged)
    Q_PROPERTY(bool favorite READ favorite WRITE setFavorite NOTIFY favoriteChanged)
    Q_PROPERTY(Enclosure *enclosure READ enclosure CONSTANT)
    Q_PROPERTY(bool hasEnclosure READ hasEnclosure NOTIFY hasEnclosureChanged)
    Q_PROPERTY(QString image READ image NOTIFY imageChanged)
    Q_PROPERTY(QString cachedImage READ cachedImage NOTIFY cachedImageChanged)
    Q_PROPERTY(bool queueStatus READ queueStatus WRITE setQueueStatus NOTIFY queueStatusChanged)

public:
    Entry(Feed *feed, const QString &id);
    ~Entry();

    QString id() const;
    QString title() const;
    QString content() const;
    QVector<Author *> authors() const;
    QDateTime created() const;
    QDateTime updated() const;
    QString link() const;
    bool read() const;
    bool getNew() const;
    bool favorite() const;
    Enclosure *enclosure() const;
    bool hasEnclosure() const;
    QString image() const;
    QString cachedImage() const;
    bool queueStatus() const;
    Feed *feed() const;

    QString baseUrl() const;

    void setRead(bool read);
    void setNew(bool state);
    void setFavorite(bool favorite);
    void setQueueStatus(bool status);

    Q_INVOKABLE QString adjustedContent(int width, int fontSize);

    void setNewInternal(bool state);
    void setReadInternal(bool read);
    void setFavoriteInternal(bool favorite);
    void setQueueStatusInternal(bool state);

Q_SIGNALS:
    void titleChanged(const QString &title);
    void contentChanged(const QString &content);
    void authorsChanged(const QVector<Author *> &authors);
    void createdChanged(const QDateTime &created);
    void updatedChanged(const QDateTime &updated);
    void linkChanged(const QString &link);
    void baseUrlChanged(const QString &baseUrl);
    void readChanged(bool read);
    void newChanged(bool state);
    void favoriteChanged(bool favorite);
    void hasEnclosureChanged(bool hasEnclosure);
    void imageChanged(const QString &url);
    void cachedImageChanged(const QString &imagePath);
    void queueStatusChanged(bool queueStatus);

private:
    void updateFromDb(bool emitSignals = true);
    void updateAuthors(bool emitSignals = true);
    void setTitle(const QString &title, bool emitSignal = true);
    void setContent(const QString &content, bool emitSignal = true);
    void setCreated(const QDateTime &created, bool emitSignal = true);
    void setUpdated(const QDateTime &updated, bool emitSignal = true);
    void setLink(const QString &link, bool emitSignal = true);
    void setHasEnclosure(bool hasEnclosure, bool emitSignal = true);
    void setImage(const QString &url, bool emitSignal = true);

    Feed *m_feed;
    QString m_id;
    QString m_title;
    QString m_content;
    QVector<Author *> m_authors;
    QDateTime m_created;
    QDateTime m_updated;
    QString m_link;
    bool m_read;
    bool m_new;
    bool m_favorite;
    Enclosure *m_enclosure = nullptr;
    QString m_image;
    bool m_hasenclosure = false;
};
