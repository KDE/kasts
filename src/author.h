/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

class Author : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString email READ email CONSTANT)
    Q_PROPERTY(QString url READ url CONSTANT)

public:
    Author(const QString &name, const QString &email, const QString &url, QObject *parent = nullptr);

    QString name() const;
    QString email() const;
    QString url() const;

private:
    QString m_name;
    QString m_email;
    QString m_url;
};
