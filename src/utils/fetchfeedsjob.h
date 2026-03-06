/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KJob>
#include <QVector>

class FetchFeedsJob : public KJob
{
    Q_OBJECT

public:
    explicit FetchFeedsJob(const QStringList &urls, QObject *parent = nullptr);

    void start() override;
    bool aborted();
    void abort();

Q_SIGNALS:
    void aborting();

private:
    QStringList m_urls;
    QList<qint64> m_feeduids;

    void fetch();
    void monitorProgress();

    bool m_abort = false;
};
