/**
 * SPDX-FileCopyrightText: 2026 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "utils/networkaccessmanagerfactory.h"

#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QQmlNetworkAccessManagerFactory>
#include <QStandardPaths>

#include "utils/networkaccessmanager.h"
#include "utils/storagemanager.h"

QNetworkAccessManager *NetworkAccessManagerFactory::create(QObject *parent)
{
    QNetworkAccessManager *manager = new NetworkAccessManager(parent);
    auto cache = new QNetworkDiskCache(manager);
    QString directory = StorageManager::instance().imageDirPath();
    cache->setCacheDirectory(directory);
    cache->setMaximumCacheSize(500 * 1024 * 1024);
    manager->setCache(cache);
    return manager;
}
