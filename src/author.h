/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef AUTHOR_H
#define AUTHOR_H

#include <QObject>

class Author : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString email READ email CONSTANT)
    Q_PROPERTY(QString url READ url CONSTANT)

public:
    Author(QString name, QString email, QString url, QObject *parent = nullptr);
    ~Author();

    QString name() const;
    QString email() const;
    QString url() const;

private:
    QString m_name;
    QString m_email;
    QString m_url;
};

#endif // AUTHOR_H
