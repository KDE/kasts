/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KJob>

class StorageMoveJob : public KJob
{
public:
    explicit StorageMoveJob(const QString &from, const QString &to, QStringList &list, QObject *parent = nullptr);

    void start() override;
    bool doKill() override;

private:
    void moveFiles();

    QString m_from;
    QString m_to;
    QStringList m_list;
    bool m_abort = false;
};
