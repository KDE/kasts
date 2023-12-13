/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "author.h"

Author::Author(const QString &name, const QString &email, const QString &url, QObject *parent)
    : QObject(parent)
    , m_name(name)
    , m_email(email)
    , m_url(url)
{
}

QString Author::name() const
{
    return m_name;
}

QString Author::email() const
{
    return m_email;
}

QString Author::url() const
{
    return m_url;
}
