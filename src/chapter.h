/**
 * SPDX-FileCopyrightText: 2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QDebug>
#include <QObject>
#include <QString>
#include <qqmlintegration.h>

#include "entry.h"

class Chapter : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(Entry *entry READ entry CONSTANT)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString link READ link NOTIFY linkChanged)
    Q_PROPERTY(QString image READ image NOTIFY imageChanged)
    Q_PROPERTY(QString cachedImage READ cachedImage NOTIFY cachedImageChanged)
    Q_PROPERTY(int start READ start NOTIFY startChanged)

public:
    Chapter(Entry *entry, const QString &title, const QString &link, const QString &image, const int &start, QObject *parent = nullptr);

    Entry *entry() const;
    QString title() const;
    QString link() const;
    QString image() const;
    QString cachedImage() const;
    int start() const;

    void setTitle(const QString &title, bool emitSignal = true);
    void setLink(const QString &link, bool emitSignal = true);
    void setImage(const QString &image, bool emitSignal = true);
    void setStart(const int &start, bool emitSignal = true);

Q_SIGNALS:
    void titleChanged(const QString &title);
    void linkChanged(const QString &link);
    void imageChanged(const QString &url);
    void cachedImageChanged(const QString &imagePath);
    void startChanged(const int &start);

private:
    Entry *m_entry = nullptr;
    QString m_title;
    QString m_link;
    QString m_image;
    int m_start;
};
