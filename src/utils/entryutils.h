/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>

#include "datatypes.h"

class EntryUtils : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    EntryUtils(QObject *parent = nullptr);

    static QString entryImage(const QString &entryImage,
                              const QString &feedImage,
                              const QString &enclosureUrl,
                              const DataTypes::EnclosureStatus enclosureStatus,
                              const QString &entryTitle,
                              const QString &feedDirname);
    static QString
    cachedEmbeddedImage(const QString &enclosureUrl, const DataTypes::EnclosureStatus enclosureStatus, const QString &entryTitle, const QString &feedDirname);
    static qint64 checkSizeOnDisk(const qint64 entryuid, const QString &filename, const qint64 size, bool updateStatus = true);
    Q_INVOKABLE static QString adjustedContent(const int width, const int fontSize, const QString &content, const QString &link);
    Q_INVOKABLE static QString baseUrl(const QString &link);
};
