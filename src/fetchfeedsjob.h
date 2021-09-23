/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
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

Q_SIGNALS:
    void abort();
    void logError(Error::Type type, const QString &url, const QString &id, const int errorId, const QString &errorString, const QString &title);

private:
    QStringList m_urls;

    void fetch();
    void monitorProgress();

    bool m_abort = false;
    QVector<UpdateFeedJob *> m_feedjobs;
};
