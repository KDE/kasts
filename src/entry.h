/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef ENTRY_H
#define ENTRY_H

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
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QString content READ content CONSTANT)
    Q_PROPERTY(QVector<Author *> authors READ authors CONSTANT)
    Q_PROPERTY(QDateTime created READ created CONSTANT)
    Q_PROPERTY(QDateTime updated READ updated CONSTANT)
    Q_PROPERTY(QString link READ link CONSTANT)
    Q_PROPERTY(QString baseUrl READ baseUrl CONSTANT)
    Q_PROPERTY(bool read READ read WRITE setRead NOTIFY readChanged);
    Q_PROPERTY(bool new READ getNew WRITE setNew NOTIFY newChanged);
    Q_PROPERTY(Enclosure *enclosure READ enclosure CONSTANT);
    Q_PROPERTY(bool hasEnclosure READ hasEnclosure CONSTANT);
    Q_PROPERTY(QString image READ image WRITE setImage NOTIFY imageChanged)

public:
    Entry(Feed *feed, QString id);
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
    Enclosure *enclosure() const;
    bool hasEnclosure() const;
    QString image() const;
    Feed *feed() const;

    QString baseUrl() const;

    void setRead(bool read);
    void setNew(bool state);
    void setImage(const QString &url);

    Q_INVOKABLE QString adjustedContent(int width, int fontSize);

Q_SIGNALS:
    void readChanged(bool read);
    void newChanged(bool state);
    void imageChanged(const QString &url);

private:
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
    Enclosure *m_enclosure = nullptr;
    QString m_image;
    bool m_hasenclosure = false;
};

#endif // ENTRY_H
