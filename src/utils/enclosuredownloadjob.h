/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QNetworkReply>
#include <ThreadWeaver/Job>
#include <qtmetamacros.h>

#include "networkaccessmanager.h"

class EnclosureDownloadJob : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT

public:
    explicit EnclosureDownloadJob(const QString &url, const QString &filename, QObject *parent = nullptr);

    void run(ThreadWeaver::JobPointer, ThreadWeaver::Thread *) override;
    void requestAbort() override;
    int error() const;
    QString errorString() const;
    bool isComplete() const;

Q_SIGNALS:
    void finished() const;
    void processedAmountChanged(const qulonglong m_processedAmount, const qulonglong m_totalAmount) const;

private:
    void startDownload();
    QNetworkReply *download(const QString &url, const QString &filePath) const;

    QString m_url;
    QString m_filename;
    int m_error;
    QString m_errorString;
    qulonglong m_processedAmount;
    qulonglong m_totalAmount;
    bool m_complete = false;

    QNetworkReply *m_reply = nullptr;
    NetworkAccessManager *m_manager = nullptr;
};
