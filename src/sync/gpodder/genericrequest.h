/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QNetworkReply>
#include <QObject>

#include "sync/syncutils.h"

class GenericRequest : public QObject
{
    Q_OBJECT

public:
    GenericRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent);

    int error() const;
    QString errorString() const;
    bool aborted() const;
    void abort();

Q_SIGNALS:
    void finished();
    void aborting();

protected:
    virtual void processResults() = 0;

    QString cleanupUrl(const QString &url) const;

    QNetworkReply *m_reply;
    SyncUtils::Provider m_provider;
    int m_error = 0;
    QString m_errorString;
    bool m_abort = false;
};
