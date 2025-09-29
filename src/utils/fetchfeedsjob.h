/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KJob>
#include <QVector>

#include "error.h"

class FetchFeedsJob : public KJob
{
    Q_OBJECT

public:
    explicit FetchFeedsJob(const QList<int> &feedids, QObject *parent = nullptr);

    void start() override;
    bool aborted();
    void abort();

Q_SIGNALS:
    void aborting();
    void logError(Error::Type type, const int &feedid, const int &entryid, const int errorId, const QString &errorString, const QString &title);

private:
    QList<int> m_feedids;

    void fetch();
    void monitorProgress();

    bool m_abort = false;
};
