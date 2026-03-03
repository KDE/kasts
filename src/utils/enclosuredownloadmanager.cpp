/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "enclosuredownloadmanager.h"

using namespace ThreadWeaver;

EnclosureDownloadManager::EnclosureDownloadManager(QObject *parent)
    : QObject(parent)
{
    // ThreadWeaver queue to handle the episode downloads
    m_queue = new Queue(this);
    m_queue->setMaximumNumberOfThreads(2);
    // TODO: make the number of thread configurable
}

EnclosureDownloadJob *EnclosureDownloadManager::download(const QString &url, const QString &filename)
{
    EnclosureDownloadJob *job = new EnclosureDownloadJob(url, filename);

    m_queue->stream() << job;

    return job;
}
