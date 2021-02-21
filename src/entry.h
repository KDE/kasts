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
    Q_PROPERTY(Enclosure *enclosure READ enclosure CONSTANT);

public:
    Entry(Feed *feed, int index);
    ~Entry();

    QString id() const;
    QString title() const;
    QString content() const;
    QVector<Author *> authors() const;
    QDateTime created() const;
    QDateTime updated() const;
    QString link() const;
    bool read() const;
    Enclosure *enclosure() const;
    Feed *feed() const;

    QString baseUrl() const;

    void setRead(bool read);

    Q_INVOKABLE QString adjustedContent(int width, int fontSize);

Q_SIGNALS:
    void readChanged(bool read);

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
    Enclosure *m_enclosure = nullptr;
};

#endif // ENTRY_H
