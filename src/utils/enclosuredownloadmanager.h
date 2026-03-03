/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>
#include <ThreadWeaver/Queue>

#include "enclosuredownloadjob.h"

class EnclosureDownloadManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static EnclosureDownloadManager &instance()
    {
        static EnclosureDownloadManager _instance;
        return _instance;
    }
    static EnclosureDownloadManager *create(QQmlEngine *engine, QJSEngine *)
    {
        engine->setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
        return &instance();
    }

    EnclosureDownloadJob *download(const QString &url, const QString &filename);

private:
    EnclosureDownloadManager(QObject *parent = nullptr);

    ThreadWeaver::Queue *m_queue;
};
