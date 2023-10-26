/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KJob>
#include <QVector>

#include "error.h"
#include "updatefeedjob.h"

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
    void logError(Error::Type type, const QString &url, const QString &id, const int errorId, const QString &errorString, const QString &title);

private:
    QStringList m_urls;

    void fetch();
    void monitorProgress();

    QVector<UpdateFeedJob *> m_feedjobs;
    bool m_abort = false;
};
